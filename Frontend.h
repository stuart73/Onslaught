#ifndef FRONTEND_H
#define FRONTEND_H

#include	"FrontEndData.h"
#include	"FrontEndPage.h"
#include	"FrontEndText.h"
#include	"FrontEndVideo.h"
#include	"FEMessBox.h"

#include	"FEPMain.h"
#include	"FEPDevelopment.h"
#include	"FEPCommon.h"
#include	"FEPLevelSelect.h"
#include	"FEPBriefing.h"
#include	"FEPDebriefing.h"
#include	"FEPGoodies.h"
#include	"FEPDevSelect.h"
#include	"FEPLoadGame.h"
#include	"FEPSaveGame.h"
#include	"FEPBEConfig.h"
#include	"FEPWingmen.h"
#include	"FEPIntro.h"
#include	"FEPMultiplayer.h"
#include	"FEPMultiplayerStart.h"
#include	"FEPVirtualKeyboard.h"
#include	"FEPDirectory.h"
#include	"FEPE3LevelSelect.h"
#include	"FEPLanguageTest.h"
#include	"FEPDemoMain.h"
#include	"FEPOptions.h"
#include	"FEPCredits.h"
#include	"FEPController.h"

#include	"Controller.h"
#include	"ParticleManager.h"

#include	"Career.h"
#include	"Text.h"

#include	"Game.h"

//*********************************************************************************
//#define TEXT_RESOURCE_TEST_BUILD

//*********************************************************************************

#define FED	(FRONTEND.mFrontEndData)
#define FET (FRONTEND.mFrontEndText)
#define FEV (FRONTEND.mFrontEndVideo)

//*********************************************************************************
enum	EFrontEndEntry
{
	FEE_START = 0,
	FEE_FROM_ATTRACT,
	FEE_TITLE_SCREEN
};

//*********************************************************************************
enum	EFrontEndSound
{
	FES_MOVE	 = 0,
	FES_SELECT,
	FES_BACK,

	FES_NUM_SOUNDS
};

//*********************************************************************************
// Common defines

#define	REFLECTION_OFFSET		512.f
#define	REFLECTION_COLOUR		0xff7f7f7f

//#define	TRANSITION_RING_X				320.f
//#define	TRANSITION_RING_Y				240.f
#define	TRANSITION_RING_X				285.f
#define	TRANSITION_RING_Y				225.f
#define	TRANSITION_RING_SCALE_START		0.1f
#define	TRANSITION_RING_SCALE_END		1.6f

#define	MAGIC_LEVEL_SELECT_X (680.f - 512.f)
#define	MAGIC_LEVEL_SELECT_Y (138.f - 256.f)
#define	MAGIC_GOODIES_X (300.f - 512.f)
#define	MAGIC_GOODIES_Y (460.f - 256.f)

#define	LEVEL_SELECT_LEFT_X		73.f
#define	LEVEL_SELECT_LEFT_Y		315.f
#define	LEVEL_SELECT_RIGHT_X	515.f
#define	LEVEL_SELECT_RIGHT_Y	409.f

#define LEVEL_ROT_POINT_X		360.f
#define LEVEL_ROT_POINT_Y		360.f
#define LEVEL_ROT_POINT2_X		320.f
#define LEVEL_ROT_POINT2_Y		240.f

#define MAINTIME				70

//*********************************************************************************
class	CFrontEnd : public IController
{
public:
#if TARGET == PS2
	enum {NUM_CONTROLLER_PORTS = 2};
#else
	enum {NUM_CONTROLLER_PORTS = 4};
#endif

	CFrontEnd();

	// external funcions
	SINT			Run(EFrontEndEntry	entry,BOOL inLoadedSystem);

	virtual void	ReceiveButtonAction(CController* from_controller, int button, float val);
	virtual BOOL	CanBeControlledWhenInPause() { return(TRUE); };
	virtual EControlType  GetControlType() { return CONTROL_FRONTEND ; }

	// internal functions
	SINT			GetCounter() {return mCounter;};
	void			SetQuit(SINT val) {mQuit = val;};

	void			SetPage(EFrontEndPage page, SINT time = 10);
	EFrontEndPage	GetCurrentPage() {return mActivePage;};
	EFrontEndPage	GetTransitionTo() {return mTransitionTo;};
	EFrontEndPage	GetTransitionFrom() {return mTransitionFrom;};

	CFEPCommon		*GetCommonPage() {return &mFEPCommon;};

	// graphical stuff
	void			RenderSlidingScreen(CTEXTURE *screen,int alpha, float offy, DWORD col = 0xffffffff);
	void			DrawLine(float sx, float sy, float ex, float ey, DWORD col, float width, float depth, float perc = 1.f);
	void			DrawBox(float tlx, float tly, float brx, float bry, DWORD col, float width, float depth);
	void			DrawBar(float sx, float sy, float z, SINT segs, DWORD col, float scale = 1.f);
	void			DrawPanel(float tlx, float tly, float brx, float bry, float z, DWORD col);
	void			DrawBarGraph(float tlx, float tly, float brx, float bry, float num, float max, float z, SINT bordercol, SINT backcol, SINT forecol);

	void			DrawTitleBar(WCHAR *text, float transition, EFrontEndPage dest);
	void			DrawSlidingTextBordersAndMask(float transition, EFrontEndPage dest);
	void			DrawStandardVideoBackground(float transition, DWORD alpha, EFrontEndPage dest);


	void			EnableAdditiveAlpha();
	void			EnableModulateAlpha();

	virtual BOOL	RenderStart();
	virtual void	RenderEnd(BOOL started);
	void			UpdateCamera();

	float			GetShadowOffsetX();
	float			GetShadowOffsetY();

	BOOL			GetSaveMode() { return(mSaveMode); }

	// autosaving has 3 states:
	enum AutoSaveMode
	{
		AUTO_SAVE_NOT, // it's an explicit save, ie the first save of the career
		AUTO_SAVE_NORMAL, // the usual, we autosave when the career changes ie at the end of levels.
		AUTO_SAVE_PRETEND, // as normal, we're autosaving, but the actual write to disk doesn't happen.
	};

	AutoSaveMode	GetAutoSave() { return(mAutoSave); }
	void			SetAutoSave(AutoSaveMode as) { mAutoSave = as; }

	int				GetMemoryCardNumber() { return(mMemoryCardNumber); }
	void			SetMemoryCardNumber(int num) { mMemoryCardNumber=num; }

	void			SetSaveMode(BOOL mode) { mSaveMode = mode; }
	void			SetSuccessFEP(EFrontEndPage page, UINT time)	{ mOnSuccessFEP = page; mFEPTransTime = time; }
	EFrontEndPage	GetSuccessPage() { return(mOnSuccessFEP); }
	UINT			GetSuccessTransTime() { return(mFEPTransTime); }

	BOOL			AllLevelsCheatActive()							{ return mAllLevelsCheatActive; }

	// sound
	void			PlaySound(EFrontEndSound sound);
	char			*GetSoundName(EFrontEndSound sound);	

	CFrontEndData	mFrontEndData;
	CFrontEndText	mFrontEndText;
	CFRONTENDVIDEO	mFrontEndVideo;

	// Selected stuff
	SINT			mSelectedLevel;
	
	virtual void	Shutdown();

	void			SetLanguage(SINT l);

	// which port is player 0 connected to?
	int				GetPlayer0ControllerPort();

	// mostly for the xbox preserve-state-through-reboot code.
	bool			SerialiseState(FILE *dest, bool saving);

	void			ForceDisplay();

	void			StartMCOperation(WCHAR *message);
	void			EndMCOperation();

protected:
	BOOL	Init(EFrontEndEntry	entry,BOOL inLoadedSystem);

	void	Process();
	BOOL	Render(BOOL forcerender=FALSE);	
	
	// Internal State
	EFrontEndPage	mActivePage;
	EFrontEndPage	mTransitionFrom, mTransitionTo;
	SINT			mTransitionCount, mTransitionTime;
	EFrontEndPage	mDevReturnPage;

	float			mTimerStart;

	// Page Pointers
	CFrontEndPage	*mPages[FEP_NUM_PAGES];

public:
	// controller who's who.
	int	mPlayer0ControllerPort;

	// Pages
	CFEPMain				mFEPMain;
	
	CFEPDevelopment			mFEPDevelopment;

	CFEPCommon				mFEPCommon;

	CFEPBEConfig			mFEPBEConfig;
	CFEPWingmen				mFEPWingmen;
	CFEPBriefing			mFEPBriefing;
	CFEPDebriefing			mFEPDebriefing;
	CFEPLevelSelect			mFEPLevelSelect;
	CFEPGoodies				mFEPGoodies;
	CFEPDevSelect			mFEPDevSelect;
	CFEPLoadGame			mFEPLoadGame;
	CFEPSaveGame			mFEPSaveGame;
	CFEPIntro				mFEPIntro;
	CFEPMultiplayer			mFEPMultiplayer;
	CFEPMultiplayerStart	mFEPMultiplayerStart;
	CFEPOptions				mFEPOptions;
	CFEPCredits				mFEPCredits;
	CFEPController			mFEPController;

	CFEPVirtualKeyboard		mFEPVirtualKeyboard;
	CFEPDirectory			mFEPDirectory;
	CFEPE3LevelSelect		mFEPE3LevelSelect;
	CFEPLanguageTest		mFEPLanguageTest;

	CFEPDemoMain			mFEPDemoMain;

	CFEPUnknown				mFEPUnknown;

	int						mReceivingButtonActionPort;
	
	int						NumControllersPresent();

	CCONTROLLER *			GetController(int which) {ASSERT(which >= 0 && which < NUM_CONTROLLER_PORTS); return mControllers[which];}

	CTextDB					*GetTextSet(SINT n) {return &mTextSets[n];};

	// on reboot the platform wants to pretend that this isn't the first run
	void					ClearFirstRun() {mFirstRun = FALSE;}

protected:

	// Other Stuff
	CCONTROLLER			*mControllers[NUM_CONTROLLER_PORTS];

	int					GetPortFromController(CController *controller);

	SINT				mQuit;
	SINT				mCounter;

	BOOL				mFirstRun;
	float				mLastRenderTime;
	
	BOOL				mAllLevelsCheatActive;

	// Save Game Stuff
	BOOL				mSaveMode;
	AutoSaveMode		mAutoSave;						// AutoSave mode flag
	char				mSaveFilename[MAX_PATH];		// Save game filename
	EFrontEndPage		mOnSuccessFEP;
	UINT				mFEPTransTime;
	int					mMemoryCardNumber;

	// Language selection stuff
	CTextDB				mTextSets[NUM_LANGUAGES];
};

#include	"TransitionHelpers.h"


//#define	FO(x) ((sqrtf(float(x) / 1000.f)))
#define	FO(x) (((float(x) / 1000.f)))

#ifdef _DIRECTX
#include	"DXFrontend.h"
#elif TARGET == PS2
#include	"PS2Frontend.h"
#endif



#endif