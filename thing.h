#ifndef THING_H
#define THING_H

#include	"oids.h"
#include	"PtrSet.h"
#include	"renderablething.h"
//#include	"rtmesh.h"
#include	"mapwho.h"
#include	"mapwhoentry.h"
#include	"monitor.h"
#include	"PCRenderData.h"
#include	"ThingType.h"
#include	"Generalvolume.h"
#include	"ActiveReader.h"
#include	"SoundMaterial.h"
#include	"AudibleThing.h"
#include	"ResourceAccumulator.h"

#include	"MissionScript/iscript.h"

class CCollisionSeekingThing ;
class CCSPersistentThing ;
class CMotionController;
class CCollisionReport; 

#define BATTLE_ENGINE_COFGHEIGHT 1.9f

#define GAME_FR 20.0f
#define CLOCK_TICK 0.05f 

#include	"InitThing.h"		// separated off for editor purposes

enum EThingEvent
{
	SHUTDOWN = 2000,
	INIT_SCRIPT,
	START_DIE_PROCESS,
	READY_SCRIPT
};

enum THING_FLAGS
{
	TF_DECLARED_SHUTDOWN = 1,
	TF_IN_MAP_WHO = 2,
	TF_DYING = 4,
	TF_DONT_RENDER = 8,			// can't see but can touch it
	TF_INVISIBLE = 16,				// can't see it can't touch it
	TF_MARKED_OBJECTIVE = 32,
	TF_IS_BIG_THING = 64,			// used for gamut boys for speed up
	TF_SLIDE	= 128,
	TF_REMOVED_UNIT_TYPE = 256
};

enum EAIState
{
	AI_ON=0,
	AI_OFF,
	AI_NORMAL,
	AI_DEFENSIVE,
	AI_ONF,
};

class CThing ;
//******************************************************************************************
DECLARE_MULTI_INTERFACE_CLASS(CThing, IAudibleThing, IRenderableThing)
public:
							CThing();
	virtual					~CThing();

	virtual void			Init(CInitThing* init)	;	
	virtual	FVector			GetRenderPos()					{return mPos;};
	virtual	FMatrix			GetRenderOrientation()			{return ID_FMATRIX;};
	virtual	BOOL			GetRenderSelected()				{return FALSE;};
	virtual	SINT			GetRenderColourOffset()			{return 0;};
	virtual	DWORD			GetRenderColour()				{return 0xff000080 ;}
	virtual float			GetRenderTurn()					{return 1;};
	virtual float			GetRenderFrame()				{return 0;};
	virtual SINT			GetRealAnimIndex()				{ return -1;};
	virtual	float			GetRenderRadius()				{ if (mRenderThing) return mRenderThing->GetRTRadius(); else return 1.0f;} // {return 1;};
	virtual	FVector			GetRenderStartPos()				{return mPos;};
	virtual	FVector			GetRenderEndPos()				{return mPos;};
	virtual	SINT			GetRenderMesh()					{return -1;};
//	virtual SINT			GetRenderThingMiscFlags() 		{return (mWhite ? RTMF_WHITE : 0) + (mBlack ? RTMF_BLACK : 0);}
	virtual	BOOL			GetRenderRotateShadow()			{return FALSE;};
	virtual BOOL			GetRenderCanGoFlatShaded()		{return FALSE;};
	virtual	BOOL			RenderUseHierarchy(SINT pn)		{return TRUE;};
	virtual int				GetShouldDoHitEffect(SINT mesh_part_no) { return 0; } 
	virtual	float			GetRenderYaw()					{return 0;};
	virtual float			GetRenderScale()				{return 1;};
	virtual BOOL			GetRenderWithTransforms()		{return TRUE;};
	virtual	SINT			GetRenderImposterNo()			{return 0;};
	virtual float			GetRenderImposterBias()			{return 1.0f;};
	virtual int				GetSnowDensity()				{return 0;};
	virtual BOOL			GetCanBeStaticallyShadowed()	{return(FALSE);};
	virtual class CStaticShadow *GetStaticShadow()			{ASSERT(0); return(NULL); };
	virtual void			SetStaticShadow(class CStaticShadow *shadow) {ASSERT(0);};	
	virtual	BOOL			GetRequiresPolyBucket()			{return TRUE;}

	virtual	FVector			GetSoundPos()					{return mPos;};
	virtual	FMatrix			GetSoundOrientation()			{return ID_FMATRIX;};
	virtual FVector			GetSoundVelocity()				{return FVector(0,0,0);};

	virtual float			GetCueFactor()					{ if (mRenderThing) return mRenderThing->GetCueFactor(); else return 0.0001f;} // very small!

			void			DrawDebugCuboid() ;

	virtual void			SampleFinishedPlaying(CSoundEvent* event) {}

	virtual	void			Shutdown() ;
	virtual void			HandleEvent(CEvent *event) ;
	virtual void			AddShutdownEvent() ;

			FVector&		GetPos()						{return mPos;};
	inline	FMatrix& 		GetOrientation(); // This is defined below
		
			void			SetPos(FVector &inPos)			{ mPos=inPos; }

	virtual float			GetMaxVelocity()	{ return 0.0f;}		// Max Distance traveled in one second (cannot possibly exceed)

	virtual float			GetRadius()       { if (mRenderThing) return mRenderThing->GetRTRadius(); else return 0.0f;}
	virtual float			GetBoundingRadius() ;
	virtual CBoundingBox*	GetBoundingBox();
			void			GetCentrePos(FVector &outPos) ;

	virtual void			StickToGround();
			void			SitOnGround();
			
			void			UpdatePosition();

			float			GetRenderThingFrameIncrement(EAnimMode am, int* realindex);
	virtual void			MoveTo(const FVector& pos) ;
	virtual void			Teleport(const FVector& pos);
	virtual void			TeleportOrientation(const FMatrix& orientation) {}
	virtual void			Activate() {}
	virtual void			Deactivate() {}
	virtual float			GetMoveMultiplier() { return 1.0f ; }
			void			SetObjective(BOOL val) ;
	virtual FVector			GetLocalLastFrameMovement() { return FVector(0.0f,0.0f,0.0f) ; }
	virtual BOOL			IsObjective() { return FALSE ;}

	// JCL - overloads for actor...
	virtual	FVector			GetVelocity() { return ZERO_FVECTOR;};
	virtual	void			SetVelocity(const FVector& vel) {};
	virtual	void			AddVelocity(const FVector& vel) {};
	virtual	FVector			GetOldPos() {return mPos;};
	virtual FMatrix			GetOldOrientation() {return ID_FMATRIX;};

			FVector			GetTopPos();			// SLOW - don't call all the time!!

			CMapWhoEntry*   GetMapWhoEntry()				{if (mMapWhoEntry.GetSectorIn().GetLayer() == -1) return NULL; else return &mMapWhoEntry;}
			void			SetRenderThing(CRenderThing* rtthing)			{ mRenderThing = rtthing ; }
			CRenderThing*	GetRenderThing() { return mRenderThing ; }
		    CCollisionSeekingThing* GetCST() { return mCollisionSeekingThing ; }
			CCSPersistentThing* GetCSPT() ;

/*virtual*/ BOOL			GetIsInvisible() { return (mFlags & TF_INVISIBLE) != 0 ; }
	virtual void			MakeVisible()   {	mFlags &= ~TF_INVISIBLE ; }
	virtual void			MakeInvisible() { mFlags |= TF_INVISIBLE ; }
	virtual	void			InitRenderThing(CInitThing *init);
	virtual void			InitCollisionSeekingThing(CInitCSThing *init);
	virtual void			Render(DWORD flags=0);
	virtual	void			RenderImposter();
	virtual bool			GetCanBeImpostered() { return true; }
	virtual int				GetImposterFrames() { return 1; };
	virtual EAnimMode		GetImposterAnimMode() { return AM_NONE; };

	virtual CThing* GetThing() { return this ; }


			CMESH			*GetRTMesh() {return (mRenderThing) ? mRenderThing->GetRTMesh() : NULL;};

	virtual	void			SetThingType(ULONG t) { mThingType = (t | THING_TYPE_THING); }
			ULONG&			GetThingType() { return mThingType ; }
	const	BOOL			IsA(EThingType type) const { return (type & mThingType); }
	virtual void			Hit(CThing* other_thing, CCollisionReport* report) {}
	virtual void			Damage(float amount,CThing *inByThis,BOOL inDamageShields=TRUE, int mesh_part_no = -1) {} 
	virtual	char*			GetName() { return NULL ; }
	virtual void			SetName(char *inName)					{ }
	// Sound Stuff
	virtual SINT			GetSoundMaterial()						{ return SM_DEFAULT; }

   //
  // At some point in the somefuture these should probably be made into
 // variables in Actor and accessed through non virtual access functions
//
	virtual BOOL			ClipToGround()			{ return TRUE;}
	virtual float			Gravity()				{ return 0.01f;}
	virtual BOOL			ObeyGravity()			{ return TRUE; }
	virtual	float			BounceFactor()			{ return 0;}
	virtual	float			COfGHeight()			{ return 0;}

	virtual BOOL			CanGoUnderWater()		{ return FALSE; }
	
			short			GetFlags() { return mFlags ;}
			void			SetFlags(short val) { mFlags =val;}
			BOOL			IsDying()			{ if (mFlags & TF_DYING) return TRUE; else return FALSE ; }
			BOOL			IsShuttingDown()	{ if (mFlags & TF_DECLARED_SHUTDOWN) return TRUE; else return FALSE; }

	virtual BOOL			StartDieProcess() ;

	virtual CMotionController	*GetMotionController() { return NULL; }

	virtual void			DrawDebugStuff(float &y_offset) ;
	virtual void			DrawDebugStuff3d();

	virtual EAnimMode		GetRenderAnimation()				{ return AM_NONE; }


	virtual EAIState		GetAIState()						{ return AI_ON; }
	virtual void			SetAIState(EAIState inAIState)		{}

			void			Reset(SINT c);
			void			Select();

#ifdef RESBUILDER
	virtual void			AccumulateResources(CResourceAccumulator * accumulator,DWORD flags=0);
#endif
	virtual void			AccumulateScore()				{ }

	static	void			ResetThingCounter()				{ sThingCount=0; }
			int				GetThingNumber()				{ return mThingNumber; }

	virtual void			SetVulnerable(BOOL val)			{ }
	virtual BOOL			GetVulnerable()					{ return TRUE;}

	virtual void			IncreaseThingCounter(
								CInitThing		*inInit);

			BOOL			IsOverWater();

/*	// Things have to calculate themselves from MapWhoEntry's because of header file inclusion order type stuff.
	static CThing *			FromMapWhoEntry(CMapWhoEntry *m)
	{
		return (CThing *)(((char *)m) - offsetof(CThing, mMapWhoEntry));
	}*/

protected:

	friend class CMapWhoEntry;

	// SRG DON'T CHANGE OR ADD ATTRIBUTES HERE UNLESS MY SAY SO !!!
	CMapWhoEntry			mMapWhoEntry ;
	FVector					mPos;
	short					mFlags;
	short					mThingNumber;
	CRenderThing*			mRenderThing;
	ULONG					mThingType;
	CCollisionSeekingThing* mCollisionSeekingThing ;

	static short			sThingCount;
	static FMatrix			sIdMatrix;
};




DECLARE_THING_CLASS(CComplexThing, CThing)

public:

	CComplexThing();
	~CComplexThing();
	virtual void			Init(CInitThing* init)	;	
	virtual	void			Shutdown() ;
	virtual void			HandleEvent(CEvent* event);


	virtual	FMatrix			GetRenderOrientation()			{return mOrientation;};
	virtual float			GetRenderFrame();
	virtual	void			SetThingType(ULONG t)		 { SUPERTYPE::SetThingType(t | THING_TYPE_COMPLEX_THING); }
	virtual SINT			GetRealAnimIndex()				{ if (mAnimation) return mAnimation->GetAnimIndex(); else return -1;};
	virtual	FMatrix			GetSoundOrientation()			{return mOrientation;};
			FMatrix&		GetOrientation()				{return mOrientation;}
	virtual BOOL			IsObjective() { if ((mFlags & TF_MARKED_OBJECTIVE) == 0) return FALSE ; else return TRUE ;}
	virtual FMatrix			GetOldOrientation() {return mOrientation;};
	virtual CMotionController	*GetMotionController() { return mMotionController; }
	virtual EAnimMode		GetRenderAnimation()				{ if (mAnimation) return mAnimation->GetAnimMode() ; else return AM_NONE; }
	virtual	char*			GetName() { return mName ;}
	virtual void			SetName(char *inName);
			IScript*		GetMissionScript() { return mMissionScript ; }
	virtual void			AddShutdownEvent() ;
	virtual BOOL			StartDieProcess() ;
	virtual BOOL			FinishedPlayingCurrentAnimation();
	virtual	BOOL			SetAnimMode(EAnimMode am, BOOL reset_frame, BOOL force_looped = FALSE);
			BOOL			SetAnimMode(char* name, BOOL reset_frame, BOOL force_looped = FALSE);
			EAnimMode		GetAnimModeByName(char* name); 
	virtual void			TeleportOrientation(const FMatrix& orientation);
			void			SetScript(char *inScript);
	virtual void			Hit(CThing* other_thing, CCollisionReport* report) ; 
#ifdef RESBUILDER
	virtual void			AccumulateResources( CResourceAccumulator * accumulator,DWORD flags=0 );
#endif
	virtual void			GoToPoint(FVector  point, BOOL		inOverride=FALSE)				{}
	virtual	void			SetVar(CStringDataType* name, CDataType* data) ;
	virtual void			SetSpawnScript(char *inScript)		{};
	virtual void			Stop()								{}
	virtual SPtrSet<CActiveReader<CThing> >* GetContainedInside() { return NULL ; }

protected:

	FMatrix					mOrientation;
	CAnimation*				mAnimation;
	CMotionController*		mMotionController;
	IScript					*mMissionScript;
	char					*mName;
};


FMatrix &CThing::GetOrientation()				
{
	if (IsA(THING_TYPE_COMPLEX_THING))
		return(((CComplexThing *) this)->GetOrientation());
	else
		return(sIdMatrix);
}


#endif
