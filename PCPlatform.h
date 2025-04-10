#ifndef PCPLATFORM_H
#define PCPLATFORM_H

// don't include this directly - include "Platform.h" instead

#define KEYCODE_BACK		VK_BACK
#define KEYCODE_TAB			VK_TAB
#define KEYCODE_CLEAR		VK_CLEAR
#define KEYCODE_RETURN		VK_RETURN
#define KEYCODE_SHIFT		VK_SHIFT
#define KEYCODE_CONTROL		VK_CONTROL
#define KEYCODE_PAUSE		VK_PAUSE
#define KEYCODE_ESCAPE		VK_ESCAPE
#define KEYCODE_SPACE		VK_SPACE
#define KEYCODE_END			VK_END
#define KEYCODE_HOME		VK_HOME
#define KEYCODE_LEFT		VK_LEFT
#define KEYCODE_RIGHT		VK_RIGHT
#define KEYCODE_UP			VK_UP
#define KEYCODE_DOWN		VK_DOWN
#define KEYCODE_PRINT		VK_PRINT
#define KEYCODE_INSERT		VK_INSERT
#define KEYCODE_DELETE		VK_DELETE
#define KEYCODE_NUMPAD0		VK_NUMPAD0
#define KEYCODE_NUMPAD1		VK_NUMPAD1
#define KEYCODE_NUMPAD2		VK_NUMPAD2
#define KEYCODE_NUMPAD3		VK_NUMPAD3
#define KEYCODE_NUMPAD4		VK_NUMPAD4
#define KEYCODE_NUMPAD5		VK_NUMPAD5
#define KEYCODE_NUMPAD6		VK_NUMPAD6
#define KEYCODE_NUMPAD7		VK_NUMPAD7
#define KEYCODE_NUMPAD8		VK_NUMPAD8
#define KEYCODE_NUMPAD9		VK_NUMPAD9
#define KEYCODE_MULTIPLY	VK_MULTIPLY	
#define KEYCODE_ADD			VK_ADD
#define KEYCODE_SEPARATOR	VK_SEPARATOR
#define KEYCODE_SUBTRACT	VK_SUBTRACT
#define KEYCODE_DIVIDE		VK_DIVIDE
#define KEYCODE_F1			VK_F1
#define KEYCODE_F2			VK_F2
#define KEYCODE_F3			VK_F3
#define KEYCODE_F4			VK_F4
#define KEYCODE_F5			VK_F5
#define KEYCODE_F6			VK_F6
#define KEYCODE_F7			VK_F7
#define KEYCODE_F8			VK_F8
#define KEYCODE_F9			VK_F9
#define KEYCODE_F10			VK_F10
#define KEYCODE_F11			VK_F11
#define KEYCODE_F12			VK_F12
#define KEYCODE_NUMLOCK		VK_NUMLOCK
#define KEYCODE_SCROLL		VK_SCROLL

class	CFrameTimer;
class	CViewport;

class	CPCPlatform : public CPlatform
{
public:
	CPCPlatform()
	{
		mFrameTimer = NULL;
		mFont=NULL;
		mDebugFont=NULL;
		mSmallFont=NULL;
		mTitleFont=NULL;
		mXboxFont=NULL;
		mSmallXboxFont = NULL;
	}

	BOOL		Init();
	void		Shutdown();

	EQuitType	Process();	// process system stuff

	// rendering
	BOOL		BeginScene();
	void		EndScene();
	void		DeviceFlip(BOOL in_game);
	void		ClearScreen(DWORD col);

	SINT		GetScreenWidth();
	SINT		GetScreenHeight();

	// profiling
	float		GetFPS();
	float		GetSysTimeFloat();

	// input
	BOOL		UpdateJoystick(int joypad);
	BOOL		KeyOn(SINT c);
	BOOL		KeyOnce(SINT c);
	void		FlushInputBuffers();
	void		SetKeytrap(pKeyTrapper trap);
	
	// rumble
	void		TriggerRumble( int pad );
	void		SetRumbleEnabled( BOOL aRumble );
	void		SetVertexShadersEnabled( BOOL aShaders );

	void		FMatrixToD3DMatrix(D3DMATRIX *out,FMatrix *in);

	void		SetGeforce3(BOOL f) { mGeforce3=f; };
	BOOL		IsGeforce3() { return(mGeforce3); };
		
	void		SetMemorySize(long size) { mMemorySize=size; };
	long		GetMemorySize() { return(mMemorySize);	};
	
	CBITMAPFONT	*Font( EFontType aFontType );
	
	BOOL		Ask(char *msg);
	
	void		MakeD3DViewport(D3DVIEWPORT8 *out,CViewport *in);
	void		SetViewport(CViewport *vp);
	
	int			GetWindowWidth();
	int			GetWindowHeight();

	CBITMAPFONT*			Font() { return mFont; };
	CBITMAPFONT*			DebugFont() { return mDebugFont; };
	CBITMAPFONT*			SmallFont() { return mSmallFont ; };	
	CBITMAPFONT*			TitleFont() { return mTitleFont ; };
	
	void		AccumulateResources(class CResourceAccumulator *accumulator);
	void		Serialize(class CChunker *c,class CResourceAccumulator *ra);
	void		Deserialize(class CChunkReader *c);
	void		InitFonts();
	
	void		SetRegKey(char *keyname,char *value);
	void		GetRegKey(char *keyname,char *value);

protected:
	CFrameTimer		*mFrameTimer;

	LARGE_INTEGER	mFrequency;

	float			mClockDivisor;

	BOOL			mGeforce3;

	long			mMemorySize;

    class CBITMAPFONT	*mFont;              // Font for normal text
    class CBITMAPFONT	*mDebugFont;         // Font for debug text
	class CBITMAPFONT	*mSmallFont;	
	class CBITMAPFONT	*mTitleFont;	


	class CBITMAPFONT	*mXboxFont;
	class CBITMAPFONT	*mSmallXboxFont;
};

#endif