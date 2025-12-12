// DXEngine.h: interface for the CDXEngine class.
//
//////////////////////////////////////////////////////////////////////

#ifndef DXENGINE_H
#define DXENGINE_H

#include "Engine.h"
#include "Landscape.h"
#include "Lights.h"
#include "ResourceAccumulator.h"

class	CParticleDescriptor;

   //=============================================-===--==---=---- ---  --   -
  //
 // EXPERIMENTAL NEW DX WATER TOGGLE
//
//#define NEW_WATER
//=============================================-===--==---=---- ---  --   -

class CDXEngine : public CEngine
{
	friend class		CEditorRenderer;
	
	class				CTEXTURE			*mScreenTexture;

	bool				mCaptureScreen;
	int					mCaptureAreaTop;
	int					mCaptureAreaBottom;
	int					mBlurAlpha;
	int					mNewBlurAlpha; // Needed as the effect of changing this must start *next* frame!
	D3DLOCKED_RECT		mTopLevelLockedRect;
	int					mMaxValidX;
	int					mMaxValidY;
	
public:
//						CDetailLevel*		mLevels;
	
	void				ShutdownRestartLoop();

						CDXEngine();
						~CDXEngine();
	virtual BOOL		Init();
	virtual void		InitResources();
	virtual void		ShutDown();
	
	
	class CTEXTURE		*GetDefaultTexture()
	{
		if (!mDefaultTexture)
		{
			// This sometimes gets called before the engine is initresources'ed but after the textures are available...
			mDefaultTexture = CTEXTURE::GetTextureByName("meshtex\\default.tga");
		}

		return mDefaultTexture;
	}; //###

	class CTEXTURE		*GetScreenTexture() { return mScreenTexture; };
	void				TriggerScreenCapture() { mCaptureScreen=true; mCaptureAreaTop=0; mCaptureAreaBottom=1000; };
	void				TriggerPartialScreenCapture(int top,int bottom) { mCaptureScreen=true; mCaptureAreaTop=top; mCaptureAreaBottom=bottom; };
	void				SetBlurAlpha(int a,bool thisframe);

	void				DrawDebugCuboid(DWORD col, FVector &axes, FVector &pos, FMatrix ori, CTEXTURE* for_texture) ;
	void				DrawDebugLine(DWORD col, FVector &start, FVector &end);
	void				RenderArrow(FVector v1, FVector v2, DWORD col);

	CTEXTURE*			GetOutlineTexture() { return mTexOutline; }
	CTEXTURE*			GetOpaqueTexture()	{ return mTexOpaque ; }

	void				SetGammaBias(float value);

	// Stuff for other Rendering things to use
	HRESULT				RenderLandscape();
	HRESULT				PreRender(CViewport *viewport);
	HRESULT				Render(int viewpoint);
	HRESULT				PostRender(CViewport *viewport);

	class CDetailLevel*	GetDetailLevel(int lev=4);		// default to the top level

	void				ReallySetDefaultMaterial();
	void				SetDefaultMaterial() {}        // do nothing.
	
	CTEXTURE*			mDefaultTexture;
	CTEXTURE*			mTexOutline;
	CTEXTURE*			mTexOpaque;
#if TARGET == PC
	CTEXTURE*			mTexEdArrow;
#endif
	class CVertexShader	*mDefaultShader;
	
	D3DGAMMARAMP		mDefaultGammaRamp;

	bool				mRenderReflections;
	bool				mTextureReflections;
	int					mRipplePhase;
	BOOL				mFirstFrame;

	void				SetModulate2X( const bool aSet );
	
//	CTEXTURE*			GetWaterTexture() { return mWater->GetTexture();}
	

	void				AccumulateResources( CResourceAccumulator * accumulator );


	// JCL - Backup matrices for HUD
	D3DMATRIX			mWorldMatrices[VIEWPOINTS];
	D3DMATRIX			mViewMatrices[VIEWPOINTS];
	D3DMATRIX			mProjectionMatrices[VIEWPOINTS];

	CParticleDescriptor	*mSunPD;
};
//=============================================-===--==---=---- ---  --   -

#endif // DXENGINE_H
