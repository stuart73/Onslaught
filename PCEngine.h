// PCEngine.h: interface for the CPCEngine class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PCENGINE_H
#define PCENGINE_H

#include "Engine.h"
#include "Landscape.h"
#include "Lights.h"
#include "ResourceAccumulator.h"
   //=============================================-===--==---=---- ---  --   -
  //
 // EXPERIMENTAL NEW PC WATER TOGGLE
//
//#define NEW_WATER
//=============================================-===--==---=---- ---  --   -

class CPCEngine : public CEngine
{
	friend class		CEditorRenderer;
	
						CWaterReflection	*mWaterReflection;

	class				CTEXTURE			*mScreenTexture;

	bool				mCaptureScreen;
	int					mCaptureAreaTop;
	int					mCaptureAreaBottom;
	int					mBlurAlpha;
	int					mNewBlurAlpha; // Needed as the effect of changing this must start *next* frame!
	BOOL				mHudAditive;
	D3DLOCKED_RECT		mTopLevelLockedRect;
	int					mMaxValidX;
	int					mMaxValidY;
	
public:
//						CDetailLevel*		mLevels;
	

						CPCEngine();
						~CPCEngine();
	virtual BOOL		Init();
	virtual void		InitResources();
	virtual void		ShutDown();
	
#if TARGET == PC
	class	CPCSky*		GetSky() { return mSky; }
# elif TARGET == XBOX
	class	CXBOXSky*	GetSky() { return mSky; }
#endif
	
	class CTEXTURE		*GetDefaultTexture()	{return mDefaultTexture;}; //###

	class CTEXTURE		*GetScreenTexture() { return mScreenTexture; };
	void				TriggerScreenCapture() { mCaptureScreen=true; mCaptureAreaTop=0; mCaptureAreaBottom=1000; };
	void				TriggerPartialScreenCapture(int top,int bottom) { mCaptureScreen=true; mCaptureAreaTop=top; mCaptureAreaBottom=bottom; };
	void				SetBlurAlpha(int a,bool thisframe);

	void				DrawDebugCuboid(DWORD col, FVector &axes, FVector &pos, FMatrix ori, CTEXTURE* for_texture) ;
	void				DrawDebugLine(DWORD col, FVector &start, FVector &end);
	void				RenderArrow(FVector v1, FVector v2, DWORD col);

	void				ToggleHudAlphaMode() { mHudAditive = !mHudAditive; }
	BOOL				HudAditive()		{ return mHudAditive; }

	CTEXTURE*			GetOutlineTexture() { return mTexOutline; }
	CTEXTURE*			GetOpaqueTexture()	{ return mTexOpaque ; }

	void				SetGammaBias(float value);

	// Stuff for other Rendering things to use
	HRESULT				RenderLandscape();
	HRESULT				PreRender(CViewport *viewport);
	HRESULT				Render(int viewpoint);
	HRESULT				PostRender(CViewport *viewport);

	class CDetailLevel*	GetDetailLevel(int lev=4);		// default to the top level

	void				SetDefaultMaterial();
	
	CTEXTURE*			mDefaultTexture;
	CTEXTURE*			mTexOutline;
	CTEXTURE*			mTexOpaque;
	CTEXTURE*			mTexEdArrow;
	class CVertexShader	*mDefaultShader;
	
	D3DGAMMARAMP		mDefaultGammaRamp;

	bool				mRenderReflections;
	bool				mTextureReflections;
	int					mRipplePhase;
	BOOL				mFirstFrame;

	void				SetModulate2X( const bool aSet );
	
	CTEXTURE*			GetWaterTexture() { return mWater->GetTexture();}

	void				AccumulateResources( CResourceAccumulator * accumulator );
};
//=============================================-===--==---=---- ---  --   -

#endif // PCENGINE_H
