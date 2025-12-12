#ifndef	ENGINE_H
#define	ENGINE_H

//#define SLOW_BUT_FLASHY

class CCamera;
class CWaterReflection;

#include "membuffer.h"
#include "Lights.h"

//=============================================-===--==---=---- ---  --   -
#define DEFAULT_CAMERA_OFFSET	0.0f
#define	DEFAULT_Z_FAR			256.0f
#define DEFAULT_Z_NEAR			0.1f
#define VIEWPOINTS				2
//=============================================-===--==---=---- ---  --   -
#define TREE_SHADOW_SIZE		6
#define TREE_SHADOW_XOFF		(-0.4f)
#define TREE_SHADOW_YOFF		(0.1f)
//=============================================-===--==---=---- ---  --   -
#define USE_BIGLIST_FOR			3
        //
       // NOTE:
      //
     // That 'top' and 'bottom' in the engine source refer to coordinates
    // rather than the relative on screen positions, so 'top' neans 'toward 0'
   // and 'bottom' means 'toward lots', left and right similarly, but that's
  // fucking obvious.
 //
//=============================================-===--==---=---- ---  --   -
//#define	N_DETAIL_LEVELS	5
#define N_MAPMIPS		7
#define	MAX_GLOBAL_MESHES 256
    //=============================================-===--==---=---- ---  --   -
   //
  // Engine
 // 
//=============================================-===--==---=---- ---  --   -

#include "Texture.h"
#include "Mesh.h"
#include "MemBuffer.h"
#include "landscape.h"
#include "kempycube.h"
#include "water.h"

#define DRAW_MAP_WHO 1 
#define DRAW_PROFILE 2
#define DRAW_OBJECTS_AS_CUBOIDS 4
#define DRAW_COCKPIT 8
#define DRAW_SKELETAL_ACCURATELY 16 // Note : Forces slow (but accurate) rendering of skeletal meshes
#define DRAW_OUTER_RADIUS 32
#define DRAW_MEM_MANAGER 64

// Viewport structure

class CViewport
{
public:
	int	Width,Height;
	int	X,Y;
	float MinZ,MaxZ;
};

class CEngine
{
protected:
				CEngine();
	class		CCamera				*mCamera[VIEWPOINTS];
	int			mViewpoints;	
	CLANDSCAPE			*mLandscape;
	class		CWATER*				mWater;
	class		CLightManager*		mLights;
	class		CSKY*				mSky;
	class		CTEXTURE*			mHitEffectTexture;
	class		CTEXTURE*			mCloakTexture;
	
	virtual	void	Shutdown();
	virtual BOOL	Init();
	virtual void	InitResources();

	void ShutdownRestartLoop() {}
	

	CMESH		*mDefaultMesh;
	CMESH		*mGlobalMeshes[MAX_GLOBAL_MESHES];
	SINT		mNumGlobalMeshes;

	float		mNearZ;
	float		mFarZ;

	CViewport			mViewport[VIEWPOINTS];
	class CPlayer		*mPlayer[VIEWPOINTS];

	class C3DGamut		*mGamut;	
	
	CViewport			mCurrentViewport;

	int					mHitEffectFactorR;
	int					mHitEffectFactorG;
	int					mHitEffectFactorB;	

	class CKEMPYCUBE	*mKempyCube;	
	
	class	CMapTex		*mMapTexs;	

public:
	class		CTEXTURE			*mHilightTexture;
	
	void		LoadAllNamedMeshes( CMEMBUFFER &dataFile );
	void		ResetNamedMeshes();

	SINT		AddNewGlobalNamedMesh(char *name);

	CMESH		*GetDefaultMesh()		{mDefaultMesh->AddRef(); return mDefaultMesh;};
	CMESH		*GetGlobalMesh(SINT num){mGlobalMeshes[num]->AddRef(); return mGlobalMeshes[num];};

	CCamera		*GetCamera()			{ /*ASSERT(mCamera[mCurrentViewpoint]);*/ return mCamera[mCurrentViewpoint];	}
	void		SetCamera(CCamera *c)	{ mCamera[mCurrentViewpoint] = c;}  // JCL - sorry!
		
	void				ToggleDebugDraw(ULONG num) { if (mDrawDebugStuff & num) mDrawDebugStuff &=(~num) ; else 
										mDrawDebugStuff |= num ;}

	class CTEXTURE	*GetHitEffectTexture() { return(mHitEffectTexture); };
	class CTEXTURE	*GetCloakTexture() { return(mCloakTexture); };

	void		ToggleParticles() {mDebugNoParticles = !mDebugNoParticles;};
	BOOL		DebugNoParticles() {return mDebugNoParticles;};

	ULONG		GetDrawDebugStuff() { return mDrawDebugStuff; }	
	
	void		GetViewMatrixFromCamera(CCamera *cam, FMatrix *matview);
	
	float		GetNearZ() { return mNearZ; }
	float		GetFarZ() { return mFarZ; }
	void		SetNearZ( const float aNearZ ) { mNearZ = aNearZ; }
	void		SetFarZ( const float aFarZ ) { mFarZ = aFarZ; }

	void				SetCurrentViewport(CViewport *vp) { mCurrentViewport=*vp; };
	CViewport			GetCurrentViewport()		{ return mCurrentViewport; };
	int					GetCurrentViewportWidth()	{ return mCurrentViewport.Width; };
	int					GetCurrentViewportHeight()	{ return mCurrentViewport.Height; };
	int					GetCurrentViewportX()		{ return mCurrentViewport.X; };
	int					GetCurrentViewportY()		{ return mCurrentViewport.Y; };	

	CCamera				*GetCameraForViewpoint(int n) { return mCamera[n]; }
	CPlayer				*GetPlayerForViewpoint(int n) { return mPlayer[n]; }
	CViewport			*GetViewportForViewpoint(int n) { return &mViewport[n]; }
	int					GetNumViewpoints() { return mViewpoints; }
	void				SelectViewpoint(int n);
	
	void				SetViewpoint(int viewpoint,CCamera *camera,CViewport *viewport,CPlayer *player);
	void				SetNumViewpoints(int n);
	
	float				GetRenderTime();	
	
	int					GetHitEffectFactorR() { return mHitEffectFactorR; };
	int					GetHitEffectFactorG() { return mHitEffectFactorG; };
	int					GetHitEffectFactorB() { return mHitEffectFactorB; };

	CLANDSCAPE			*GetLandscape() { return mLandscape; };
	
	void				UpdatePos(CCamera* cam); // ##GC changed from pos to camera 19/9/01
	void				ResetPos(int x, int y);
	void				BuildLevelSpecifics();
	void				InitDamageSystem();

	void				LoadMixers(int set);

	class	CMapTex*	GetMapTex() { return mMapTexs; }
	
	void				SetKempyCube(UBYTE number);	
	
	void				SetWater(UBYTE number);

	void				SetupLights();

	void				LogLight(const FVector &inPos,eLightType lt,float mRadius);
	void				AddDamage(float x, float y, int size) { GetLandscape()->AddDamage(x, y, size);	}
	void				RemoveDamage(float x, float y, int size) { GetLandscape()->AddDamage(x, y, -size);	} // note: size flipped on remove
	void				LockCurrentDamage() { GetLandscape()->LockCurrentDamage();	}
	void				SwitchAllLights(BOOL onoff);	
	
	void				UpdateArea(
							SINT	inStartX,
							SINT	inStartY,
							SINT	inFinishX,
							SINT	inFinishY, BOOL updategeometryinstead=FALSE);

	void				SetTreeAlphaMode( const bool aSet );
	void				EnableAdditiveAlpha();
	void				EnableAlpha();
	void				DisableAlpha();
	void				DisableLighting();
	void				EnableLighting();

	void				ToggleHudAlphaMode() { mHudAditive = !mHudAditive; }
	BOOL				HudAditive()		{ return mHudAditive; }

	class CLightManager	*GetLights() { return(mLights); };

	void				SetSky(UBYTE number);
	
	class CTEXTURE*		GetHilightTexture() {return mHilightTexture;};
	void				AccumulateResources(CResourceAccumulator *accumulator);

	void				Serialize(class CChunker *c,CResourceAccumulator *ra);
	void				Deserialize(class CChunkReader *c);
	
//#ifdef EDITORBUILD2
	BOOL				mNavDisplay;
	void				LandscapeNavDisplay(BOOL onoff) {mNavDisplay=onoff;};
	BOOL				GetNavDisplayMODE() { return mNavDisplay;};
	FVector				GetCursorOffsetMatrix(float x,float y);
//#endif

	bool		mRenderLandscape;
	int			mCurrentViewpoint;
	
	BOOL		mHudAditive;

	ULONG		mDrawDebugStuff;

	BOOL		mDebugNoParticles;
	BOOL		mDrawPolyBuckets;
};

#ifdef _DIRECTX

#include "DXEngine.h"
extern class CDXEngine ENGINE;

#elif TARGET == PS2

#include "PS2Engine.h"
extern class CPS2Engine ENGINE;

#endif

#endif