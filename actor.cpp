#include "common.h"
#include "map.h"
#include "actor.h"
#include "debuglog.h"
#include "mapwho.h" 
#include "mapwhoentry.h"
#include "collisionseekingthing.h"
#include "eventmanager.h"
#include "Profile.h"
#include "game.h"
#include "cliparams.h"
 

//******************************************************************************************
void	CActor::Init(CInitThing* init)
{
	mLastTimeOnGround=-100.f;
	mLastTimeInWater=-100.f;
	mLastTimeOnObject=-100.0f;
	mLastMoveTime = EVENT_MANAGER.GetTime() ;

	// SRG when initalising collision stuff in Thing::Init is may call GetVelocity() 
	// so setting up the attribute here 
	mVelocity	= init->mVelocity ;

	// SRG collision stuff also needs oldPos to be sensible aswell
	mOldPos			= init->mPos;

	if (init->mOrientationType == EULER_ANGLES)
		mOldOrientation = FMatrix(init->mYaw, init->mPitch, init->mRoll);
	else if (init->mOrientationType == DIRECTION_COSINE_MATRIX)
		mOldOrientation = init->mOrientation; 

	SUPERTYPE::Init(init);

	int r = (int)GetMoveMultiplier() ;

	mDoFullMoveCount = GAME.Random() % r ;

	AddMoveEvent(NULL);
}


//******************************************************************************************
CActor::~CActor()
{

}

//******************************************************************************************
// don't add to this without my say so (SRG)  
// A basic low fidelity move function which has to be called every frame which the normal
// move function was not called on.  This is here in order for the collision to work properly.
void CActor::LowFidelityMove()
{
	PROFILE_FN(ActorLFMove);
	mOldPos = mPos;
	mPos+=mVelocity;
	
	if (ClipToGround())
	{
		float		alt=MAP.Collide(mPos)-COfGHeight();

		if (alt <=mPos.Z)
		{	
			mPos.Z=alt;
			mLastTimeOnGround = EVENT_MANAGER.GetTime();
		}
	}
}


//******************************************************************************************
void CActor::Move()
{
	PROFILE_FN(ActorMove);
	mLastMoveTime = EVENT_MANAGER.GetTime();

	// store old position
	mOldPos = mPos;
	mOldOrientation = mOrientation;

	CSector previous_sector;
	if (GetMapWhoEntry())
		previous_sector=mMapWhoEntry.GetSectorIn();

	if (CLIPARAMS.mDeveloperMode)
	{
		// ### TEMP debug check to make sure we don't go above max velocity

		if (IsA(THING_TYPE_UNIT))
		{
			float	m=mVelocity.MagnitudeXY();
			float	mv=GetMaxVelocity()/GAME_FR;
			if (m>(mv+0.001f))
			{
			//	LOG.AddMessage("Warning object %s travling beyond max velocity (%2.8f per sec) mv/s = %2.8f", _GetClassName(), (m-mv) * GAME_FR, GetMaxVelocity()  ) ;
			}
		}
		else
		{

			float	m=mVelocity.Magnitude();
			float	mv=GetMaxVelocity()/GAME_FR;
			if (m>(mv+0.001f))
			{
			//	LOG.AddMessage("Warning object %s travling beyond max velocity (%2.8f per sec) mv/s = %2.8f", _GetClassName(), (m-mv) * GAME_FR, GetMaxVelocity()  ) ;
			}
		}
	}

	// SRG end temp check

	mPos+=mVelocity;

	float water_height = MAP.GetWaterLevel()-COfGHeight();

	if (ClipToGround())
	{
		float		alt=MAP.Collide(mPos)-COfGHeight();

		if (alt <=mPos.Z)
		{	
		
			mPos.Z=alt;

			// sorry last minute hack stops you bouncing down hills
			if (mFlags & TF_SLIDE)
			{		
				// slide like you did in moho
				FVector slidePlaneNormal = MAP.Normal(mPos) ;
				float backoff = (mVelocity * slidePlaneNormal);
				mVelocity -=slidePlaneNormal*backoff;
				// not on ground !
			}
			else
			{
				DeclareOnGround();
				if (fabsf(mVelocity.Z)>Gravity()+0.05f)
					mVelocity.Z=-mVelocity.Z*BounceFactor();
				else
					mVelocity.Z=0;
			}
		}

		//if (water_height <= mPos.Z) mPos.Z = water_height 

			if (water_height <= mPos.Z)
		DeclareInWater();
	} 



	if (GetMapWhoEntry())
	{
		if (mMapWhoEntry.UpdateEntry(mOldPos) && mCollisionSeekingThing)
		{
			mCollisionSeekingThing->MovedSector(previous_sector, mMapWhoEntry.GetSectorIn()) ;
		}
	}


	/*
	// If we've moved, remove some snow

	if ((mOldPos!=mPos) || (mOldOrientation!=mOldOrientation))
	{
		mSnowDensity-=8; // Needs to be greater than the snow addition value as snow will still be added!
		if (mSnowDensity<0)
			mSnowDensity=0;
	}
	*/
}


//******************************************************************************************
// used by collsion etc ( resets where the thing has moved over the frame )
void	CActor::MoveTo(const FVector& pos)
{
//	mOldPos = mPos ;
	SUPERTYPE::MoveTo(pos) ;
}

//******************************************************************************************
void	CActor::Teleport(const FVector& pos)
{
	CSector previous_sector;
	if (GetMapWhoEntry())
		previous_sector=mMapWhoEntry.GetSectorIn();

	mOldPos = pos ;

	SUPERTYPE::Teleport(pos) ;

	if (GetMapWhoEntry())
	{
		if (mMapWhoEntry.UpdateEntry(mOldPos) && mCollisionSeekingThing)
		{
			mCollisionSeekingThing->MovedSector(previous_sector, mMapWhoEntry.GetSectorIn()) ;
		}
	}
}

//******************************************************************************************
void	CActor::TeleportOrientation(const FMatrix& orientation)
{
	mOldOrientation=orientation;
	SUPERTYPE::TeleportOrientation(orientation);
}


//******************************************************************************************
void	CActor::AddMoveEvent(CEvent* event)
{
	if (!(mFlags & TF_DECLARED_SHUTDOWN))
	{		
		mDoFullMoveCount-- ;
		ActorEvent m_event = LF_MOVE ; 
		if (mDoFullMoveCount <=0)
		{
			m_event = MOVE ;
			mDoFullMoveCount = (int)GetMoveMultiplier() ;
		}
	
		EVENT_MANAGER.AddEvent((int)m_event, this, NEXT_FRAME,  START_OF_FRAME, NULL, (CScheduledEvent*)event);
	}
}

//******************************************************************************************
void	CActor::HandleEvent(CEvent* event) 
{
	switch( (ActorEvent)event->GetEventNum() )
	{
		case MOVE:	
		{
			PROFILE_START(MoveThings) ;
			Move() ;
			AddMoveEvent(event);

			
			PROFILE_END(MoveThings) ;
			break ;
		}
		case LF_MOVE:
		{
			PROFILE_START(MoveThings) ;
			LowFidelityMove(); 
			AddMoveEvent(event);
			PROFILE_END(MoveThings) ;
			break ;
		}

	default:
		{
			SUPERTYPE::HandleEvent(event) ;
		}
	}
}


//******************************************************************************************
float CActor::GetFractionTime()
{
	float mm = GetMoveMultiplier() ;
	if ( mm > 1.0f)
	{	
		float t = EVENT_MANAGER.GetTime() - mLastMoveTime ;
		float step = 1 / GetMoveMultiplier() ;
		float fraction =  step * t * GAME_FR;
		fraction += step * GAME.GetFrameRenderFraction(); 
		if (fraction< 0.0f) fraction = 0.0f ;
		if (fraction> 1.0f) fraction = 1.0f; 
		return fraction ;
	}
	
	return GAME.GetFrameRenderFraction() ;
}



//******************************************************************************************

FVector CActor::GetRenderPos()
{
	return mOldPos+((mPos-mOldPos) * GAME.GetFrameRenderFraction()) ;
}


//******************************************************************************************

FMatrix CActor::GetRenderOrientation()
{
	FMatrix newori;

#if TARGET == PS2
	asm __volatile__
	(
	"
		qmtc2		%9,vf31		\n
		vsub		%0,%3,%6	\n
		vsub		%1,%4,%7	\n
		vsub		%2,%5,%8	\n
		vmulax		ACC,%0,vf31	\n
		vmaddw		%0,%6,vf00	\n
		vmulax		ACC,%1,vf31	\n
		vmaddw		%1,%7,vf00	\n
		vmulax		ACC,%2,vf31	\n
		vmaddw		%2,%8,vf00	\n
	"
	:	"=&j" (newori.Row[0].lVec), "=&j" (newori.Row[1].lVec), "=&j" (newori.Row[2].lVec)
	:	"j" (mOrientation.Row[0].lVec), "j" (mOrientation.Row[1].lVec), "j" (mOrientation.Row[2].lVec),
		"j" (mOldOrientation.Row[0].lVec), "j" (mOldOrientation.Row[1].lVec), "j" (mOldOrientation.Row[2].lVec),
		"r" (GetFractionTime())
	:	"vf31"
	);
#else
	newori = mOldOrientation+((mOrientation-mOldOrientation) * GetFractionTime());
#endif
//	newori.Normalise();

	return newori;
}


//******************************************************************************************
BOOL	CActor::IsOnGround()
{ 
	return	(EVENT_MANAGER.GetTime() - mLastTimeOnGround < 0.15f);
}

//******************************************************************************************
BOOL	CActor::IsInWater()
{
	return	(EVENT_MANAGER.GetTime() - mLastTimeInWater < 0.25f);
}

//******************************************************************************************
BOOL	CActor::IsOnObject()
{
	return	(EVENT_MANAGER.GetTime() - mLastTimeOnObject < 0.15f);
}

//******************************************************************************************
void	CActor::DeclareOnGround()
{
	mLastTimeOnGround = EVENT_MANAGER.GetTime();
}

//******************************************************************************************
void	CActor::DeclareInWater()
{
	mLastTimeInWater = EVENT_MANAGER.GetTime();
}

//******************************************************************************************
void	CActor::DeclareOnObject(CThing* on_object)
{
	mLastTimeOnObject = EVENT_MANAGER.GetTime();
}

//******************************************************************************************
void	CActor::StickToGround()
{
	SUPERTYPE::StickToGround();

	mOldPos=mPos;
}