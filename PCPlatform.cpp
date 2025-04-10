#include	"Common.h"

#if TARGET == PC

#include	"Platform.h"
#include	"DX.h"
#include	"cframetimer.h"
#include	"float.h"
#include	"pcsimd.h"
#include	"VertexShader.h"
#include	"cliparams.h"
#include	"engine.h"
#include	"font.h"
#include	"mathtest.h"
#include	"chunker.h"
#include	"resourceaccumulator.h"

//*********************************************************************************
CPCPlatform	PLATFORM;

//*********************************************************************************
BOOL	CPCPlatform::Init()
{
    if(!(mFrameTimer = new( MEMTYPE_UNKNOWN ) CFrameTimer()))	return FALSE;
    mFrameTimer->Start(1.0f);

	// get how many processor ticks there are in a second
	QueryPerformanceFrequency((LARGE_INTEGER *)&mFrequency);

	// Can't do this here because the console isn't initialised yet!
	//CONSOLE.RegisterVariable("g_clockdivisor","Global clock divisor",CVar_float,&mClockDivisor);

	mClockDivisor=1.0f;

#ifdef MATH_TEST
	// Run math test
	
	MATHTEST.Run();
#endif

	// Work out what graphics card we're on

	mGeforce3=LT.IsThisAGeForce3();

	if (CLIPARAMS.mForcedCard)
	{
		mGeforce3=CLIPARAMS.mGeforce3;
	}
	else
	{
		if (!mGeforce3)
		{
			LT.ForceToWindow();			

			if (!PLATFORM.Ask("This game only runs on GeForce 3 cards.\nPress OK if you have a GeForce 3 card installed."))
				exit(1);

//			LT.ToggleFullscreen();
			
			mGeforce3=TRUE;
		}
	}

	BOOL vshaders=mGeforce3;

	if (CLIPARAMS.mForcedShaders)
		vshaders=CLIPARAMS.mVShaders;
	
	if (vshaders)
	{
		TRACE("Vertex shader suppport ENABLED\n");
		PLATFORM.SetVertexShadersEnabled(TRUE);
	}
		
	CVertexShader::InitShaders();

	return TRUE;
}

//*********************************************************************************

void	CPCPlatform::InitFonts()
{
	// Fonts
	
	if (!mFont)
	{
		TRACE("Warning : loading font manually\n");
		mFont=new( MEMTYPE_FONT ) CBITMAPFONT();
		//	mFont->InitialiseAsBitmapFont("font.tga",32);
		mFont->InitialiseAsBitmapFont("font22.512.tga",32);
		mFont->EnableCharSwapHack(); // JCL - see header
	}
	
	if (!mDebugFont)
	{
		TRACE("Warning : loading debug font manually\n");
		mDebugFont=new( MEMTYPE_FONT ) CBITMAPFONT();
		mDebugFont->InitialiseAsSysFont(ToTCHAR("Terminal"), 7, 0 );
	}
	
	if (!mSmallFont)
	{
		TRACE("Warning : loading small font manually\n");
		mSmallFont =new( MEMTYPE_FONT ) CBITMAPFONT();
		//	mSmallFont->InitialiseAsBitmapFont("smallfont.tga",16);	
		mSmallFont->InitialiseAsBitmapFont("Font13PS.tga",16);	
	}
	
	if (!mTitleFont)
	{
		TRACE("Warning : loading title font manually\n");
		mTitleFont =new( MEMTYPE_FONT ) CBITMAPFONT();
		mTitleFont->InitialiseAsBitmapFont("TitleFont.tga",32);	
	}
	
	if (!mXboxFont)
	{
		TRACE("Warning : loading XBox font manually\n");
		mXboxFont = new( MEMTYPE_FONT ) CBITMAPFONT();
		mXboxFont->InitialiseAsBitmapFont("font22.512Xbox.tga",32);
	}	
	if (!mSmallXboxFont)
	{
		TRACE("Warning : loading Small XBox font manually\n");
		mSmallXboxFont = new( MEMTYPE_FONT ) CBITMAPFONT();
		mSmallXboxFont->InitialiseAsBitmapFont("font13Xbox.tga",16);
	}	
}

//*********************************************************************************
void	CPCPlatform::Shutdown()
{
	SAFE_DELETE(mFont);
	SAFE_DELETE(mDebugFont);
	SAFE_DELETE(mSmallFont);
	SAFE_DELETE(mTitleFont);
	SAFE_DELETE(mXboxFont);
	SAFE_DELETE(mSmallXboxFont);
	SAFE_DELETE(mFrameTimer);
}

//*********************************************************************************
EQuitType	CPCPlatform::Process()
{
	EQuitType	quit = QT_NONE;
	BOOL		more = TRUE;
	do
	{
		more = LT.HandlingSystemStuff();
		if (LT.WantsToQuit())
			quit = QT_QUIT_TO_SYSTEM;
	} while (more);

	return quit;
}	

//*********************************************************************************
BOOL	CPCPlatform::BeginScene()
{
	return SUCCEEDED(LT.D3D_BeginScene());
}

//*********************************************************************************
void	CPCPlatform::EndScene()
{
	LT.D3D_EndScene();
}

//*********************************************************************************
void	CPCPlatform::DeviceFlip(BOOL in_game)
{
	if (mFrameTimer)
		mFrameTimer->Frame();

	LT.DisplayScreen();
}

//*********************************************************************************
void	CPCPlatform::ClearScreen(DWORD col)
{
	LT.D3D_Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, col, 1.0f, 0L );
}

//*********************************************************************************
SINT		CPCPlatform::GetScreenWidth()
{
	return LT.GetWindowWidth();
}
	
//*********************************************************************************
SINT		CPCPlatform::GetScreenHeight()
{
	return LT.GetWindowHeight();
}

//*********************************************************************************
float	CPCPlatform::GetFPS()
{
	if (mFrameTimer)
		return mFrameTimer->GetFramesPerSec();

	return 1.f;
}

BOOL	CPCPlatform::UpdateJoystick(int joypad)
{
	if( FAILED( LT.UpdateJoystick(joypad) ) )
	{
		return FALSE;
	}
	return TRUE;
}

//*********************************************************************************
BOOL	CPCPlatform::KeyOn(SINT c)
{
	return LT.xKeyOn(c);
}

//*********************************************************************************
BOOL	CPCPlatform::KeyOnce(SINT c)
{
	return LT.xKeyOnce(c);
}

//*********************************************************************************
void	CPCPlatform::FlushInputBuffers()
{
	LT.FlushInputBuffers();
}

//*********************************************************************************
void	CPCPlatform::SetKeytrap(pKeyTrapper trap)
{
	LT.SetKeytrap( trap );
}

//*********************************************************************************

float	CPCPlatform::GetSysTimeFloat()
{
	LARGE_INTEGER ts;
	
	static	BOOL	fs_done = FALSE;
	static LARGE_INTEGER first_seen;

	if(mFrequency.QuadPart)
	{
		QueryPerformanceCounter(&ts);
		if (!fs_done)
		{
			first_seen.QuadPart = ts.QuadPart;
			fs_done = TRUE;
		}
		// JCL - think about floating point inaccuracies here!!!!!
		return float(ts.QuadPart- first_seen.QuadPart) / (float(mFrequency.QuadPart)*mClockDivisor);
	}

	return float(timeGetTime()) / 1000.0f;	
}

//*********************************************************************************

void	CPCPlatform::TriggerRumble( int pad )
{
	LT.TriggerRumble( pad );
}

//*********************************************************************************

void	CPCPlatform::SetRumbleEnabled( BOOL aRumble )
{
	LT.mRumbleEnabled = aRumble;
}

//*********************************************************************************

void	CPCPlatform::SetVertexShadersEnabled( BOOL aShaders )
{
	CVertexShader::VertexShadersEnabled = aShaders;
}

//*********************************************************************************

void	CPCPlatform::FMatrixToD3DMatrix(D3DMATRIX *out,FMatrix *in)
{
	out->_11 = in->Row[0].X; out->_12 = in->Row[1].X; out->_13 = in->Row[2].X;
	out->_21 = in->Row[0].Y; out->_22 = in->Row[1].Y; out->_23 = in->Row[2].Y;
	out->_31 = in->Row[0].Z; out->_32 = in->Row[1].Z; out->_33 = in->Row[2].Z;
	out->_14 = out->_24 = out->_34 = 0.f;
	out->_41 = 0.0f; // X pos
	out->_42 = 0.0f; // Y pos
	out->_43 = 0.0f; // Z pos
	out->_44 = 1.f;
}

//*********************************************************************************

CBITMAPFONT	*CPCPlatform::Font( EFontType aFontType )
{
	switch( aFontType )
	{
	case FONT_NORMAL:
		return(Font());
	case FONT_DEBUG:
		return(DebugFont());
	case FONT_SMALL:
		return(SmallFont());
	case FONT_TITLE:
		return(TitleFont());
	}
	return 0;
}

//*********************************************************************************

BOOL CPCPlatform::Ask(char *msg)
{
	if (MessageBox(NULL,ToTCHAR(msg),ToTCHAR("Onslaught"),MB_OKCANCEL | MB_ICONQUESTION)==IDOK)
		return(TRUE);
	else
		return(FALSE);
}

//*********************************************************************************

void CPCPlatform::MakeD3DViewport(D3DVIEWPORT8 *out,CViewport *in)
{
	out->Width=in->Width;
	out->Height=in->Height;
	out->X=in->X;
	out->Y=in->Y;
	out->MinZ=in->MinZ;
	out->MaxZ=in->MaxZ;
}

//*********************************************************************************

void CPCPlatform::SetViewport(CViewport *vp)
{
	D3DVIEWPORT8 d3dvp;

	MakeD3DViewport(&d3dvp,vp);

	LT.D3D_SetViewport(&d3dvp);
}

//*********************************************************************************

int	CPCPlatform::GetWindowWidth()
{
	return(LT.GetWindowWidth());
};

//*********************************************************************************

int	CPCPlatform::GetWindowHeight()
{
	return(LT.GetWindowHeight());
};

//*********************************************************************************
#ifdef RESBUILDER

void CPCPlatform::Serialize(CChunker *c,CResourceAccumulator *ra)
{
	c->Start(MKID("PLAT"));

	if (ra->GetTargetPlatform()==XBOX)
	{
		mXboxFont->Serialize(c,ra);
		mDebugFont->Serialize(c,ra);
		mTitleFont->Serialize(c,ra);
		mSmallXboxFont->Serialize(c,ra);	
	}
	else if (ra->GetTargetPlatform()==PS2)
	{
		mFont->Serialize(c,ra);
		mSmallFont->Serialize(c,ra);
		mTitleFont->Serialize(c,ra);
		mSmallFont->Serialize(c,ra);
	}
	else
	{
		mFont->Serialize(c,ra);
		mDebugFont->Serialize(c,ra);
		mTitleFont->Serialize(c,ra);
		mSmallFont->Serialize(c,ra);
	}

	c->End();	
}
#endif
//*********************************************************************************

void CPCPlatform::Deserialize(CChunkReader *c)
{
	if (mFont)
	{
		TRACE("Warning : deserializing font twice!\n");
		SAFE_DELETE(mFont);
	}

	if (mXboxFont)
	{
		TRACE("Warning : deserializing font twice!\n");
		SAFE_DELETE(mXboxFont);
	}

	if (mSmallXboxFont)
	{
		TRACE("Warning : deserializing font twice!\n");
		SAFE_DELETE(mSmallXboxFont);
	}

	if (mFont)
	{
		TRACE("Warning : deserializing font twice!\n");
		SAFE_DELETE(mFont);
	}

	if (mDebugFont)
	{
		TRACE("Warning : deserializing font twice!\n");
		SAFE_DELETE(mDebugFont);
	}

	if (mTitleFont)
	{
		TRACE("Warning : deserializing font twice!\n");
		SAFE_DELETE(mTitleFont);
	}

	if (mSmallFont)
	{
		TRACE("Warning : deserializing font twice!\n");
		SAFE_DELETE(mSmallFont);
	}
	
	mFont=new CDXBitmapFont();
	mXboxFont=NULL;
	mSmallXboxFont=NULL;
	mDebugFont=new CDXBitmapFont();
	mTitleFont=new CDXBitmapFont();
	mSmallFont=new CDXBitmapFont();
	
	mFont->Deserialize(c);
	mDebugFont->Deserialize(c);
	mTitleFont->Deserialize(c);
	mSmallFont->Deserialize(c);

	mFont->EnableCharSwapHack(); // JCL - see header
}

//*********************************************************************************

void	CPCPlatform::AccumulateResources(CResourceAccumulator *accumulator)
{
}

//*********************************************************************************

void CPCPlatform::SetRegKey(char *keyname,char *value)
{
	HKEY key;
	DWORD disposition;

	if (RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Lost Toys\\Battle Engine Aquila",0,"REG_SZ",REG_OPTION_VOLATILE,KEY_ALL_ACCESS,NULL,&key,&disposition)!=ERROR_SUCCESS)
	{
		ASSERT(0);
	}
	
	if (RegSetValueEx(key,keyname,0,REG_SZ,(unsigned char *) value,strlen(value)+1)!=ERROR_SUCCESS)
	{
		ASSERT(0);
	}
	
	RegCloseKey(key);
}

//*********************************************************************************

void CPCPlatform::GetRegKey(char *keyname,char *value)
{
	HKEY key;
	DWORD disposition;
	if (RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Lost Toys\\Battle Engine Aquila",0,"REG_SZ",REG_OPTION_VOLATILE,KEY_ALL_ACCESS,NULL,&key,&disposition)!=ERROR_SUCCESS)
	{
		ASSERT(0);
	}
	
	DWORD size=255;
	DWORD type=REG_SZ;
	
	if (RegQueryValueEx(key,keyname,0,&type,(unsigned char *) value,&size)!=ERROR_SUCCESS)
	{
		ASSERT(0);
	}
	
	
	RegCloseKey(key);
}


#endif