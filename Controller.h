#ifndef CONTROLLER_INCLUDE
#define CONTROLLER_INCLUDE


#include "monitor.h"
#include "Activereader.h"
#include "MemBuffer.h"


// oh dear, the Xbox controllers appear to need really huge dead zones.
#define ANALOGUE_X_DEAD 0.36f 
#define ANALOGUE_Y_DEAD 0.36f
#define ANALOGUE_BUTTON_DEAD 0.3f


#define END_CONTROL_MAPPINGS 999 

enum EControlType
{
	CONTROL_FRONTEND,
	CONTROL_MECH,
	CONTROL_CAMERA,
	CONTROL_GAMEINTERFACE,
	CONTROL_GAME,
	CONTROL_MESSAGE_LOG,
	CONTROL_LEVEL_BRIEFING_LOG,
	CONTROL_GAME_MENU
};

class CController ;

// object which inherent from iController can be controlled from a controller

// has to be a child of cmonitor becuase a CController will place an active reader
// on this class

class IController : public CMonitor
{
public:
	virtual void	ReceiveButtonAction(CController* from_controller, int button, float ana_val) =0; ;
	virtual BOOL	CanBeControlledWhenInPause() =0; ;
	virtual EControlType    GetControlType() =0; ;
};


#define ANALOGUE_ACT_AS_DIGITAL_THRESHOLD 0.9f 


#define ANALOGUE_X1 -1
#define ANALOGUE_Y1 -2
#define ANALOGUE_X2 -3
#define ANALOGUE_Y2 -4




enum EJoyButtonPushType
{
	BUTTON_ON,
	BUTTON_ONCE,
	BUTTON_RELEASE,
	BUTTON_REPEAT,
	ANALOGUE_PLUS,
	ANALOGUE_MINUS,
	ANALOGUE_PLUS_ACT_AS_BUTTON_REPEAT,
	ANALOGUE_MINUS_ACT_AS_BUTTON_REPEAT,
	KEY_ONCE,
	KEY_ON
};


struct ControllerMaping
{
	int configuration;		// for different controller configurations ( -1 means all configurations )
	int vc_button_action;
	EJoyButtonPushType push_type;
	int joy_button;
};




// buttons in the virtual controller


// IMPORTANT.  Debug buttons  THESE buttons are for debug stuff only.  add game controls (e.g. pause)  adter 16.  Ta Stu.

// these buttons are stored because they can change the way the game could go if you where recording.
// the response to these buttons ALWAY go to the GAME class

#define BUTTON_TOGGLE_GOD_MODE			0
#define BUTTON_TOGGLE_FREE_CAMERA		1
#define BUTTON_ADVANCE_ONE_FRAME		2
#define BUTTON_TOGGLE_DEBUG_SQUAD_FORWARD		3
#define BUTTON_TOGGLE_DEBUG_SQUAD_BACKWARD		4
#define BUTTON_TOGGLE_DEBUG_UNIT_FORWARD		5
#define BUTTON_TOGGLE_DEBUG_UNIT_BACKWARD		6
#define BUTTON_SKIP_CUTSCENE					7 // hey this isn't a debug button now
#define BUTTON_CONSOLE_MENU_UP					8
#define BUTTON_CONSOLE_MENU_DOWN				9
#define BUTTON_CONSOLE_MENU_SELECT				10
#define	BUTTON_WIN_LEVEL						11
#define	BUTTON_LOOSE_LEVEL						12
#define BUTTON_LOG_CAREER						13
#define BUTTON_COMPLETE_ALL_OBJECTIVES			14

// ------------------------------------------- end debug buttons ----------------------------

// normal buttons in virtual controller.  these buttons get sent to who ever is been controlled
#define START_ACTION_BUTTONS			16

// buttons sent to IControler ReciveButtonAction
#define BUTTON_MECH_CHANGE_ZOOM_IN		16	
#define BUTTON_MECH_CHANGE_ZOOM_OUT		17		
#define BUTTON_MECH_FIRE_GUN_POD		18	
#define BUTTON_MECH_CHARGE_GUN_POD		19
#define BUTTON_MECH_CHANGE_WEAPON		20
//#define BUTTON_MECH_CHARGE_SUPPORT_POD	16

#define BUTTON_MECH_LANDING_JETS		21
#define BUTTON_MECH_JET_AFTERBURNER		22

#define BUTTON_MECH_YAW_LEFT			25
#define BUTTON_MECH_PITCH_UP			26
#define BUTTON_MECH_YAW_RIGHT			27
#define BUTTON_MECH_PITCH_DOWN		    28
#define BUTTON_MECH_STRAFE_LEFT			29
#define BUTTON_MECH_STRAFE_RIGHT	    30
#define BUTTON_MECH_FORWARD				31
#define BUTTON_MECH_BACKWARD			32

#define	BUTTON_MECH_MORPH				33

#define BUTTON_CAMERA_PITCH_UP			34
#define BUTTON_CAMERA_PITCH_DOWN		35
#define BUTTON_CAMERA_YAW_LEFT			36
#define BUTTON_CAMERA_YAW_RIGHT			37
#define BUTTON_CAMERA_MOVE_FORAWRD      38
#define BUTTON_CAMERA_MOVE_BACKWARDS	39
#define BUTTON_CAMERA_STRAFE_LEFT		40
#define BUTTON_CAMERA_STRAFE_RIGHT		41
#define BUTTON_FRONTEND_MENU_UP			42
#define BUTTON_FRONTEND_MENU_DOWN		43
#define BUTTON_FRONTEND_MENU_SELECT		44
#define BUTTON_FRONTEND_CHEAT			45
#define BUTTON_FRONTEND_MENU_BACK		46
#define BUTTON_SAVE_CAREER				47
#define BUTTON_LOAD_CAREER				48
#define BUTTON_PAUSE_MENU_UP			49
#define BUTTON_PAUSE_MENU_DOWN			50
#define BUTTON_PAUSE_MENU_SELECT		51
#define BUTTON_LEFT1_FOR_TOGGLE			52
#define BUTTON_RIGHT1_FOR_TOGGLE		53
#define BUTTON_FRONTEND_MENU_LEFT		54
#define BUTTON_FRONTEND_MENU_RIGHT		55
#define BUTTON_PAUSE					56
#define BUTTON_HELP						57
#define BUTTON_SKIP_PANNING				58
#define BUTTON_MECH_CLOAK				59
#define BUTTON_FRONTEND_MENU_SKIP		60
#define BUTTON_MECH_CONFIGURATION_DOWN	61
#define BUTTON_MECH_CONFIGURATION_UP	62
#define BUTTON_FRONTEND_MENU_SPECIAL	63
#define BUTTON_START					64
#define BUTTON_SELECT					65

#define BUTTON_BREAK_ATTRACT_MODE		66

#define TOTAL_BUTTONS					67	// (SRG this code can handle upto a maximum of 96 buttons at the mo)


// virtual controller class

class CController
{
public:
					CController(IController* to_control,int padnumber, int configuration = 1, BOOL reverse_look_y_axis = FALSE) ;
	virtual			~CController();

	virtual void	DeviceSetVibration(float inValue) = 0;

	// the controllers know about user inactivity:
	static void			ResetInactivityTimer();
	static void			SetNonInteractiveSection(bool onoff);

	// if this returns true at any point, it means you're in attract mode and no-one's pressed anything for so long
	// that you should quit the same
	static bool			InactivityMeansQuitGame();

	void			SetToControl(IController* to_control); 
	IController*	GetToControl();

	void			RelinquishControl();
	void			SetConfigurationNum(int num) { mConfigurationNum = num ; }
	int				GetConfigurationNum() { return mConfigurationNum ; }
	void			SetReverseLookYAxis(BOOL val) ;
	BOOL			GetReverseLookYAxis() { return mReverseLookYAxis ; }

	void			SetVibration(float inValue, int zero_based_player_number);

	// just check if it's there - this is used to check if it's come unplugged by accident
	BOOL			IsPresent() {return TRUE;}


	virtual	void	Flush() ;


	void			StartRecording(char *filename);
	void			StartPlayback(char *filename);

	void	SetButton(SINT n)
	{
		// ### SRG (fixme ) stop doing this moho idea,  and use that binary block class
		if (n >= 64)
			mButtons3 |= (1 << (n - 64));
		else if (n >= 32)
			mButtons2 |= (1 << (n - 32));
		else
			mButtons1 |= (1 <<  n      );
	}

	void	UnsetButton(SINT n)
	{
		if (n >=64)
			mButtons3 &= ~(1 << (n - 64));
		else if (n >= 32)
			mButtons2 &= ~(1 << (n - 32));
		else
			mButtons1 &= ~ (1 <<  n     );
	}


	BOOL	IsButtonSet(SINT n)
	{
		ULONG b = 0 ;

		if (n >= 64)
			b= mButtons3 & (1 << (n - 64));
		else if (n >= 32)
			b= mButtons2 & (1 << (n - 32));
		else
			b= mButtons1 & (1 <<  n      );
		if (b==0) return FALSE; else return TRUE ;
	}

	BOOL	IsButtonOldSet(SINT n)
	{
		ULONG b= 0 ;
		if (n >= 64)
			b= mButtons3Old & (1 << (n - 64));
		if (n >= 32)
			b= mButtons2Old & (1 << (n - 32));
		else
			b= mButtons1Old & (1 <<  n      );
		if (b==0) return FALSE; else return TRUE;
	}

	int GetPadNumber() {return mPadNumber;}

protected:

	void	SendButtonAction(int button, float ana_val);
	float	GetAnalogueValueForButton(int button) ;
	void	CalcNumMappings() ;
	BOOL	RepeatOccured(BOOL repeat_on, ControllerMaping* mapping);

	// these functions get called at the platform specific level.
	// they must return a digital answer of TRUE or FALSE if that button
	// is being pressed

	virtual BOOL	GetJoyButtonOnce(int pad_number, int button) =0; ; 
	virtual BOOL	GetJoyButtonOn(int pad_number, int button) =0; ;
	virtual BOOL	GetJoyButtonRelease(int pad_number, int button) =0;;

	virtual BOOL	GetKeyOnce(int pad_number, int key) =0; ; 
	virtual BOOL	GetKeyOn(int pad_number, int key) =0; ;

	// these valeus get called at the platform specific level.
	// the must return values in the range of -1.0 and +1.0

	virtual float	GetJoyAnalogueLeftX(int pad_number) =0; ;
	virtual float	GetJoyAnalogueLeftY(int pad_number) =0; ;
	virtual	float	GetJoyAnalogueRightX(int pad_number) =0; ;
	virtual float	GetJoyAnalogueRightY(int pad_number) =0;;


	
	virtual	void	DoMappings() ;
	virtual void	RecordControllerState() =0; ;
	virtual void	ReadControllerState() =0; ;


	SPtrSet<CActiveReader<IController> >		mToControlStack ;

	ULONG		mButtons1;
	ULONG		mButtons2;
	ULONG		mButtons3;

	float		mAnaloguex1;
	float		mAnaloguex2;
	float		mAnaloguey1;
	float		mAnaloguey2;

	ULONG		mButtons1Old;
	ULONG		mButtons2Old;
	ULONG		mButtons3Old;

	CMEMBUFFER	mDataFile;
	bool		mRecording;
	bool		mPlaying;
	float		mLastRepeatTime;
	float		mNextDelayTime;
	
	int			mPadNumber;
	BOOL		mPausedBeforeMap;
	BOOL		mReverseLookYAxis;
	int			mConfigurationNum;

	static float	mLastTimeAnythingPressed; // not always true
	static bool		mNonInteractiveSection;

	static		int mNumMappings;
	static      ControllerMaping mMappings[];
};

#if TARGET == PC

#include "PCController.h"
#define CCONTROLLER CPCController

#elif TARGET == PS2

#include "PS2Controller.h"
#define CCONTROLLER CPS2Controller

#elif TARGET == XBOX

#include "XBOXController.h"
#define CCONTROLLER CXBOXController
#endif

#endif

