// Camera.cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"

#include "Camera.h"
#include "thing.h"
#include "BattleEngine.h"
#include "EventManager.h"
#include "Game.h"
#include "mesh.h"
#include "MeshPart.h"
#include "BSpline.h"
#include "debuglog.h"
#include "map.h"

#include <stdio.h>


//******************************************************************************************
//******************************************************************************************
float	CCamera::GetYaw()
{
	// Careful - doesn't work if there's any roll in the camera

	FVector	v = GetOrientation().Row[1];

	v.Z = 0;

	float	l = v.Magnitude();

	if (l > 0.001f)
		v /= l;

	return -atan2f(v.X, v.Y);
}

//******************************************************************************************
//******************************************************************************************

FVector CThingCamera::GetPos() 
{ 
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		FVector pos=thing->GetPos() ;
		if (!mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{
			pos.Y+=thing->GetBoundingBox()->mAxes.Y*1.5f;
			pos.Z-=thing->GetBoundingBox()->mAxes.Z;
		}
		return(pos);
	}
	
	return ZERO_FVECTOR ;
}

//******************************************************************************************
FMatrix CThingCamera::GetOrientation() 
{	 
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		return thing->GetOrientation() ;
	}
	
	return ID_FMATRIX ;
}

//******************************************************************************************
FVector CThingCamera::GetOldPos()
{
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		FVector pos=thing->GetOldPos() ;
		if (!mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{
			pos.Y+=thing->GetBoundingBox()->mAxes.Y;
			pos.Z-=thing->GetBoundingBox()->mAxes.Z;
		}
		return(pos);
	}
	
	return ZERO_FVECTOR ;
}

//******************************************************************************************
FMatrix CThingCamera::GetOldOrientation()
{
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		return thing->GetOldOrientation() ;
	}
	
	return ID_FMATRIX ;
}

//******************************************************************************************
const float CThingCamera::GetZoom() 
{	 
	if (mForThing.ToRead())
	{
		if (mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{
			return ((CBattleEngine*)mForThing.ToRead())->GetZoom() ;
		}
	}
	
	return 1.0f ;
}

//******************************************************************************************
const float CThingCamera::GetOldZoom() 
{	 
	if (mForThing.ToRead())
	{
		if (mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{
			return ((CBattleEngine*)mForThing.ToRead())->GetOldZoom() ;
		}
	}
	
	return 1.0f ;
}


//******************************************************************************************

bool CThingCamera::GetShowHUD() 
{
	if (mForThing.ToRead())
	{
		if (mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{	
			return true;
		}
	}
	
	return false;
}

//******************************************************************************************
//******************************************************************************************
//******************************************************************************************


//******************************************************************************************
//******************************************************************************************



//******************************************************************************************
CThing3rdPersonCamera::CThing3rdPersonCamera(CThing* for_thing) : mForThing(for_thing)
{
	SPtrSet<FVector>* points = new (MEMTYPE_PLAYER)	SPtrSet<FVector> ;

	float r = for_thing->GetRadius() ;

	float scale = 2.5f;

	points->Append(new(MEMTYPE_CAMERA) FVector(0.0f, 9.0f*scale, (-r -3.8f)*scale)) ;
	points->Append(new(MEMTYPE_CAMERA) FVector(0.0f, -9.0f*scale, (-r - 1.0f)*scale));
	points->Append(new(MEMTYPE_CAMERA) FVector(0.0f, 1.0f*scale,  1.8f*scale)) ;
	

	mCurve = new(MEMTYPE_CAMERA) CBSpline(points) ;

}


//******************************************************************************************
CThing3rdPersonCamera::~CThing3rdPersonCamera()
{
	SAFE_DELETE(mCurve) ;
}




//******************************************************************************************
FVector CThing3rdPersonCamera::GetPos() 
{ 
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		FVector pos=thing->GetPos() ;

		FMatrix pointing = thing->GetOrientation();
		CEulerAngles a(pointing) ;

	//	LOG.AddMessage("pitch = %2.4f", a.mPitch *2.0f) ;

		float fraction = 1.0f - (((a.mPitch*2.1f) + PI ) / (2.0f * PI)) ;
	
	//	LOG.AddMessage("faction = %2.4f", fraction);

		FVector pos1 = mCurve->GetPoint(fraction) ;
		pos1 = FMatrix(a.mYaw,0.0f,0.0f) * pos1 ;

		pos+=pos1 ;

		float z = MAP.Collide(pos)  ;

		if (pos.Z > z) pos.Z = z ;
		

		return(pos);
	}
	
	return ZERO_FVECTOR ;
}

//******************************************************************************************
FVector CThing3rdPersonCamera::GetOldPos()
{
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		FVector pos=thing->GetOldPos() ;

		FMatrix pointing = thing->GetOldOrientation();
		CEulerAngles a(pointing) ;

		float fraction = 1.0f - (((a.mPitch *2.1f) + PI ) / (2.0f * PI)) ;

		FVector pos1 = mCurve->GetPoint(fraction) ;
		pos1 = FMatrix(a.mYaw,0.0f,0.0f) * pos1 ;

		pos+=pos1 ;

		float z = MAP.Collide(pos)  ;

		if (pos.Z > z) pos.Z = z ;

		return(pos);
	}
	
	return ZERO_FVECTOR ;
}


//******************************************************************************************
FMatrix CThing3rdPersonCamera::GetOrientation() 
{	 

	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		CEulerAngles angles = (thing->GetOrientation()) ; 

		FVector pos = mForThing->GetPos() ;
		pos += FMatrix(angles.mYaw,0.0f,0.0f) * FVector(0.0f,-4.0f,0.0f) ;

		FVector look_at = thing->GetOrientation() * FVector(0.0f,10.0f,0.0f) ;

		FVector point = pos + look_at ;

		FVector camera_to_point = point - GetPos() ;
	
		return FMatrix(angles.mYaw,camera_to_point.Elevation(),0.0f);
	}
	
	return ID_FMATRIX ;
}

//******************************************************************************************
FMatrix CThing3rdPersonCamera::GetOldOrientation()
{
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		CEulerAngles angles = (thing->GetOldOrientation()) ; 

		FVector pos = mForThing->GetOldPos() ;
		pos += FMatrix(angles.mYaw,0.0f,0.0f) * FVector(0.0f,-4.0f,0.0f) ;

		FVector look_at = thing->GetOldOrientation() * FVector(0.0f,10.0f,0.0f) ;

		FVector point = pos + look_at ;

		FVector camera_to_point = point - GetOldPos() ;
	
		return FMatrix(angles.mYaw,camera_to_point.Elevation(),0.0f);
	}
	
	return ID_FMATRIX ;
}

//******************************************************************************************
const float CThing3rdPersonCamera::GetZoom() 
{	 
	if (mForThing.ToRead())
	{
		if (mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{
			return ((CBattleEngine*)mForThing.ToRead())->GetZoom() ;
		}
	}
	
	return 1.0f ;
}

//******************************************************************************************
const float CThing3rdPersonCamera::GetOldZoom() 
{	 
	if (mForThing.ToRead())
	{
		if (mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{
			return ((CBattleEngine*)mForThing.ToRead())->GetOldZoom() ;
		}
	}
	
	return 1.0f ;
}


//******************************************************************************************

bool CThing3rdPersonCamera::GetShowHUD() 
{
	if (mForThing.ToRead())
	{
		if (mForThing->IsA(THING_TYPE_BATTLE_ENGINE))
		{	
			return true;
		}
	}
	
	return false;
}



//******************************************************************************************
//******************************************************************************************
//******************************************************************************************


//******************************************************************************************
CPanCamera::CPanCamera(CThing* for_thing, CBSpline* curve, float length) :
 mForThing(for_thing),
 mCurve(curve),
 mStartTime(EVENT_MANAGER.GetTime()),
 mLength(length)
{
	mPos = FVector(0.0f, 0.0f,0.0f) ;
	mOrientation = ID_FMATRIX ;
	Update();

	mOldPos = mPos ;
	mOldOrientation = mOrientation;
}

//******************************************************************************************
CPanCamera::~CPanCamera()
{
	delete mCurve ;
	mCurve = NULL ;
}

//******************************************************************************************
void	CPanCamera::Update()
{
	mOldPos = mPos ;
	mOldOrientation = mOrientation ;

	float val = ((EVENT_MANAGER.GetTime() - mStartTime)/mLength) ;
	if (val > 1.0f)
	{	
		return ;
	}

	FVector pos = mCurve->GetPoint(val) ;
	CThing* thing = mForThing.ToRead() ;
	if (thing)
	{
		mPos = thing->GetPos() + pos;

		FVector us_to_them = thing->GetPos() - mPos ;
		if (us_to_them.Magnitude() != 0.0f)
		{
			mOrientation = FMatrix(us_to_them.Azimuth(), us_to_them.Elevation(), 0.0f) ;
		}
	}


	// ensure this updates after the thing has moved
	EVENT_MANAGER.AddEvent(UPDATE_CAMERA, this, NEXT_FRAME, END_OF_FRAME);
}


//******************************************************************************************
FVector CPanCamera::GetPos() 
{ 
	return mPos ;
}

//******************************************************************************************
FMatrix CPanCamera::GetOrientation() 
{	 
	return mOrientation;
}

//******************************************************************************************
FVector CPanCamera::GetOldPos()
{
	return mOldPos ;
}

//******************************************************************************************
FMatrix CPanCamera::GetOldOrientation()
{
	return mOldOrientation ;
}


//******************************************************************************************
void CPanCamera::HandleEvent(CEvent* event)
{
	switch ((ECameraEvent)event->GetEventNum() )
	{
		case UPDATE_CAMERA:
		{
			Update() ;
			break ;
		}
		default:
		{
			CMonitor::HandleEvent(event) ;
			break;
		}
	}
}


//******************************************************************************************
//******************************************************************************************
//******************************************************************************************



//******************************************************************************************
CViewPointCamera::CViewPointCamera(const FVector& point,
								   const float& rotate_speed,
								   const float& start_distance,
								   const float& end_distance,
								   const float& time_between_distance)
{
	mLookAtPoint = point ;
	mRotateYawSpeed =rotate_speed;
	mStartDistance =start_distance;
	mEndDistance = end_distance;
	mTimeBetweenDistances = time_between_distance;
	mStartTime = EVENT_MANAGER.GetTime() ;

	float mLastCalcPosTime = -2.0f ;
	float mLastCalcOriTime = -2.0f ;
	
	mLastCalcPos = point;
	mLastCalcOri=ID_FMATRIX;

	mOldPos = point;
	mOldOrientation=ID_FMATRIX;
}


//******************************************************************************************
FVector CViewPointCamera::GetPos()
{
	if (mLastCalcPosTime == EVENT_MANAGER.GetTime())
	{
		return mLastCalcPos ;
	}

	mOldPos=mLastCalcPos;

	float yaw = ((EVENT_MANAGER.GetTime() - mStartTime) * mRotateYawSpeed);
	FMatrix m(yaw,0.0f,0.0f) ;
	
	float dist ;
	if (EVENT_MANAGER.GetTime() >= mStartTime+mTimeBetweenDistances)
	{
		dist = mEndDistance ;
	}
	else
	{
		float d = (mEndDistance - mStartDistance) / (mTimeBetweenDistances ) ;
		float t = EVENT_MANAGER.GetTime() - mStartTime ;
		dist = d * t ;
	}

	FVector s1(dist,0.0, -dist) ;
	FVector s2 = m * s1 ;

	mLastCalcPosTime = EVENT_MANAGER.GetTime() ;
	mLastCalcPos = mLookAtPoint + s2 ;
	return mLastCalcPos ;
}

//******************************************************************************************
FMatrix CViewPointCamera::GetOrientation()
{
	if (mLastCalcOriTime == EVENT_MANAGER.GetTime())
	{
		return mLastCalcOri ;
	}
	mOldOrientation=mLastCalcOri;
	float yaw = ((EVENT_MANAGER.GetTime() - mStartTime) * mRotateYawSpeed);
	FMatrix m(yaw,0.0f,0.0f) ;
	FVector s = m*  FVector(-1.0f,0.0f,1.0f) ;
	s.Normalise();
	FMatrix ori(s.Azimuth(), s.Elevation(), 0.0f) ;
	mLastCalcOriTime = EVENT_MANAGER.GetTime() ;
	mLastCalcOri = ori ;
	return ori;
}

//******************************************************************************************

FMatrix CViewPointCamera::GetOldOrientation()
{
	return mOldOrientation;
}

//******************************************************************************************

FVector CViewPointCamera::GetOldPos()
{
	return mOldPos;
}

//******************************************************************************************
const float CViewPointCamera::GetZoom()
{
	return 1.0f ;
}

//******************************************************************************************
bool CViewPointCamera::GetShowHUD() 
{	 
	return FALSE;
}

//******************************************************************************************
// Camera for cutscenes - uses camera from object (if it can find it)
//******************************************************************************************

CMovieCamera::CMovieCamera(CThing* for_thing) : mForThing(for_thing)
{
	mOldPos=FVector(0,0,0);
	mOldOrientation=ID_FMATRIX;
	mLastCalcPos=FVector(0,0,0);
	mLastCalcOri=ID_FMATRIX;
	mLastCalcPosTime=-2.0f;
	mLastCalcOriTime=-2.0f;
	mLastCalcZoom=1.0f;
	mLastCalcZoomTime=-2.0f;
	mOldZoom=1.0f;
}

//******************************************************************************************
FVector CMovieCamera::GetPos() 
{ 
	if (mLastCalcPosTime == EVENT_MANAGER.GetTime())
	{
		return mLastCalcPos ;
	}
	
	mOldPos=mLastCalcPos;

	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		CRenderThing *rt=thing->GetRenderThing();		
		if (rt!=NULL)
		{
			FVector pos;
			FMatrix ori;
			float fov;
			rt->GetMovieCameraPosition(&ori,&pos,&fov,thing);
			mLastCalcPos=pos;
			mLastCalcPosTime=EVENT_MANAGER.GetTime();
			return(pos);
		}
		TRACE("Warning - movie camera using default position!\n");
		return(thing->GetPos());
	}
	
	TRACE("Warning - movie camera has no thing!\n");
	return ZERO_FVECTOR;
}

//******************************************************************************************
FMatrix CMovieCamera::GetOrientation() 
{	 
	if (mLastCalcOriTime == EVENT_MANAGER.GetTime())
	{
		return mLastCalcOri;
	}
	mOldOrientation=mLastCalcOri;

	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		CRenderThing *rt=thing->GetRenderThing();		
		if (rt!=NULL)
		{
			FVector pos;
			FMatrix ori;
			float fov;
			rt->GetMovieCameraPosition(&ori,&pos,&fov,thing);

			mLastCalcOri=ori;
			mLastCalcOriTime=EVENT_MANAGER.GetTime();			

			return(ori);
		}
		return(thing->GetOrientation());
	}
	
	return ID_FMATRIX ;
}

//******************************************************************************************
const float CMovieCamera::GetZoom() 
{	 
	if (mLastCalcZoomTime == EVENT_MANAGER.GetTime())
	{
		return mLastCalcZoom;
	}
	mOldZoom=mLastCalcZoom;	

	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		CRenderThing *rt=thing->GetRenderThing();		
		if (rt!=NULL)
		{
			FVector pos;
			FMatrix ori;
			float fov;
			rt->GetMovieCameraPosition(&ori,&pos,&fov,thing);

			// Default FOV is 90, so get our desired FOV as a ratio from that

			mLastCalcZoom=(fov/90)/2.0f;
			mLastCalcZoomTime=EVENT_MANAGER.GetTime();
			
			return((fov/90)/2.0f);
		}

		mLastCalcZoom=1.0f;
		mLastCalcZoomTime=EVENT_MANAGER.GetTime();

		return(1.0f);
	}

	mLastCalcZoom=1.0f;
	mLastCalcZoomTime=EVENT_MANAGER.GetTime();	

	return(1.0f);
}

//******************************************************************************************
const float CMovieCamera::GetOldZoom() 
{	 
	return(mOldZoom);
}

//******************************************************************************************
bool CMovieCamera::GetShowHUD() 
{	 
	return false;
}

//******************************************************************************************
int CMovieCamera::GetCameraNumber()
{
	CThing* thing = mForThing.ToRead() ;
	if (thing) 
	{
		CRenderThing *rt=thing->GetRenderThing();		
		if (rt!=NULL)
		{
			return(rt->GetMovieCameraNumber(thing));
		}
		return(0);
	}
	
	return(0);
}

//******************************************************************************************

FMatrix CMovieCamera::GetOldOrientation()
{
	return mOldOrientation;
}

//******************************************************************************************

FVector CMovieCamera::GetOldPos()
{
	return mOldPos;
}

//******************************************************************************************
// controllable camera used in debug and that
//******************************************************************************************

SINT	CControllableCamera::mFrameCount = 0;

CControllableCamera::CControllableCamera(FVector pos, FMatrix orientation) :
  mOrientation(orientation),
  mPos(pos)
{
	  mOldPos = mPos;
	  mOldOrientation = mOrientation;

	  mTempPos = mPos;
	  mTempOrientation = mOrientation;

	  mLastFrameCount = mFrameCount;
}

void	CControllableCamera::PrepareForInterpolation()
{
	if (mLastFrameCount != mFrameCount)
	{
		// update
		mLastFrameCount = mFrameCount;

		mOldPos = mPos;
		mOldOrientation = mOrientation;

		mPos = mTempPos;
		mOrientation = mTempOrientation;
	}
}

void CControllableCamera::ReceiveButtonAction(CController* from_controller, int button, float val)
{
	switch (button)
	{
		case BUTTON_CAMERA_PITCH_UP:
		case BUTTON_CAMERA_PITCH_DOWN:
		{
			FMatrix p = FMatrix(0.0,-val/11.0f,0.0) ;
			mTempOrientation = mTempOrientation * p ;
			break ;
		}

		case BUTTON_CAMERA_YAW_LEFT:
		case BUTTON_CAMERA_YAW_RIGHT:
		{
			FMatrix p = FMatrix(-val/11.0f,0.0,0.0) ;
			mTempOrientation = p *mTempOrientation ;
			break ;
		}

		case BUTTON_CAMERA_MOVE_FORAWRD:
		{
			FVector s(0.0f,-val/*/3.5f*/,0.0f) ;
			FVector s1 = mTempOrientation * s;
			mTempPos +=s1 ;
			break ;
		}

		case BUTTON_CAMERA_MOVE_BACKWARDS:
		{
			FVector s(0.0f,-val/*/3.5f*/,0.0f) ;
			FVector s1 = mTempOrientation * s;
			mTempPos +=s1 ;
			break ;
		}

		case BUTTON_CAMERA_STRAFE_LEFT:
		{
			FVector s(val/*/3.5f*/,0.0f,0.0f) ;
			FVector s1 = mTempOrientation * s;
			mTempPos +=s1 ;
			break ;
		}

		case BUTTON_CAMERA_STRAFE_RIGHT:
		{
			FVector s(val/*/3.5f*/,0.0f,0.0f) ;
			FVector s1 = mTempOrientation * s;
			mTempPos +=s1 ;
			break ;
		}


		// In the freecam it's a toggle even though thanks to the xbox's specs it's not meant to be (1-7-3-16)
		case  BUTTON_PAUSE:
		{
			if (GAME.IsPaused())
			{
				GAME.UnPause() ;
			}
			else
			{
				GAME.Pause() ;
			}
			break ;
		}
	}
}


//*****************************************************************************************
//*****************************************************************************************
//** Interpolated camera

CInterpolatedCamera::CInterpolatedCamera(CCamera *cam)
{
	cam->PrepareForInterpolation();

	FVector pos    = cam->GetPos();
	FVector oldpos = cam->GetOldPos();
	mPos    =  oldpos + ((pos - oldpos) * GAME.GetFrameRenderFraction2());

	FMatrix ori    = cam->GetOrientation();
	FMatrix oldori = cam->GetOldOrientation();

	//er??
	mOrientation = oldori + ((ori - oldori) * GAME.GetFrameRenderFraction2());
	mOrientation.Normalise();

	float zoom,oldzoom;

	zoom=cam->GetZoom();
	oldzoom=cam->GetOldZoom();

	mZoom = oldzoom+((zoom-oldzoom) * GAME.GetFrameRenderFraction2());
}

void CInterpolatedCamera::Mirror(float aroundz)
{
	// Note that when rendering with this you need to swap culling modes,
	// and gamut creation won't work!
	mOrientation.Row[2].X=-mOrientation.Row[2].X; 
	mOrientation.Row[2].Y=-mOrientation.Row[2].Y;
	mOrientation.Row[2].Z=-mOrientation.Row[2].Z;
	mPos.Z=aroundz-(mPos.Z-aroundz);
}

const float CCamera::GetAspectRatio()
{
	if (GAME.IsMultiplayer())
		return 0.5f;
	else
		return 0.75f;
};