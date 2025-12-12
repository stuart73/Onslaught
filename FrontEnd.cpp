#include	"Common.h"

#include	"FrontEnd.h"
#include	"EventManager.h"
#include	"CLIParams.h"
#include	"Platform.h"
#include	"SoundManager.h"
#include	"Music.h"
#include    "Career.h"
#include	"RenderInfo.h"
#include	"SpriteRenderer.h"
#include	"engine.h"
#include	"meshrenderer.h"
#include	"BattleEngineDataManager.h"
#include	"WorldPhysicsManager.h"
#include	"FEMessBox.h"
#include	"memorycard.h"
#include	"reconnectinterface.h"
#include	"endleveldata.h"

#include	"Game.h"

#if TARGET == PS2
#include "PS2Profiler.h"
#endif

//*********************************************************************************
extern SINT performing_stress_test;

//*********************************************************************************

CFrontEnd::CFrontEnd(void)
:
mAutoSave(CFrontEnd::AUTO_SAVE_NOT),
mFirstRun(TRUE),
mSelectedLevel(-1),
mAllLevelsCheatActive(FALSE)
{
	mSaveFilename[0] = 0;
	mPlayer0ControllerPort = -1;
}

// slightly different loading speeds on the different platforms.
#define LOADING_STEP_1 80
#define LOADING_STEP_2 90
#define LOADING_STEP_3 100

//*********************************************************************************
BOOL	CFrontEnd::Init(EFrontEndEntry entry,BOOL inLoadedSystem)
{
	CONSOLE.SetLoading(TRUE);

	if (inLoadedSystem)
		CONSOLE.SetLoadingRange( 50.f, 62.5f );
	else
		CONSOLE.SetLoadingRange( 0.f, 25.f );

#if TARGET == XBOX
	// Let's get the sounds for the frontend.
	if (CLIPARAMS.mSound)
		SOUND.LoadXAPFile();
#endif

	EVENT_MANAGER.Init() ;
//	CAREER.Test();

	CAREER.Update() ;

	if (inLoadedSystem)
		CONSOLE.SetLoadingRange( 62.5f, LOADING_STEP_1/2+50.0f );
	else
		CONSOLE.SetLoadingRange( 25.f, LOADING_STEP_1 );

	// Initialise Data
	if (!FED.Init())
		return FALSE;

	if (!FET.Init())
		return FALSE;

	if (inLoadedSystem)
		CONSOLE.SetLoadingRange( LOADING_STEP_1/2+50.0f, LOADING_STEP_2/2+50.0f );
	else
		CONSOLE.SetLoadingRange( LOADING_STEP_1, LOADING_STEP_2 );

	if (!FEV.Init())
		return FALSE;

	// Memory cards
	MEMORYCARD.Initialise();

	// Initialise the messagebox class
	FEMESSBOX.Init();

	if (inLoadedSystem)
		CONSOLE.SetLoadingRange( LOADING_STEP_2/2+50.0f, LOADING_STEP_3/2+50.0f );
	else
		CONSOLE.SetLoadingRange( LOADING_STEP_2, LOADING_STEP_3 );

	// Build pointer list for pages
	SINT	c0;

	// fill with unknown to start with.
	for (c0 = 0; c0 < FEP_NUM_PAGES; c0 ++)
		mPages[c0] = &mFEPUnknown; 

	// and the actual pages
	mPages[FEP_MAIN]				= &mFEPMain;
	
	mPages[FEP_DEVELOPMENT]			= &mFEPDevelopment;

	mPages[FEP_COMMON]				= &mFEPCommon;

	mPages[FEP_BE_CONFIG]			= &mFEPBEConfig;
	mPages[FEP_WINGMEN]				= &mFEPWingmen;
	mPages[FEP_BRIEFING]			= &mFEPBriefing;
	mPages[FEP_DEBRIEFING]			= &mFEPDebriefing;
	mPages[FEP_LEVEL_SELECT]		= &mFEPLevelSelect;
	mPages[FEP_GOODIES]				= &mFEPGoodies;
	mPages[FEP_DEVSELECT]			= &mFEPDevSelect;
	mPages[FEP_LOADGAME]			= &mFEPLoadGame;
	mPages[FEP_SAVEGAME]			= &mFEPSaveGame;
	mPages[FEP_INTRO]				= &mFEPIntro;
	mPages[FEP_MULTIPLAYER]			= &mFEPMultiplayer;
	mPages[FEP_MULTIPLAYER_START]	= &mFEPMultiplayerStart;
	mPages[FEP_OPTIONS]				= &mFEPOptions;
	mPages[FEP_CREDITS]				= &mFEPCredits;
	mPages[FEP_CONTROLLER]			= &mFEPController;

	mPages[FEP_VIRTUAL_KEYBOARD]	= &mFEPVirtualKeyboard;
	mPages[FEP_DIRECTORY]			= &mFEPDirectory;

	mPages[FEP_E3_LEVEL_SELECT]		= &mFEPE3LevelSelect;
	mPages[FEP_LANGUAGE_TEST]		= &mFEPLanguageTest;
	
	mPages[FEP_DEMOMAIN]			= &mFEPDemoMain;

	mPages[FEP_UNKNOWN]				= &mFEPUnknown;

	// Initialise all pages
	BOOL	ok = TRUE;

	for (c0 = 0; c0 < FEP_NUM_PAGES; c0 ++)
	{
		char buf[20];
		sprintf(buf, "FEP %d...", c0);
		TRACE(buf);
		ok &= mPages[c0]->Init();
		TRACE("done.\n");

		CONSOLE.SetLoadingFraction(float(c0) / float(FEP_NUM_PAGES));
	}

	if (!ok)
		return FALSE;

#ifdef E3BUILD
	GAME.mSupervisorMode = FALSE; // for E3
	CAREER.Blank();
#else
	GAME.mSupervisorMode = TRUE; // for E3
#endif

	// Controller Stuff
	for (int port = 0; port < NUM_CONTROLLER_PORTS; port++)
		mControllers[port]=new(MEMTYPE_CONTROLLER) CCONTROLLER(this,port);

	PLATFORM.FlushInputBuffers(); // clear up keystrokes pressed while loading

	// Other Data
	mCounter = 0;
	mQuit = -2;
	mLastRenderTime = -100.f;

	GAME.mRenderFrameNumber=0;

	// start point
	if (CLIPARAMS.mGoStraightToDeviceSelectScreen != -1)
	{
		// We were on the "new career" screen before we went to the xbox dashboard
		mActivePage = FEP_UNKNOWN;
		SetPage(FEP_DEVSELECT, 0);
		FRONTEND.SetSaveMode(1);
		FRONTEND.SetAutoSave(FRONTEND.AUTO_SAVE_NORMAL);
		ASSERT(!CAREER.InProgress());
		mFEPDevSelect.SetCurrentCard(CLIPARAMS.mGoStraightToDeviceSelectScreen);
	}
	else if (mFirstRun || entry == FEE_FROM_ATTRACT)
	{
		mFirstRun = FALSE;

		if (!CLIPARAMS.mDeveloperMode)
		{
			mActivePage = FEP_UNKNOWN;

			if (PLAYABLE_DEMO)
			{
				SetPage(FEP_INTRO, 0);
			}
			else
			{
				SetPage(FEP_INTRO, 0);
			}

//			SetPage(FEP_MAIN, 0);
//			SetPage(FEP_LEVEL_SELECT, 0);
//			SetPage(FEP_BRIEFING, 0);
//			SetPage(FEP_DEBRIEFING, 0);
//			SetPage(FEP_DEBRIEFING, 30);
		}
		else
		{
			mActivePage = FEP_UNKNOWN;

			if (PLAYABLE_DEMO)
			{
				SetPage(FEP_INTRO, 0);
			}
			else
			{
				if (GAME.mSupervisorMode)
					SetPage(FEP_MAIN, 0);
				else
					SetPage(FEP_INTRO, 0);
			}
		}
	}
	else if (entry == FEE_TITLE_SCREEN)
	{
		mActivePage = FEP_UNKNOWN;
		SetPage(FEP_MAIN, 0);
	}
	else
	{
		if ((mSelectedLevel > 900) && (mSelectedLevel < 906)) // Race levels
			mActivePage = FEP_GOODIES;
		else if ((mSelectedLevel >= 850) && (mSelectedLevel < 880))
			mActivePage = FEP_MULTIPLAYER;
		else
		{
			// If it's not the first run, we should autosave.
#ifdef E3BUILD
			mActivePage = FEP_INTRO;
#else
			if (PLAYABLE_DEMO)
			{
//				mActivePage = FEP_DEMOMAIN;
				if (CLIPARAMS.mAttractMode)
					mActivePage=FEP_INTRO;
				else
					mActivePage = FEP_DEBRIEFING;
			}
			else
			{
#if TARGET !=PC
				if ((GetAutoSave() != AUTO_SAVE_PRETEND) && (performing_stress_test == 0))
				{
					// Save the damn thing.
					FRONTEND.SetSaveMode(TRUE);
					mActivePage = FEP_UNKNOWN;
					SetPage(FEP_SAVEGAME, 0);
					SetSuccessFEP(FEP_DEBRIEFING, 50);
				}
				else
				{
					mActivePage = FEP_DEBRIEFING;
				}
#else
				mActivePage = FEP_DEBRIEFING;
#endif
			}
#endif
		}
	}

	if (mActivePage != FEP_TRANSITION)
	{
		mPages[mActivePage]->TransitionNotification(FEP_COMMON);
		mPages[mActivePage]->ActiveNotification(FEP_COMMON);
	}
	else
	{
		mPages[mTransitionTo]->TransitionNotification(FEP_COMMON);
//		mPages[mTransitionTo]->ActiveNotification(FEP_COMMON);
	}

	// Selected Data
	mSelectedLevel = 100;

#if TARGET == XBOX
	char buf[200];
	extern DWORD entry_time;
	sprintf(buf, "Step 1 after %d ms\n", GetTickCount() - entry_time);
	TRACE(buf);
#endif

#if TARGET == XBOX
	sprintf(buf, "Step 2 after %d ms\n", GetTickCount() - entry_time);
	TRACE(buf);
#endif

	// Load and cache all the language Files
	for (c0 = 0; c0 < NUM_LANGUAGES; c0 ++)
	{
		mTextSets[c0].Init();
		mTextSets[c0].Load(c0);
	}

	if (inLoadedSystem)
		CONSOLE.SetLoadingRange( LOADING_STEP_2/2+50.0f, 100.f );
	else
		CONSOLE.SetLoadingRange( LOADING_STEP_2, 100.f );

#if TARGET == XBOX
	sprintf(buf, "Step 3 after %d ms\n", GetTickCount() - entry_time);
	TRACE(buf);

	// let's get the frontend music track in place.
	PLATFORM.CacheFile(MUSIC.GetSong(MUSIC.GetTrackForFrontEnd())->mFilename);

	CONSOLE.SetLoadingFraction(.5f);

	PLATFORM.CacheFile(MUSIC.GetSong(MUSIC.GetTrackForCredits ())->mFilename);
	
	CONSOLE.SetLoadingFraction(1.f);
#endif

	CONSOLE.SetLoading(FALSE);

#ifndef TEXT_RESOURCE_TEST_BUILD
	// Start the music
	if (CLIPARAMS.mMusic)
		MUSIC.PlaySelection(MUS_FRONTEND);
#endif

#if TARGET == XBOX
	sprintf(buf, "Step 4 after %d ms\n", GetTickCount() - entry_time);
	TRACE(buf);
#endif

	return TRUE;
}	

//*********************************************************************************

#define READWRITE(t, saving, file)\
{\
	if (saving)\
	{\
		fwrite(&(t), sizeof(t), 1, file);\
	}\
	else\
	{\
		fread (&(t), sizeof(t), 1, file);\
	}\
}

bool	CFrontEnd::SerialiseState(FILE *file, bool saving)
{
	// only needed for the Xbox reboot!
#if TARGET == XBOX
	// we'll try to share loading and saving code so that I don't get them out of sync.
	READWRITE(mSaveMode                 , saving, file);
	READWRITE(mAutoSave                 , saving, file);
	READWRITE(mMemoryCardNumber         , saving, file);
	READWRITE(mFEPSaveGame.mSaveGameName, saving, file);
	READWRITE(mSelectedLevel            , saving, file);
	READWRITE(END_LEVEL_DATA            , saving, file);
	READWRITE(mPlayer0ControllerPort    , saving, file);

	MEM_MANAGER.Validate();

	if (!saving)
	{
		wcscpy(mFEPVirtualKeyboard.m_strData, mFEPSaveGame.mSaveGameName);
		mFEPDevSelect.SetCurrentCard(mMemoryCardNumber);
	}

#endif
	return true;
}

//*********************************************************************************
void	CFrontEnd::Shutdown()
{	
	// Stop the music
	if (CLIPARAMS.mMusic)
		MUSIC.Stop();

	// Memory cards
	MEMORYCARD.Shutdown();	

	// free up the text
	SINT	c0;
	for (c0 = 0; c0 < NUM_LANGUAGES; c0 ++)
	{
		mTextSets[c0].Shutdown();
	}

	IController::Shutdown();

	// Send final deactivation Notification
	if (mActivePage == FEP_TRANSITION)
	{
		mPages[mTransitionFrom]->DeActiveNotification();
		mPages[mTransitionTo]->DeActiveNotification();
	}
	else
		mPages[mActivePage]->DeActiveNotification();

	// Shutdown all pages
	for (c0 = 0; c0 < FEP_NUM_PAGES; c0 ++)
		mPages[c0]->Shutdown();

	// Shutdown FMV
	FEV.Shutdown();

	FET.Shutdown();

	// Shutdown Data
#if TARGET == XBOX
	LT.BlockUntilIdle();  // make sure we're not still rendering..
#endif
	FED.Shutdown();	

	// Shutdown Controller
	for (int port = 0; port < NUM_CONTROLLER_PORTS; port++)
	{
		delete mControllers[port];
		mControllers[port]=NULL;
	}

	// Shutdown System Stuff
	EVENT_MANAGER.Shutdown();

	CONSOLE.ClearCommandsAndVariables();

	// Clean up memory.
	MEM_MANAGER.SetMerge( TRUE );
	MEM_MANAGER.Cleanup();
}

//*********************************************************************************
int		CFrontEnd::GetPlayer0ControllerPort()
{
	if (mPlayer0ControllerPort == -1) return 0;

	return mPlayer0ControllerPort;
}

//*********************************************************************************
int		CFrontEnd::GetPortFromController(CController *controller)
{
	for (int i = 0; i < NUM_CONTROLLER_PORTS; i++)
	{
		if (controller == mControllers[i]) return i;
	}

	// Not found, what the hell.
	return 0;
}

//*********************************************************************************
int		CFrontEnd::NumControllersPresent()
{
	int retval = 0;

	for (int i = 0; i < NUM_CONTROLLER_PORTS; i++)
	{
		if (mControllers[i]->IsPresent()) retval++;
	}

	return retval;
}

//*********************************************************************************
void	CFrontEnd::ReceiveButtonAction(CController* from_controller, int button, float val)
{
	// We make decisions about which controller is which player.
	// The first person to press anything is player 0.
#if TARGET == PS2
	if (mPlayer0ControllerPort == -1 && button == BUTTON_START)
#else
	if (mPlayer0ControllerPort == -1 && button == BUTTON_FRONTEND_MENU_SELECT)
#endif
	{
		// He can be this.
		mPlayer0ControllerPort = GetPortFromController(from_controller);
	}
	else
	if (mPlayer0ControllerPort != -1)
	{
		// There was a player 0 controller already. It may not be this one,
		// in which case we ignore inputs

		// Unless we're in the multiplayer start screen which understands multiple controllers.
		if (mActivePage != FEP_MULTIPLAYER_START && GetPortFromController(from_controller) != mPlayer0ControllerPort) return;
	}

	// globally remember the current controller.
	mReceivingButtonActionPort = GetPortFromController(from_controller);

#if TARGET==PC
	// SRG temporay allow to save career
	if (button ==  BUTTON_SAVE_CAREER)
	{
		CAREER.Save() ;
	}

	if (button ==  BUTTON_LOAD_CAREER)
	{
		CAREER.Load() ;
	}

	if (button ==  BUTTON_LOG_CAREER)
	{
		CAREER.Log() ;
	}
	// SRG end hack
#endif
	mAllLevelsCheatActive = TRUE;
	if ((button == BUTTON_FRONTEND_CHEAT) &&
		(mActivePage != FEP_TRANSITION))
	{
	mAllLevelsCheatActive = TRUE;
#ifndef E3BUILD
		mAllLevelsCheatActive=!mAllLevelsCheatActive;
		mPages[mActivePage]->ButtonPressed(button, val);
#else
		if (GAME.mSupervisorMode)
		{
			SetPage(FEP_INTRO, 0);
			GAME.mSupervisorMode = FALSE;
		}
		else
		{
			SetPage(FEP_LEVEL_SELECT, 0);
			GAME.mSupervisorMode = TRUE;
			mAllLevelsCheatActive = TRUE;
		}
#endif
	}
	else if (FEMESSBOX.BeingDisplayed())
	{
		// If a message box is being displayed, then redirect the input into that
		FEMESSBOX.ButtonPressed(button, val);
	}
	else
	{
		// Pass Button Action on to page
		if (mActivePage != FEP_TRANSITION)
			mPages[mActivePage]->ButtonPressed(button, val);
	}
}

//*********************************************************************************
void	CFrontEnd::SetLanguage(SINT l)
{
	TEXT_DB.Copy(mTextSets[l]);
}

//*********************************************************************************
void	CFrontEnd::SetPage(EFrontEndPage page, SINT time)
{
//	ASSERT(mActivePage != FEP_TRANSITION); // !JS! I commented this out

	// Shouldn't happen...
	if (mActivePage == FEP_TRANSITION)
		mActivePage = mTransitionFrom;

	if (time == 0)
	{
		mPages[mActivePage]->DeActiveNotification();

		// go straight there
		mPages[page]->TransitionNotification(mActivePage);
		mPages[page]->ActiveNotification(mActivePage);

		mActivePage = page;
	}
	else
	{
		// Setup Transition Parameters
		mTransitionFrom = mActivePage;
		mTransitionTo = page;
		mTransitionCount = 0;
		mTransitionTime = time;
		mActivePage = FEP_TRANSITION;

		mPages[mTransitionTo]->TransitionNotification(mTransitionFrom);
	}
}

//*********************************************************************************
void	CFrontEnd::Process()
{
	mCounter ++;

	// Start / stop profiling - there's no doubt somewhere better for this!
#if TARGET == PS2
#ifdef PROFILE
	if(PLATFORM.KeyOnce(KEYCODE_F9))
	{
		PS2PROFILER.SetProfileEnabled( !PS2PROFILER.GetProfileEnabled() );
		if (PS2PROFILER.GetProfileEnabled())
			TRACE("Profiler on\n");
		else
			TRACE("Profiler off\n");
	}
#endif
#endif

	//System Stuff
	EQuitType q = PLATFORM.Process();

	// We need to check if we have to quit due to user inactivity (will only happen in playable demo.	
	if (CController::InactivityMeansQuitGame())
	{
#if TARGET==PS2
		CLIPARAMS.mAttractMode=TRUE;
#else
		q = QT_QUIT_TO_SYSTEM;
#endif
	}

#if TARGET==PS2
	if (PLATFORM.HasTimeoutExpired())
	{
		q=QT_QUIT_TO_SYSTEM;
	}
#endif

	if (q != QT_NONE)
		mQuit = -1;

	EVENT_MANAGER.Update() ;

	PARTICLE_MANAGER.Process(1.f, TRUE);
	SOUND.UpdateStatus();
	MUSIC.UpdateStatus();

	// Update controller
	for (int port = 0; port < NUM_CONTROLLER_PORTS; port++)
		mControllers[port]->Flush();

	// Update Video, but don't wait for the next frame of the video
#if TARGET!=PS2
	FEV.NextFrame(false);
#endif

	// We need to process insertion and removal of controllers
	if (mPlayer0ControllerPort != -1)
	{
		// As soon as someone has control, he's stuck with it. So you can't pull it out and then expect to
		// control the frontend with another controller. This is because I don't want to silently
		// not mind about controller unplugging.
		if (!RECONNECT_INTERFACE[0].Process(mControllers[mPlayer0ControllerPort], false) && RECONNECT_INTERFACE[0].GetState() != CReconnectInterface::OK)
		{
			// we're still unconnected. So no frontend processing thanks.
			return;
		}
	}

	// deal with transitions
	if (mActivePage == FEP_TRANSITION)
	{
		mTransitionCount ++;

		if (mTransitionCount == mTransitionTime)
		{
			mPages[mTransitionFrom]->DeActiveNotification();
			mActivePage = mTransitionTo;
			mPages[mActivePage]->ActiveNotification(mTransitionFrom);
		}
	}

	// process pages
	SINT	c0;

	for (c0 = 0; c0 < FEP_NUM_PAGES; c0 ++)
	{
		EFEPState state = FEPS_INACTIVE;

		if (mActivePage == c0)
			state = FEPS_ACTIVE;
		else if (mActivePage == FEP_TRANSITION)
		{
			if (c0 == mTransitionFrom)
				state = FEPS_TRANSITIONING_FROM;
			if (c0 == mTransitionTo)
				state = FEPS_TRANSITIONING_TO;
		}

		mPages[c0]->Process(state);
	}

	// Process the messagebox
	FEMESSBOX.Process();

	// Keyboard Mappings
	//!! Definitly shouldn't be here.  the Controller stuff is shit...

//	if (PLATFORM.KeyOnce(KEYCODE_DOWN))
//		ReceiveButtonAction(BUTTON_FRONTEND_MENU_DOWN, 0);

//	if (PLATFORM.KeyOnce(KEYCODE_UP))
//		ReceiveButtonAction(BUTTON_FRONTEND_MENU_UP, 0);

//	if (PLATFORM.KeyOnce(KEYCODE_RETURN))
//		ReceiveButtonAction(BUTTON_FRONTEND_MENU_SELECT, 0);

}

//*********************************************************************************
#define	LINE_SPRITE_SIZE	32.f

void	CFrontEnd::DrawLine(float sx, float sy, float ex, float ey, DWORD col, float width, float depth, float perc)
{
	// essentially a dodgy line draw
	float	dx = ex - sx;
	float	dy = ey - sy;

	float	cx = (sx + ex) / 2;
	float	cy = (sy + ey) / 2;

	float	yaw = atan2f(dy, dx);

	float	xscale = sqrtf(dx * dx + dy * dy) / LINE_SPRITE_SIZE * perc;
	float	yscale = width / LINE_SPRITE_SIZE / 2.f;

	RENDERINFO.SetFogEnabled(FALSE);
	CSPRITERENDERER::DrawColouredSprite(cx, cy, depth, FED.GetTexture(FET_LEVEL_LINK),col, xscale, yscale, SA_CENTRE, SPT_NONE, 1.f, yaw);
}

//*********************************************************************************

void	CFrontEnd::DrawBox(float tlx, float tly, float brx, float bry, DWORD col, float width, float depth)
{
	DrawLine( tlx, tly, brx, tly, col, width, depth, 1.0f );
	DrawLine( brx, tly, brx, bry, col, width, depth, 1.0f );
	DrawLine( brx, bry, tlx, bry, col, width, depth, 1.0f );
	DrawLine( tlx, bry, tlx, tly, col, width, depth, 1.0f );
}

//*********************************************************************************

#define BLANK_PANEL_SIZE	( 16.0f )

void	CFrontEnd::DrawPanel(float tlx, float tly, float brx, float bry, float z, DWORD col)
{
	RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_CLAMP);
	RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_CLAMP);
	RENDERINFO.SetFogEnabled(FALSE);

	CSPRITERENDERER::DrawColouredSprite(tlx, tly, z, FED.GetTexture(FET2_BLANK), col, ((brx - tlx) / BLANK_PANEL_SIZE), ((bry - tly) / BLANK_PANEL_SIZE), SA_TOP_LEFT);

	RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_WRAP);
	RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_WRAP);
}

//*********************************************************************************

void	CFrontEnd::DrawBarGraph(float tlx, float tly, float brx, float bry, float num, float max, float z, SINT bordercol, SINT backcol, SINT forecol)
{
	// Draw background
	DrawPanel(tlx, tly, brx, bry, z, backcol);

	// Draw bar
	if (num)
	{
		DrawPanel(tlx, tly, tlx + ((brx - tlx) * (num / max)), bry, z, forecol);
	}

}
//*********************************************************************************

// does this page have the curve on either side?
static BOOL got_standard_SlidingTextBordersAndMask(EFrontEndPage dest)
{
	switch(dest)
	{
	case FEP_DEVSELECT:
	case FEP_DIRECTORY:
	case FEP_GOODIES:
	case FEP_LEVEL_SELECT:
	case FEP_LOADGAME:
	case FEP_MULTIPLAYER:
//	case FEP_MULTIPLAYER_START:
	case FEP_SAVEGAME:
	case FEP_VIRTUAL_KEYBOARD:
	case FEP_CONTROLLER:
		return TRUE;

	default:
		return FALSE;
	}
}

BOOL	check_draw_twice(EFrontEndPage cp, EFrontEndPage dest)
{
	if (got_standard_SlidingTextBordersAndMask(cp) && got_standard_SlidingTextBordersAndMask(dest))
		return SINT(dest) > SINT(cp); // cheeky!
	else
		return FALSE;
}

void CFrontEnd::DrawSlidingTextBordersAndMask(float transition, EFrontEndPage dest)
{
	EFrontEndPage from = FEP_UNKNOWN;

	// make sure we don't draw it twice...
	if (GetCurrentPage() == FEP_TRANSITION)
	{
		from = GetTransitionTo() == dest ? GetTransitionFrom(): GetTransitionTo();
		if (check_draw_twice(from, dest))
			return;
	}

	float tr, scale, yaw;
	SINT alpha;
	DWORD	col;

	RENDERINFO.SetFogEnabled(false);

	//*************************************************************************
	// Sliding Text
	if ((dest != FEP_BRIEFING) && (dest != FEP_DEBRIEFING) && (dest != FEP_MULTIPLAYER_START))
		tr = RangeTransition(transition, 0.75f, 1.f);
	else
		tr = 1.f;

	FRONTEND.GetCommonPage()->RenderSmallForsetiText(tr);

	//*************************************************************************
	// Metal Borders
#define SELECT_BRACKET_X	328
#define SELECT_BRACKET_Y	343
#define	SELECT_BRACKET_SCALE	1.25f
#define	SELECT_BRACKET_SCALE2	1.4f

	if (got_standard_SlidingTextBordersAndMask(dest) && (from != FEP_VIRTUAL_KEYBOARD))
		transition = 1.f;
/*	else
	if ((dest == FEP_BRIEFING) || (dest == FEP_DEBRIEFING))
		tr = RangeTransition(transition, 0.25f, 1.f);
	else
		tr = RangeTransition(transition, 0.25f, 0.75f);*/

	// Draw inside bracket
	col = 0xffffffff;
	alpha = 0xff;
	scale = (GetCurrentPage() == FEP_VIRTUAL_KEYBOARD ? SELECT_BRACKET_SCALE2 : SELECT_BRACKET_SCALE);
	yaw = 0.f;
	if ((transition < 1.f) && (dest == FEP_MAIN))
	{
		if (transition < 0.2f)
		{
			alpha = 0;
		}
		if (transition < 0.5f)
		{
			tr = RangeTransition(transition, 0.2f, 0.5f);
			alpha = MakeAlpha(tr);
			scale = tr * 1.f;
			yaw = -(tr * 0.3f);
		}
		else if (transition < 0.6f)
		{
			scale = 1.f;
		}
		else if (transition < 0.7f)
		{
			tr = RangeTransition(transition, 0.6f, 0.7f);
			scale = Range(tr, 1.f, SELECT_BRACKET_SCALE);
		}
	}
	else if ((transition < 1.f) && (from == FEP_VIRTUAL_KEYBOARD))
	{
		scale = CosResize(transition, SELECT_BRACKET_SCALE, SELECT_BRACKET_SCALE2);
	}
	else if (transition < 1.f)
	{
		tr = 1.f - RangeTransition(transition, 0.5f, 1.f);
		alpha = MakeAlpha(1.f - tr);
		scale = SELECT_BRACKET_SCALE + (tr * 1.4f);
		yaw = -tr * 1.0f;
	}

	BlendAlpha(col, alpha);

	CSPRITERENDERER::DrawColouredSprite(SELECT_BRACKET_X + 5 + FRONTEND.GetShadowOffsetX(), SELECT_BRACKET_Y + 10 + FRONTEND.GetShadowOffsetY(), FO(350), FED.GetTexture(FET3_SELECT_BRACKET1), BlendAlpha2(0x3F000000, alpha), scale * 1.05f, scale * 1.05f, SA_CENTRE, SPT_NONE, 1.f, yaw);
	CSPRITERENDERER::DrawColouredSprite(SELECT_BRACKET_X, SELECT_BRACKET_Y, FO(200), FED.GetTexture(FET3_SELECT_BRACKET1), col, scale, scale, SA_CENTRE, SPT_NONE, 1.f, yaw);

	float savescale = scale;

	// Draw outside bracket
	col = 0xffffffff;
	alpha = 0xff;
	scale = 1.f;
	yaw = 0.f;
	BOOL draw = FALSE;
	if ((transition < 1.f) && (dest == FEP_MAIN))
	{
		if (transition < 0.2f)
		{
		}
		if (transition < 0.5f)
		{
			tr = RangeTransition(transition, 0.2f, 0.5f);
			alpha = MakeAlpha(tr);
			scale = tr * 1.f;
			yaw = -(tr * 0.3f);
			draw = TRUE;
		}
		else if (transition < 0.7f)
		{
			scale = 1.f;
			draw = TRUE;
		}
		else if (transition < 0.9f)
		{
			tr = 1.f - RangeTransition(transition, 0.7f, 0.9f);
			alpha = MakeAlpha(tr);
			scale = tr * 1.f;
			yaw = -(tr * 0.3f);
			draw = TRUE;
		}
	}

	BlendAlpha(col, alpha);

	if (draw)
	{
		CSPRITERENDERER::DrawColouredSprite(SELECT_BRACKET_X + 5 + FRONTEND.GetShadowOffsetX(), SELECT_BRACKET_Y + 10 + FRONTEND.GetShadowOffsetY(), FO(350), FED.GetTexture(FET3_SELECT_BRACKET2), BlendAlpha2(0x3F000000, alpha), scale * 1.05f, scale * 1.05f, SA_CENTRE, SPT_NONE, 1.f, yaw);
		CSPRITERENDERER::DrawColouredSprite(SELECT_BRACKET_X, SELECT_BRACKET_Y, FO(200), FED.GetTexture(FET3_SELECT_BRACKET2), col, scale, scale, SA_CENTRE, SPT_NONE, 1.f, yaw);
	}




/*	if (got_standard_SlidingTextBordersAndMask(dest))
		tr = 1.f;
	else
	if ((dest == FEP_BRIEFING) || (dest == FEP_DEBRIEFING))
		tr = RangeTransition(transition, 0.25f, 1.f);
	else
		tr = RangeTransition(transition, 0.25f, 0.75f);

	if (tr > 0)
	{
		if ((dest == FEP_BRIEFING) || (dest == FEP_DEBRIEFING))
		{
			tr = 2.f - tr;
			scale = 1.f + ((tr - 1.f) * 1.f);
		}
		else
		{
			scale = (tr / 2.f) + 0.5f;
		}

		alpha = MakeAlpha(tr);

		float	yaw = (1.f - tr) * (PI / 2.f);

		x = LEVEL_SELECT_LEFT_X;
		y = LEVEL_SELECT_LEFT_Y;

		if ((dest == FEP_BRIEFING) || (dest == FEP_DEBRIEFING))
			Rotate2D(x, y, LEVEL_ROT_POINT2_X, LEVEL_ROT_POINT2_Y, yaw, scale);
		else
			Rotate2D(x, y, LEVEL_ROT_POINT_X, LEVEL_ROT_POINT_Y, yaw, scale);

//		col = BlendAlpha2(0x3f000000, alpha);
//		CSPRITERENDERER::DrawColouredSprite(x + 5 + FRONTEND.GetShadowOffsetX(), y + 10 + FRONTEND.GetShadowOffsetY(), FO(200), FED.GetTexture(FET2_LEVEL_SELECT_LEFT), col, scale * 1.05f, scale * 1.05f, SA_CENTRE, SPT_NONE, 1.f, yaw);
		col = BlendAlpha2(0xffffffff, alpha);
		CSPRITERENDERER::DrawColouredSprite(x, y, FO(100), FED.GetTexture(FET2_LEVEL_SELECT_LEFT), col, scale, scale, SA_CENTRE, SPT_NONE, 1.f, yaw);
		
		x = LEVEL_SELECT_RIGHT_X;
		y = LEVEL_SELECT_RIGHT_Y;

		if ((dest == FEP_BRIEFING) || (dest == FEP_DEBRIEFING))
			Rotate2D(x, y, LEVEL_ROT_POINT2_X, LEVEL_ROT_POINT2_Y, yaw, scale);
		else
			Rotate2D(x, y, LEVEL_ROT_POINT_X, LEVEL_ROT_POINT_Y, yaw, scale);

//		col = BlendAlpha2(0x3f000000, alpha);
//		CSPRITERENDERER::DrawColouredSprite(x + 5 + FRONTEND.GetShadowOffsetX(), y + 10 + FRONTEND.GetShadowOffsetY(), FO(200), FED.GetTexture(FET2_LEVEL_SELECT_RIGHT), col, scale * 1.05f, scale * 1.05f, SA_CENTRE, SPT_NONE, 1.f, yaw);
		col = BlendAlpha2(0xffffffff, alpha);
		CSPRITERENDERER::DrawColouredSprite(x, y, FO(100), FED.GetTexture(FET2_LEVEL_SELECT_RIGHT), col, scale, scale, SA_CENTRE, SPT_NONE, 1.f, yaw);
	}
*/
	// masks?
	if (transition > 0.75f || (from == FEP_VIRTUAL_KEYBOARD))
	{
		RENDERINFO.SRS(RS_SRCBLEND,	BLEND_ZERO);
		RENDERINFO.SRS(RS_DESTBLEND, BLEND_ONE);
		RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_CLAMP);
		RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_CLAMP);
#if TARGET==PS2		
		RENDERINFO.SRS(RS_TEXCLAMPENABLE,TRUE);
#endif
		RENDERINFO.Apply();

//		CSPRITERENDERER::DrawColouredSprite(LEVEL_SELECT_LEFT_X , LEVEL_SELECT_LEFT_Y , FO(100), FED.GetTexture(FET2_LEVEL_SELECT_LEFT_MASK ), 0xffffffff, 1.f, 1.f, SA_CENTRE);
//		CSPRITERENDERER::DrawColouredSprite(LEVEL_SELECT_RIGHT_X, LEVEL_SELECT_RIGHT_Y, FO(100), FED.GetTexture(FET2_LEVEL_SELECT_RIGHT_MASK), 0xffffffff, 1.f, 1.f, SA_CENTRE);
		CSPRITERENDERER::DrawColouredSprite(SELECT_BRACKET_X, SELECT_BRACKET_Y, FO(100), FED.GetTexture(FET3_SELECT_BRACKET_MASK), 0xffffffff, savescale, savescale, SA_CENTRE);

		// cover left hand edge (doesn't quite reach....
		CSPRITERENDERER::DrawColouredSprite(10, 240, FO(100), FED.GetTexture(FET2_WHITE), 0xffffffff, 2.f, 15.f, SA_CENTRE);
		
		RENDERINFO.SRS(RS_SRCBLEND,	BLEND_SRCALPHA);		
		RENDERINFO.SRS(RS_DESTBLEND,BLEND_INVSRCALPHA);	
		RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_WRAP);
		RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_WRAP);
#if TARGET==PS2
		RENDERINFO.SRS(RS_TEXCLAMPENABLE,FALSE);
#endif
		RENDERINFO.Apply();
	}
}

//*********************************************************************************
void	CFrontEnd::DrawStandardVideoBackground(float transition, DWORD colour, EFrontEndPage dest)
{
	// We deliberately start playing the video early because really it's loading, not playing, for the first bit.
/*#if _DIRECTX
	if (transition > 0.5f)
#else
	if (transition > 0.75f)
#endif
*/	{
		// note that, amusingly, on directx machines, though the video is playing from .5, it's only
		// visible from .75 as on non-directx. This is because the video async-loads and therefore
		// needs time before it's visible.
//		SINT	alpha = SINT(RangeTransition(transition, 0.75f, 1.f) * 255.f);
		SINT	alpha;
		
		if (dest == FEP_MAIN)
			alpha = MakeAlpha(RangeTransition(transition, 0.f, 0.5f));
		else
			alpha = MakeAlpha(RangeTransition(transition, 0.5f, 1.f));

		FRONTEND.GetCommonPage()->RenderVideoBackground(1.f, BlendAlpha2(colour, alpha));
	}
}

//*********************************************************************************
void	CFrontEnd::RenderSlidingScreen(CTEXTURE *screen,int alpha, float offy, DWORD col)
{
	if (!screen)
	{
		PLATFORM.Font( FONT_DEBUG )->DrawText(16,16,0xFFFFFFFF,ToWCHAR("Frontend screen not found..."));
		return;
	}
	
	RENDERINFO.STS(0,TSS_MINFILTER,TEXF_POINT);
	RENDERINFO.STS(0,TSS_MAGFILTER,TEXF_POINT);
	
	RENDERINFO.STS(0,TSS_ADDRESSU,TADDRESS_WRAP);
	RENDERINFO.STS(0,TSS_ADDRESSV,TADDRESS_WRAP);	
	
	RENDERINFO.SRS(RS_ALPHATESTENABLE,FALSE);

	CSPRITERENDERER::DrawColouredSprite(0,0, FO(999),screen,(alpha<<24) | (col & 0x00ffffff), float(PLATFORM.GetScreenWidth()) / screen->GetWidth() ,1.0f, SA_TOP_LEFT, SPT_SCROLL_DOWN, offy);

	RENDERINFO.STS(0,TSS_MINFILTER,TEXF_LINEAR);
	RENDERINFO.STS(0,TSS_MAGFILTER,TEXF_LINEAR);
}

//*********************************************************************************
#define HEADER_BAR_SIZE	64

void	CFrontEnd::DrawBar(float sx, float sy, float z, SINT segs, DWORD col, float scale)
{
	RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_CLAMP);
	RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_CLAMP);

	float y = sy - ((HEADER_BAR_SIZE / 2) * scale);
	float x = sx - (((HEADER_BAR_SIZE / 2) * (segs + 2)) * scale);

	SINT	c0;
	for (c0 = 0; c0 < segs + 2; c0 ++)
	{
		CTEXTURE *tex = FED.GetTexture(FET_BARC);

		if (c0 == 0)		tex = FED.GetTexture(FET_BARL);
		if (c0 == segs + 1) tex = FED.GetTexture(FET_BARR);

		CSPRITERENDERER::DrawColouredSprite(x, y, z, tex,col, scale, scale, SA_TOP_LEFT);

		x += (HEADER_BAR_SIZE) * scale;
	}

	RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_WRAP);
	RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_WRAP);
}

//*********************************************************************************

//#define	TITLE_TEXT_SCALE_X	0.8f
#define	TITLE_TEXT_SCALE_X	1.f

#define HEADER_BAR_X		390.f
#define HEADER_BAR_Y		69.f
#define	HEADER_BAR_SCALE	1.25f

void	CFrontEnd::DrawTitleBar(WCHAR *text, float transition, EFrontEndPage dest)
{
	DWORD	col;
	SINT	alpha = 0xff;
	float	tr;
	float	scale, yaw;
	float	yo = 0;

	RENDERINFO.SetFogEnabled(false);

	if ((transition < 1.f) && (dest != FEP_MAIN))
	{
		if (transition < 0.5f)
			return;

		yo = -sinf(transition * PI) * 200.f;
	}

	// Draw Text Box
	scale = 1.f;
	col = 0x7f000000;
	yaw = 0.f;
	tr = RangeTransition(transition, 0.8f, 1.f);
	alpha = MakeAlpha(tr);
	BlendAlpha(col, alpha);


	CSPRITERENDERER::DrawColouredSprite(HEADER_BAR_X, HEADER_BAR_Y + yo + 10, FO(330), FED.GetTexture(FET3_HEADER_TEXT_BOX), col, scale, scale, SA_CENTRE, SPT_NONE, 1.f, yaw);


	//*************************************************************************
	// Options Brackets #1

	// Draw inside bracket
	col = 0xffffffff;
	alpha = 0xff;
	scale = HEADER_BAR_SCALE;
	yaw = 0.f;
	if ((transition < 1.f) && (dest == FEP_MAIN))
	{
		if (transition < 0.3f)
			alpha = 0;
		else if (transition < 0.7f)
		{
			tr = RangeTransition(transition, 0.3f, 0.7f);
			alpha = MakeAlpha(tr);
			scale = tr * 1.f;
			yaw = -((1.f - tr) * 0.3f);
		}
		else if (transition < 0.8f)
		{
			scale = 1.f;
		}
		else if (transition < 1.f)
		{
			tr = RangeTransition(transition, 0.8f, 1.f);
			scale = Range(tr, 1.f, HEADER_BAR_SCALE);
		}
	}

	BlendAlpha(col, alpha);

	CSPRITERENDERER::DrawColouredSprite(HEADER_BAR_X + 5 + FRONTEND.GetShadowOffsetX(), HEADER_BAR_Y + yo + 5 + FRONTEND.GetShadowOffsetX(), FO(350), FED.GetTexture(FET3_HEADER_BRACKET1), BlendAlpha2(0x3F000000, alpha), scale * 1.05f, scale * 1.05f, SA_CENTRE, SPT_NONE, 1.f, yaw);
	CSPRITERENDERER::DrawColouredSprite(HEADER_BAR_X, HEADER_BAR_Y + yo, FO(300), FED.GetTexture(FET3_HEADER_BRACKET1), col, scale, scale, SA_CENTRE, SPT_NONE, 1.f, yaw);

	// Draw outside bracket
	col = 0xffffffff;
	alpha = 0xff;
	scale = 1.f;
	yaw = 0.f;
	BOOL	draw = FALSE;
	if ((transition < 1.f) && (dest == FEP_MAIN))
	{
		if (transition < 0.3f)
		{
		}
		else if (transition < 0.7f)
		{
			tr = RangeTransition(transition, 0.3f, 0.7f);
			alpha = MakeAlpha(tr);
			scale = tr * 1.f;
			yaw = -((1.f - tr) * 0.3f);
			draw = TRUE;
		}
		else if (transition < 0.9f)
		{
			scale = 1.f;
			draw = TRUE;
		}
		else if (transition < 1.f)
		{
			tr = 1.f - RangeTransition(transition, 0.9f, 1.f);
			alpha = MakeAlpha(tr);
			scale = tr * 1.f;
			yaw = -((1.f - tr) * 0.3f);
			draw = TRUE;
		}
	}

	BlendAlpha(col, alpha);

	if (draw)
	{
		CSPRITERENDERER::DrawColouredSprite(HEADER_BAR_X + 5 + FRONTEND.GetShadowOffsetX(), HEADER_BAR_Y + yo + 5 + FRONTEND.GetShadowOffsetX(), FO(350), FED.GetTexture(FET3_HEADER_BRACKET2), BlendAlpha2(0x3F000000, alpha), scale * 1.05f, scale * 1.05f, SA_CENTRE, SPT_NONE, 1.f, yaw);
		CSPRITERENDERER::DrawColouredSprite(HEADER_BAR_X, HEADER_BAR_Y + yo, FO(300), FED.GetTexture(FET3_HEADER_BRACKET2), col, scale, scale, SA_CENTRE, SPT_NONE, 1.f, yaw);
	}


	col = 0xff7f7f7f;
	if (dest == FEP_MAIN)
	{
		tr = RangeTransition(transition, 0.9f, 1.f);
		alpha = MakeAlpha(tr);
		BlendAlpha(col, alpha);
	}

	SIZE	s;
	PLATFORM.Font( FONT_NORMAL )->GetTextExtent(text, &s);
	PLATFORM.Font( FONT_NORMAL )->DrawTextDynamic(HEADER_BAR_X - s.cx  * 0.5f * TITLE_TEXT_SCALE_X, HEADER_BAR_Y + yo - 4.f, FO(100), TITLE_TEXT_SCALE_X, 1.f, col, text, 1000.f, FALSE, FONT_USE_Z_BUFFER);

	RENDERINFO.SetFogEnabled(false);
}

//*********************************************************************************
void	CFrontEnd::EnableAdditiveAlpha()
{
	RENDERINFO.SRS(RS_SRCBLEND,   BLEND_ONE );
	RENDERINFO.SRS(RS_DESTBLEND,  BLEND_ONE );
}

//*********************************************************************************
void	CFrontEnd::EnableModulateAlpha()
{
	RENDERINFO.SRS(RS_SRCBLEND,   BLEND_SRCALPHA );
	RENDERINFO.SRS(RS_DESTBLEND,  BLEND_INVSRCALPHA );
}

//*********************************************************************************

void	CFrontEnd::ForceDisplay()
{
	Render(TRUE);
#if TARGET == PS2
	PLATFORM.Flip();
	Render(TRUE);
	PLATFORM.Flip();
	Render(TRUE);
	PLATFORM.Flip();
#endif
}

//*********************************************************************************
BOOL	CFrontEnd::Render(BOOL forcerender)
{
	if ((PLATFORM.GetSysTimeFloat() - mLastRenderTime < (1.f / 60.f)) && (!forcerender))
		return FALSE;

	// A bit naughty, but necessary otherwise the pose caching never thinks the frame has changed
	GAME.mRenderFrameNumber++;

	// jcl - Sorry..
	RENDERINFO.STS(0,TSS_COLOROP,TOP_MODULATE2X);
	RENDERINFO.Apply();

	RENDERINFO.SRS( RS_ZENABLE, TRUE );
	RENDERINFO.SRS( RS_ZWRITEENABLE, TRUE );
	RENDERINFO.SRS( RS_ZFUNC, ZFUNC_LEQUAL );

#if TARGET == PS2
	CPS2Texture::UpdateTextureLODs(FALSE);
#endif

	mLastRenderTime = PLATFORM.GetSysTimeFloat();

	BOOL	started = FRONTEND.RenderStart();
	
	// We're using a virtual 640x480 screen

	RENDERINFO.SetVirtualScreenXSize(640);
	RENDERINFO.SetVirtualScreenYSize(480);
	RENDERINFO.SetVirtualScreenEnabled(true);

	if (started)
	{
		float trans = float(mTransitionCount) / float(mTransitionTime);
		
		// render pages pre common
		if (mActivePage == FEP_TRANSITION)
		{
			// render highest numbered page first
			if (mTransitionTo > mTransitionFrom)
			{
				mPages[mTransitionTo]->RenderPreCommon(trans, mTransitionFrom);
				mPages[mTransitionFrom]->RenderPreCommon(1.f - trans, mTransitionTo);
			}
			else
			{
				mPages[mTransitionFrom]->RenderPreCommon(1.f - trans, mTransitionTo);
				mPages[mTransitionTo]->RenderPreCommon(trans, mTransitionFrom);
			}
		}
		else
		{
			mPages[mActivePage]->RenderPreCommon(1.f, FEP_NONE);
		}

		// render common page
		mPages[FEP_COMMON]->Render(trans, FEP_NONE);

		// render pages
		if (mActivePage == FEP_TRANSITION)
		{
			// render highest numbered page first
			if (mTransitionTo > mTransitionFrom)
			{
				mPages[mTransitionTo]->Render(trans, mTransitionFrom);
				mPages[mTransitionFrom]->Render(1.f - trans, mTransitionTo);
			}
			else
			{
				mPages[mTransitionFrom]->Render(1.f - trans, mTransitionTo);
				mPages[mTransitionTo]->Render(trans, mTransitionFrom);
			}
		}
		else
		{
			mPages[mActivePage]->Render(1.f, FEP_NONE);
		}

		// render more common page stuff
		mFEPCommon.RenderAfter(1.f, FEP_NONE);

		// Render the message box
		FEMESSBOX.Render();
	}

	// maybe stick some memory output here.
	// draw memory manager
	if (ENGINE.GetDrawDebugStuff() & DRAW_MEM_MANAGER)
	{
		MEM_MANAGER.PrintStats() ;	
	}

	if (mPlayer0ControllerPort != -1)
	{
		RECONNECT_INTERFACE[0].Render(NULL, mControllers[mPlayer0ControllerPort]);
	}

	FRONTEND.RenderEnd(started);

	RENDERINFO.SetVirtualScreenEnabled(false);

	return started;
}

//*********************************************************************************

// save out the resource for the whole frontend
#ifdef RESBUILDER
static void build_fe_resource(char *strPlatform, int iPlatform)
{
	CResourceAccumulator lAccumulator;

	// save out the rest of the frontend instead.
	MEMORYCARD.AccumulateResources(&lAccumulator,0);
	FED.AccumulateResources(&lAccumulator);

	// and write to disk.
	lAccumulator.WriteResources(-2, iPlatform);
}

static void build_fe_resources(char *strPlatform, int iPlatform)
{
	CONSOLE.StatusProgress("Building resource files",strPlatform);

	// note that we're not allowed mesh compression in the front end, if the mesh is here, it's probably
	// really important and people look at it for ages.
	extern bool allowed_to_compress;
	bool old_allowed = allowed_to_compress;
	allowed_to_compress = false;

	// build the resources for the frontend
	build_fe_resource(strPlatform, iPlatform);

	// and then all the goodies too.
	if (CLIPARAMS.mBuildGoodies)
		FRONTEND.mFEPGoodies.BuildAllResources(iPlatform);

	allowed_to_compress = old_allowed;
}
#endif
//*********************************************************************************
SINT	CFrontEnd::Run(EFrontEndEntry entry,BOOL inLoadedSystem)
{
	if (!Init(entry,inLoadedSystem))
		return -1;	

#if TARGET == PC
#ifdef RESBUILDER
	if (CLIPARAMS.mBuildResources)
	{
		// Build Resources for Frontend
		CONSOLE.SetLoading(TRUE);

		// We do the memory card accumulation here because it only wants to have its
		// data in the frontend resources anyway.
		
		if (CLIPARAMS.mBuildPCResources)
			build_fe_resources("PC", PC);

		if (CLIPARAMS.mBuildPS2Resources)
			build_fe_resources("PS2", PS2);

		if (CLIPARAMS.mBuildXBOXResources)
			build_fe_resources("XBOX", XBOX);


		CONSOLE.SetLoading(FALSE);

		mQuit = -1;
	}
#endif
#endif	

	while (mQuit == -2)
	{
		Process();

		if (mQuit == -2)
		{
			while( !Render() );
		}

		//JCL - Stress Test
		if (performing_stress_test > 0)
			if (performing_stress_test == 3)
			{
				if (mCounter > 60)
					mQuit = -3;
			}
			else
			{
				if (mCounter > 2)
					mQuit = -3;
			}
	}

	Shutdown();

	return mQuit;
}

//*********************************************************************************
void	CFrontEnd::UpdateCamera()
{
	FMatrix	matview;
	FVector	campos = FED.mCamera.GetPos();
	ENGINE.GetViewMatrixFromCamera(&FED.mCamera, &matview);
	RENDERINFO.SetView(&matview,&campos);
}

//*********************************************************************************
BOOL	CFrontEnd::RenderStart()
{
	BOOL beginsceneok=FALSE;

#if TARGET==PS2
	BOOL donebeginscene=PLATFORM.BeginScene();
	while (!donebeginscene)
	{
		PLATFORM.SleepWaitingForVSync(10);
		donebeginscene=PLATFORM.BeginScene();
	}
	beginsceneok=TRUE;
#else
	beginsceneok=PLATFORM.BeginScene();
#endif

	if (beginsceneok)
	{
#if TARGET == PS2
		RENDERINFO.SRS(RS_ZWRITEENABLE,FALSE);
		CSPRITERENDERER::DrawColouredSprite( 0, 0, FO(999), FED.GetTexture(FET2_WHITE), 0xff1f1f3f, 640/(float)FED.GetTexture(FET2_WHITE)->GetWidth(), 480/(float)FED.GetTexture(FET2_WHITE)->GetHeight(), SA_TOP_LEFT);
		RENDERINFO.SRS(RS_ZWRITEENABLE,TRUE);

		CViewport viewport;
		
		viewport.Width=PLATFORM.GetWindowWidth();
		viewport.Height=PLATFORM.GetWindowHeight();
		viewport.X=0;
		viewport.Y=0;
		viewport.MinZ=0.0f;
		viewport.MaxZ=1.0f;
		
		PLATFORM.SetViewport(&viewport);
#endif		

		float howfar = 700, zoom = 1.f;
		RENDERINFO.SetProjection(ENGINE.GetNearZ(),howfar,ENGINE.GetNearZ()*zoom,ENGINE.GetNearZ()*zoom*0.75f);

		UpdateCamera();

		CMESHRENDERER::SetRenderMode(R_Normal);

		RENDERINFO.SetWorld(ZERO_FVECTOR, ID_FMATRIX);

		ENGINE.SetCamera(&FED.mCamera);

		RENDERINFO.SetFogEnabled(FALSE);

		RENDERINFO.SetReorder( FALSE );

		RENDERINFO.Apply();

		return TRUE;
	}
	else
		return(FALSE);
}

//*********************************************************************************
void	CFrontEnd::RenderEnd(BOOL started)
{
	if (started)
	{
		RENDERINFO.SetReorder( TRUE );

		PLATFORM.EndScene();
	}
	
	PLATFORM.Flip();


#if TARGET == XBOX
	static bool arrived_once = false;

	if (!arrived_once)
	{
		char buf[200];
		extern DWORD entry_time;
		sprintf(buf, "Intro page arrived after %d ms\n", GetTickCount() - entry_time);
		TRACE(buf);
		arrived_once = true;
	}
#endif
}

//*********************************************************************************
#define	SHADOW_PERIOD	100.f
#define	SHADOW_RADIUS_X	6.f
#define	SHADOW_RADIUS_Y	3.f

float	CFrontEnd::GetShadowOffsetX()
{
	return 	sinf(float(FRONTEND.GetCounter()) / SHADOW_PERIOD) * SHADOW_RADIUS_X;
}

//*********************************************************************************
float	CFrontEnd::GetShadowOffsetY()
{
	return 	cosf(float(FRONTEND.GetCounter()) / SHADOW_PERIOD) * SHADOW_RADIUS_Y;
}

//*********************************************************************************
void	CFrontEnd::StartMCOperation(WCHAR *message)
{
	SOUND.PauseAllSamples();
	
	FEMESSBOX.Kill();					
	FEMESSBOX.Create(320.0f,240.0f,300.0f,FO(100),message,PLATFORM.Font(FONT_SMALL), 0, 0, FALSE, FEMB_OPTION_NONE, FALSE);
	ForceDisplay();
	
	mTimerStart=PLATFORM.GetSysTimeFloat();
}

//*********************************************************************************
void	CFrontEnd::EndMCOperation()
{
	// Wait for 3 seconds to expire to be TRC-compliant

	while (PLATFORM.GetSysTimeFloat()<(mTimerStart+3))
	{
		PLATFORM.Process();
#if TARGET==PS2
		PLATFORM.Sleep(100);
#else
		Sleep(100);
#endif
	}

	SOUND.UnPauseAllSamples();

	FEMESSBOX.Kill();
}

//*********************************************************************************
void	CFrontEnd::PlaySound(EFrontEndSound sound)
{
	char	*sample = GetSoundName(sound);

	CEffect	*effect = SOUND.GetEffectByName(sample);

	SOUND.PlayEffect(effect, NULL);

#if TARGET==PS2
	SOUND.HandleStreams(); // To ensure the sound gets played
#endif
}

//*********************************************************************************
char	*CFrontEnd::GetSoundName(EFrontEndSound sound)
{
	switch (sound)
	{
	case FES_MOVE:
		return "Front End Move";

	case FES_SELECT:
		return "Front End Select";

	case FES_BACK:
		return "Front End Back";

	default:
		return "";
	};
}
