#ifndef LT_SHELL_INCLUDE
#define LT_SHELL_INCLUDE

#include "deviceobject.h"
#include "Platform.h"

// index buffers are index buffers on the PC.
#define CBEIndexBuffer IDirect3DIndexBuffer8

#define MAX_JOYPADS 4

#ifdef  _DEBUG
#define	LT_DEBUG
#else
#ifdef  OPTIMISED_DEBUG
#define	LT_DEBUG
#endif
#endif

#ifdef	LT_DEBUG
#define	CHECK_D3D_STATE(TAG, RSX,TSX)	LT.CheckD3DState(TAG, RSX, TSX)
#else
#define	CHECK_D3D_STATE(TAG, RSX,TSX)
#endif

#ifdef LT_DEBUG
#define SANITY_CHECK_AND_RETURN(CALL) \
{ \
	HRESULT sanity_check_hresult=CALL;\
	ASSERT(!FAILED(sanity_check_hresult));\
	return(sanity_check_hresult);\
}
#else
#define SANITY_CHECK_AND_RETURN(CALL)	return(CALL);
#endif

#define	UNDEFAULTED	0xF0CCFACE
extern class PCLTShell LT;

extern int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class provides just about all the
//       functionality we want, so we're just supplying stubs to interface with
//       the non-C++ functions of the app.
//-----------------------------------------------------------------------------
#define	N_RENDERSTATES			172
#define N_TEXTURESTAGESTATES	30
#define INVALID_STATE			0xfedcba98

#ifndef EDITORBUILD2
class PCLTShell : public CD3DApplication
#else
class PCLTShell : public CEditorD3DApp
#endif
{
// cached states and stuff
static	DWORD					mRenderStates[N_RENDERSTATES];
static	DWORD					mTextureStageStates[8][N_TEXTURESTAGESTATES];
#ifdef LT_DEBUG
static	DWORD					mRenderStates2[N_RENDERSTATES];
static	DWORD					mTextureStageStates2[8][N_TEXTURESTAGESTATES];
#endif
static	IDirect3DBaseTexture8*	mCurrentTextures[8];
static	DWORD					mLastVertexShader;
//
	bool				mRunning;
	static	char		mNotRI[N_RENDERSTATES];

	BOOL				DodgyJoyX[MAX_JOYPADS];

	static DeviceObject*mDeviceObjects;
	BOOL				mD3DActive;

	int					mDeadZone;
	
	BYTE				KeyDown[256];
	BYTE				KeyWasDown[256];
	DIJOYSTATE2*		mJoyState[MAX_JOYPADS];
	DIJOYSTATE2*		mOldJoyState[MAX_JOYPADS];
    HRESULT ConfirmDevice( D3DCAPS8*, DWORD, D3DFORMAT );

	int					mD3DErrorCount;
protected:
//    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FinalCleanup();


	LPDIRECTINPUT8			pDI;         
	LPDIRECTINPUTDEVICE8	pJoystick[MAX_JOYPADS];     
	DIDEVCAPS				diDevCaps;
	bool					mJoypadIsNewtype[MAX_JOYPADS];
	int						mJoypads;		

	int						mMouseX;
	int						mMouseY;
	int						mMouseButtons;

	void					ReleaseSurfaces();
	HRESULT					RestoreSurfaces();

	void					InitialiseRumble(int joypad);

	pKeyTrapper				mCurrentKeytrap;

	BOOL					m_bSmallTexturesDisabled, m_bTextureCompressionDisabled;

public:
							PCLTShell();
							~PCLTShell();

	LPDIRECTINPUTEFFECT		mRumbleEffect[MAX_JOYPADS];
	bool					mRumbleEnabled; // Do not use this when playing effects - use mRumbleEffect!=NULL instead!
	bool					mRumbleInitialised[MAX_JOYPADS];

	void					TriggerRumble(int pad);
	
	HRESULT					UpdateJoystick(int joypad);	

	int						GetMouseX() { return mMouseX; };
	int						GetMouseY() { return mMouseY; };
	bool					GetMouseLButton() { return ((mMouseButtons & MK_LBUTTON)==MK_LBUTTON); };
	bool					GetMouseMButton() { return ((mMouseButtons & MK_MBUTTON)==MK_MBUTTON); };
	bool					GetMouseRButton() { return ((mMouseButtons & MK_RBUTTON)==MK_RBUTTON); };
	void					ClearD3DErrorCount() { mD3DErrorCount = 0;	}
//    HRESULT Render();
//    HRESULT FrameMove();
// ### GC simplifying main loop
	MSG		mMsg;
	HACCEL  mhAccel;
    BOOL	SetupSystemMessageHandler();
    BOOL	HandlingSystemStuff();
	void	DisplayScreen();
	int		MsgInfo()		{return (INT)mMsg.wParam;}
	BOOL	WantsToQuit()	{return (WM_QUIT == mMsg.message);}
	void	IWantToQuit()	{SendMessage( LT.GetHWnd(), WM_CLOSE, 0, 0 );}
	int		MainLoop();
	void	TriggerQuit();

	BOOL	DisableSmallTextures(BOOL bSmallTexturesDisabled = TRUE)
	{
		BOOL bResult = m_bSmallTexturesDisabled;
		m_bSmallTexturesDisabled=bSmallTexturesDisabled;
		return bResult;
	}
	BOOL	SmallTexturesDisabled() {return m_bSmallTexturesDisabled;}
	BOOL	DisableTextureCompression(BOOL bTextureCompressionDisabled = TRUE)
	{
		BOOL bResult = m_bTextureCompressionDisabled;
		m_bTextureCompressionDisabled=bTextureCompressionDisabled;
		return bResult;
	}
	BOOL	TextureCompressionDisabled() {return m_bTextureCompressionDisabled;}

	HINSTANCE	mhInstance;
//

	BOOL					D3DActive() { return mD3DActive;};

	void					InvalidateAllStates();
//	Name changed to avoid missuse...	
	
	LPDIRECT3DDEVICE8		DeviceForD3DXTextureLoad() { return m_pd3dDevice;};	// will go when we encapsulate texture load, we'll probably remove D3DX at that point too... 
	LPDIRECT3DDEVICE8		GetDevice() { return m_pd3dDevice;}; // Needed to match XBox 
// Encapsulated device stuff
// SetRenderState and SetTeXtureStageState are abreviated to SRS and STS
// Note: these are all void as the result is rarely checked
//
// If you've hit this assert, it's because you've used a stage not in the default setup ->!!!!!!!!!!!!!!!!!!!!!!!<- add it to state.cpp
	void	SRS(D3DRENDERSTATETYPE state, DWORD value) {
															//ASSERT(mNotRI[state]);
															DWORD* t=&mRenderStates[state];
														//	ASSERT(state<N_RENDERSTATES);
														//	ASSERT(state>=0);
														//	ASSERT(*t!=INVALID_STATE);
															if(*t!=value) m_pd3dDevice->SetRenderState(state,*t=value);
														}
	void	RI_SRS(D3DRENDERSTATETYPE state, DWORD value) {
															DWORD* t=&mRenderStates[state];
														//	ASSERT(state<N_RENDERSTATES);
														//	ASSERT(state>=0);
														//	ASSERT(*t!=INVALID_STATE);
															if(*t!=value) m_pd3dDevice->SetRenderState(state,*t=value);
														}
	void	STS(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD val) {
														//					ASSERT(type<N_TEXTURESTAGESTATES);
														//					ASSERT(type>=0);
														//					ASSERT(stage<4);
														//					ASSERT(stage>=0);
																			DWORD* t=&mTextureStageStates[stage][type];
														//					ASSERT(*t!=INVALID_STATE);
																			if(*t!=val) m_pd3dDevice->SetTextureStageState(stage, type, *t=val);
																		}
#ifdef	LT_DEBUG
	void	CheckD3DState(char* tag, DWORD* rsexceptions, DWORD* tsexceptions);
	void	ForceRS(D3DRENDERSTATETYPE state, DWORD value) { m_pd3dDevice->SetRenderState(state,mRenderStates[state]=value); mRenderStates2[state]=value;}
	void	ForceTS(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD val) { m_pd3dDevice->SetTextureStageState(stage, type,mTextureStageStates[stage][type]=val);mTextureStageStates2[stage][type]=val;}
#else
	void	ForceRS(D3DRENDERSTATETYPE state, DWORD value) { m_pd3dDevice->SetRenderState(state,mRenderStates[state]=value);}
	void	ForceTS(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD val) { m_pd3dDevice->SetTextureStageState(stage, type,mTextureStageStates[stage][type]=val);}
#endif
// And for those few places where we do check...
	HRESULT	SRS_Ret(D3DRENDERSTATETYPE state, DWORD value) {	
																DWORD* t=&mRenderStates[state];
																if(*t==value) return S_OK;
																else
																{
																	HRESULT hr=m_pd3dDevice->SetRenderState(state, value);
																	if(!FAILED(hr))
																	{
																		*t=value;
																	}
																	return hr;
																}
															}
	void	D3D_SetTexture(  UINT Name, IDirect3DBaseTexture8* pTexture)				{ IDirect3DBaseTexture8** t=&mCurrentTextures[Name]; if(*t!=pTexture) m_pd3dDevice->SetTexture( Name, *t=pTexture);}
	void	D3D_SetVertexShader( DWORD Handle)											{ if(Handle!=mLastVertexShader) m_pd3dDevice->SetVertexShader( mLastVertexShader = Handle);}
//	Name is as D3D docs, Treat D3D_ as equivilent to pD3DDevice->

	HRESULT D3D_SetStreamSource( UINT StreamNumber,
		IDirect3DVertexBuffer8* pStreamData, UINT Stride)								{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetStreamSource( StreamNumber, pStreamData, Stride))}
	HRESULT	D3D_X_SetTransform(  D3DTRANSFORMSTATETYPE State,  CONST D3DMATRIX* pMatrix){ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetTransform(State, pMatrix))}
	HRESULT	D3D_GetRenderTarget(  IDirect3DSurface8** ppRenderTarget)					{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->GetRenderTarget(ppRenderTarget))}
	HRESULT D3D_X_GetLight( DWORD Index, D3DLIGHT8* pLight)								{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->GetLight( Index, pLight))}
	HRESULT D3D_X_SetLight( DWORD Index, CONST D3DLIGHT8* pLight)						{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetLight( Index, pLight))}
	HRESULT D3D_SetMaterial( CONST D3DMATERIAL8* pMaterial )							{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetMaterial( pMaterial))}
	HRESULT D3D_X_LightEnable( DWORD LightIndex, BOOL bEnable)							{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->LightEnable( LightIndex, bEnable))}
	HRESULT D3D_Clear(	DWORD Count, CONST D3DRECT* pRects, DWORD Flags,
		D3DCOLOR Color, float Z, DWORD Stencil)											{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->Clear(Count, pRects, Flags, Color, Z, Stencil))}

	// pData is used only the xbox build, it's ignored on the PC for now
	HRESULT D3D_CreateTexture(	UINT Width, UINT Height, UINT  Levels, DWORD Usage,
		D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture, void **pData)	{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreateTexture( Width, Height, Levels, Usage, Format, Pool, ppTexture))}

	HRESULT D3D_CreateCubeTexture(  UINT EdgeLength, UINT Levels, DWORD Usage,
		D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture)			{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreateCubeTexture( EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture));};
	
	int D3D_ReleaseTexture(IDirect3DTexture8* pTexture, void *pData)					{ return pTexture->Release();}
	HRESULT D3D_SetIndices( IDirect3DIndexBuffer8* pIndexData)	{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetIndices( pIndexData, 0))}
	HRESULT D3D_DrawIndexedPrimitive( D3DPRIMITIVETYPE Type, UINT MinIndex, 
		UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)							{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->DrawIndexedPrimitive(Type, MinIndex, NumVertices, StartIndex, PrimitiveCount))}
	HRESULT D3D_CreateIndexBuffer( UINT Length, DWORD Usage, D3DFORMAT Format,
		D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer)							{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreateIndexBuffer( Length, Usage, Format, Pool, ppIndexBuffer))}
	HRESULT D3D_DrawPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
		CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)					{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->DrawPrimitiveUP( PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride))}
	HRESULT D3D_X_GetTransform( D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)		{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->GetTransform( State, pMatrix))}
	HRESULT D3D_SetViewport( CONST D3DVIEWPORT8* pViewport)								{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetViewport( pViewport))}
	HRESULT D3D_BeginScene()															{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->BeginScene())}
	HRESULT D3D_EndScene()																{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->EndScene())}
	HRESULT D3D_GetViewport( D3DVIEWPORT8* pViewport)									{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->GetViewport(pViewport))}
	HRESULT D3D_SetVertexShaderConstant( DWORD Register, CONST void* pConstantData,
		DWORD  ConstantCount)															{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetVertexShaderConstant( Register, pConstantData, ConstantCount))}
	void D3D_SetGammaRamp(DWORD flags,CONST D3DGAMMARAMP *pRamp)						{ m_pd3dDevice->SetGammaRamp(flags,pRamp);}
	void D3D_GetGammaRamp(D3DGAMMARAMP *pRamp)											{ m_pd3dDevice->GetGammaRamp(pRamp);}
	HRESULT D3D_DrawIndexedPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex,
		UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData,
		D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride)													{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->DrawIndexedPrimitiveUP( PrimitiveType, MinIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride))}
	HRESULT D3D_CreateVertexBuffer( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
		IDirect3DVertexBuffer8** ppVertexBuffer, void **pData);
	int D3D_ReleaseVertexBuffer(IDirect3DVertexBuffer8 * pVertexBuffer, void *pData);
	HRESULT D3D_GetDepthStencilSurface( IDirect3DSurface8** ppZStencilSurface)			{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->GetDepthStencilSurface( ppZStencilSurface))}
	HRESULT D3D_SetRenderTarget( IDirect3DSurface8* pRenderTarget,
		IDirect3DSurface8* pNewZStencil)												{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetRenderTarget( pRenderTarget, pNewZStencil))}
	HRESULT D3D_DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex,
		UINT PrimitiveCount)															{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->DrawPrimitive( PrimitiveType, StartVertex, PrimitiveCount))}
	HRESULT D3D_GetDeviceCaps( D3DCAPS8* pCaps)											{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->GetDeviceCaps( pCaps))}
	HRESULT D3D_DeleteVertexShader( DWORD Handle)										{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->DeleteVertexShader( Handle))}
	HRESULT D3D_CreateVertexShader( CONST DWORD* pDeclaration, CONST DWORD* pFunction,
		DWORD* pHandle, DWORD Usage)													{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreateVertexShader( pDeclaration, pFunction, pHandle, Usage))}
	HRESULT D3D_CreateRenderTarget(UINT Width,UINT Height,D3DFORMAT Format,
								   D3DMULTISAMPLE_TYPE MultiSample,BOOL Lockable,
								   IDirect3DSurface8 **ppSurface)						{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreateRenderTarget(Width,Height,Format,MultiSample,Lockable,ppSurface))}
	HRESULT D3D_CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,IDirect3DSurface8 **ppSurface)					{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreateDepthStencilSurface(Width,Height,Format,MultiSample,ppSurface))}
	HRESULT D3D_UpdateTexture(IDirect3DBaseTexture8* pSourceTexture, 
		IDirect3DBaseTexture8* pDestinationTexture)										{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->UpdateTexture(pSourceTexture, pDestinationTexture))}	

	HRESULT D3D_SetPixelShader( DWORD Handle)											{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->SetPixelShader( Handle))}
	HRESULT D3D_CreatePixelShader( CONST DWORD* pFunction, DWORD* pHandle )				{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreatePixelShader( pFunction, pHandle))}
	HRESULT D3D_DeletePixelShader( DWORD Handle)										{ SANITY_CHECK_AND_RETURN(m_pd3dDevice->DeletePixelShader( Handle))}
	
	
	
//	NOTE:	         this x | is to invalidate these interfaces to force use via platform 
	BYTE					xKeyOn(int c) { return KeyDown[c]; };
	BYTE					xKeyOnce(int c) { BYTE a=KeyWasDown[c]; KeyWasDown[c]=0; return a; };
	void					FlushInputBuffers();

	void					AddDeviceObject(DeviceObject* devob)			{ devob->mNext=mDeviceObjects; mDeviceObjects=devob;}; 
	BOOL					RemoveDeviceObject(DeviceObject* devob);
    LRESULT					MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK	PCLTShell::EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext );
	static BOOL CALLBACK	PCLTShell::EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
	static BOOL CALLBACK	PCLTShell::EnumEffectsProc(LPCDIEFFECTINFO pei, LPVOID pv);
	ETextureFormat			GetDisplayFormat();
	
	HRESULT					InitDirectInput( HWND hDlg );
	VOID					FreeDirectInput();
	int						DeadZone(const int axis);
	DIJOYSTATE2*			JoyState(int padnumber) { return mJoyState[padnumber]; };
	DIJOYSTATE2*			OldJoyState(int padnumber) { return mOldJoyState[padnumber]; };


	BOOL					JoyButtonOnce(int padnumber, int button) {	if ((!OldJoyState(padnumber)->rgbButtons[button]) &&	
															JoyState(padnumber)->rgbButtons[button]) 
															{ return TRUE ; } else return FALSE ; } 

	BOOL					JoyButtonOn(int padnumber, int button) {	if (JoyState(padnumber)->rgbButtons[button]) 
															{ return TRUE ; } else return FALSE ; } 

	BOOL					JoyButtonRelease(int padnumber, int button) {	if ((OldJoyState(padnumber)->rgbButtons[button]) &&	
															JoyState(padnumber)->rgbButtons[button] == FALSE) 
															{ return TRUE ; } else return FALSE ; } 

	void					ForceFile(char* dst, char* src, char* path);

	bool					Running()					{ return mRunning; }
	void					SetRunning(bool inRunning)	{ mRunning=inRunning; }
	void					DumpScreen(int number);
	int						GetWindowWidth() { return m_d3dpp.BackBufferWidth; }
	int						GetWindowHeight() { return m_d3dpp.BackBufferHeight; }
	void					SetKeytrap(pKeyTrapper trap);

	int						GetBPP(D3DFORMAT fmt);
	BOOL					IsThisAGeForce3();
	void					ShowGFXCardInfo();
};

#endif 
