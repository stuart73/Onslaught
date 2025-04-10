// PCController.cpp: implementation of the PCController class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"

#if TARGET == PC

#include "PCController.h"
#include "console.h"
#include "Platform.h"
//#include "DX.h"
  

ControllerMaping CController::mMappings[] =  {

{ -1, BUTTON_PAUSE,		BUTTON_ONCE,		11},   
{ -1, BUTTON_START,		BUTTON_ONCE,		11},   
{ -1, BUTTON_PAUSE_MENU_UP,		BUTTON_ONCE,		12},	
{ -1, BUTTON_PAUSE_MENU_DOWN,   BUTTON_ONCE,       14},	   
{ -1, BUTTON_PAUSE_MENU_SELECT, BUTTON_ONCE,		1},     
{ -1, BUTTON_HELP,				BUTTON_ONCE,		3},     
{ -1, BUTTON_FRONTEND_MENU_BACK, BUTTON_ONCE,		3},     
{ -1, BUTTON_CONSOLE_MENU_UP,	BUTTON_ONCE,		12},    
{ -1, BUTTON_FRONTEND_MENU_UP,   BUTTON_REPEAT,		12},	
{ -1, BUTTON_CONSOLE_MENU_DOWN,	BUTTON_ONCE,		14},	
{ -1, BUTTON_FRONTEND_MENU_DOWN, BUTTON_REPEAT,		14},	
{ -1, BUTTON_CONSOLE_MENU_SELECT,		 BUTTON_ONCE,		0},		
{ -1, BUTTON_FRONTEND_MENU_SELECT, BUTTON_ONCE,    0},		
{ -1, BUTTON_CONSOLE_MENU_SELECT,		BUTTON_ONCE,		1},		
{ -1, BUTTON_FRONTEND_MENU_SELECT, BUTTON_ONCE,	1},			
{ -1, BUTTON_CAMERA_YAW_LEFT,			ANALOGUE_MINUS,  ANALOGUE_X2},	
{ -1, BUTTON_CAMERA_YAW_RIGHT,			ANALOGUE_PLUS, ANALOGUE_X2},	
{ -1, BUTTON_CAMERA_PITCH_UP,			ANALOGUE_PLUS,	 ANALOGUE_Y2},	
{ -1, BUTTON_CAMERA_PITCH_DOWN,			ANALOGUE_MINUS, ANALOGUE_Y2},	
{ -1, BUTTON_CAMERA_MOVE_FORAWRD,		ANALOGUE_MINUS,  ANALOGUE_Y1},	
{ -1, BUTTON_CAMERA_MOVE_BACKWARDS,		ANALOGUE_PLUS, ANALOGUE_Y1},	
{ -1, BUTTON_CAMERA_STRAFE_LEFT,		ANALOGUE_MINUS,  ANALOGUE_X1},	
{ -1, BUTTON_CAMERA_STRAFE_RIGHT,		ANALOGUE_PLUS, ANALOGUE_X1},
{ -1, BUTTON_TOGGLE_GOD_MODE,			KEY_ONCE,		'V'},			
{ -1, BUTTON_TOGGLE_FREE_CAMERA,		KEY_ONCE,		'A'},			
{ -1, BUTTON_PAUSE,				KEY_ONCE,		'O'},			
{ -1, BUTTON_TOGGLE_DEBUG_SQUAD_FORWARD, KEY_ONCE,		'9'},		
{ -1, BUTTON_TOGGLE_DEBUG_SQUAD_BACKWARD, KEY_ONCE,     '8'},			
{ -1, BUTTON_TOGGLE_DEBUG_UNIT_FORWARD, KEY_ONCE,		'5'},				
{ -1, BUTTON_TOGGLE_DEBUG_UNIT_BACKWARD, KEY_ONCE,		'4'},		
{ -1, BUTTON_SKIP_CUTSCENE,				BUTTON_ONCE,		' '},	
{ -1, BUTTON_BREAK_ATTRACT_MODE,		BUTTON_ONCE,		0},
{ -1, BUTTON_CONSOLE_MENU_UP,			KEY_ONCE,	VK_NUMPAD8},		
{ -1, BUTTON_CONSOLE_MENU_DOWN,			KEY_ONCE, VK_NUMPAD2},		
{ -1, BUTTON_SAVE_CAREER,				KEY_ONCE,		'S'},			
{ -1, BUTTON_LOAD_CAREER,				KEY_ONCE,		'L'},			
{ -1, BUTTON_FRONTEND_MENU_SELECT,		KEY_ONCE,KEYCODE_RETURN},		
{ -1, BUTTON_TOGGLE_FREE_CAMERA,		KEY_ONCE,		'F'},			
{ -1, BUTTON_FRONTEND_MENU_DOWN,		KEY_ONCE,KEYCODE_DOWN},			
{ -1, BUTTON_FRONTEND_MENU_UP,			KEY_ONCE,KEYCODE_UP},  		
{ -1, BUTTON_FRONTEND_MENU_LEFT,		KEY_ONCE,KEYCODE_LEFT},		
{ -1, BUTTON_FRONTEND_MENU_RIGHT,		KEY_ONCE,KEYCODE_RIGHT},  		
{ -1, BUTTON_FRONTEND_MENU_LEFT,		BUTTON_REPEAT,15},				
{ -1, BUTTON_FRONTEND_MENU_RIGHT,		BUTTON_REPEAT,13},  			
{ -1, BUTTON_MECH_CLOAK,				BUTTON_ONCE,	0 },			
{ -1, BUTTON_WIN_LEVEL,				    KEY_ONCE,		'U'},			
{ -1, BUTTON_LOOSE_LEVEL,				KEY_ONCE,		'I'},			
{ -1, BUTTON_LOG_CAREER,				KEY_ONCE,		'Z'},	
{ -1, BUTTON_COMPLETE_ALL_OBJECTIVES,	KEY_ONCE,		'7'},	
{ -1, BUTTON_FRONTEND_MENU_DOWN,	ANALOGUE_PLUS_ACT_AS_BUTTON_REPEAT,  ANALOGUE_Y1},
{ -1, BUTTON_FRONTEND_MENU_UP,		ANALOGUE_MINUS_ACT_AS_BUTTON_REPEAT,  ANALOGUE_Y1},
{ -1, BUTTON_FRONTEND_MENU_RIGHT,	ANALOGUE_PLUS_ACT_AS_BUTTON_REPEAT,  ANALOGUE_X1},
{ -1, BUTTON_FRONTEND_MENU_LEFT,	ANALOGUE_MINUS_ACT_AS_BUTTON_REPEAT,  ANALOGUE_X1},	
{ -1, BUTTON_SELECT,				BUTTON_ONCE,		8},

// is really common
{ -1, BUTTON_LEFT1_FOR_TOGGLE,			BUTTON_ON,		6},		
{ -1, BUTTON_RIGHT1_FOR_TOGGLE,			BUTTON_ON,		7},		
{ -1, BUTTON_MECH_JET_AFTERBURNER,		BUTTON_ON,		9},		
{ -1, BUTTON_SKIP_PANNING,				BUTTON_ONCE,    1},		
{ -1, BUTTON_MECH_CONFIGURATION_UP,		BUTTON_ONCE,	13},	
{ -1, BUTTON_MECH_CONFIGURATION_DOWN,	BUTTON_ONCE,	15},	

// All configurations
{ -1, BUTTON_MECH_CHARGE_GUN_POD,		BUTTON_ON,		7},		
{ -1, BUTTON_MECH_FIRE_GUN_POD,			BUTTON_RELEASE,	7},	
{ -1, BUTTON_MECH_CHARGE_GUN_POD,		BUTTON_ON,		5},		
{ -1, BUTTON_MECH_FIRE_GUN_POD,			BUTTON_RELEASE,	5},		
{ -1, BUTTON_MECH_CHANGE_WEAPON,			BUTTON_ONCE,	6},		
{ -1, BUTTON_MECH_CHANGE_WEAPON,			BUTTON_ONCE,	4},		
{ -1, BUTTON_MECH_CHANGE_ZOOM_IN,		BUTTON_ON,		12},	
{ -1, BUTTON_MECH_CHANGE_ZOOM_OUT,		BUTTON_ON,		14},	

// Configuration 1
{  1, BUTTON_MECH_YAW_LEFT,	 			ANALOGUE_MINUS,  ANALOGUE_X2},	
{  1, BUTTON_MECH_YAW_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X2},	
{  1, BUTTON_MECH_PITCH_UP,				ANALOGUE_PLUS,	 ANALOGUE_Y2},  
{  1, BUTTON_MECH_PITCH_DOWN,			ANALOGUE_MINUS,  ANALOGUE_Y2},	
{  1, BUTTON_MECH_STRAFE_LEFT,			ANALOGUE_MINUS,  ANALOGUE_X1},	
{  1, BUTTON_MECH_STRAFE_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X1},	
{  1, BUTTON_MECH_FORWARD,				ANALOGUE_MINUS,	 ANALOGUE_Y1},	
{  1, BUTTON_MECH_BACKWARD,				ANALOGUE_PLUS,   ANALOGUE_Y1},	
{  1, BUTTON_MECH_MORPH,				BUTTON_ONCE,	2},		
{  1, BUTTON_MECH_LANDING_JETS,			BUTTON_ON,		1},		

// Configuration 2
{  2, BUTTON_MECH_YAW_LEFT,	 			ANALOGUE_MINUS,  ANALOGUE_X1},	
{  2, BUTTON_MECH_YAW_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X1},	
{  2, BUTTON_MECH_PITCH_UP,				ANALOGUE_PLUS,	 ANALOGUE_Y1},  
{  2, BUTTON_MECH_PITCH_DOWN,			ANALOGUE_MINUS,  ANALOGUE_Y1},	
{  2, BUTTON_MECH_STRAFE_LEFT,			ANALOGUE_MINUS,  ANALOGUE_X2},	
{  2, BUTTON_MECH_STRAFE_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X2},	
{  2, BUTTON_MECH_FORWARD,				ANALOGUE_MINUS,	 ANALOGUE_Y2},	
{  2, BUTTON_MECH_BACKWARD,				ANALOGUE_PLUS,   ANALOGUE_Y2},	
{  2, BUTTON_MECH_MORPH,				BUTTON_ONCE,	2},		
{  2, BUTTON_MECH_LANDING_JETS,			BUTTON_ON,		1},		

// Configuration 3
{  3, BUTTON_MECH_YAW_LEFT,	 			ANALOGUE_MINUS,  ANALOGUE_X2},	
{  3, BUTTON_MECH_YAW_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X2},	
{  3, BUTTON_MECH_PITCH_UP,				ANALOGUE_PLUS,	 ANALOGUE_Y2},  
{  3, BUTTON_MECH_PITCH_DOWN,			ANALOGUE_MINUS,  ANALOGUE_Y2},	
{  3, BUTTON_MECH_STRAFE_LEFT,			ANALOGUE_MINUS,  ANALOGUE_X1},	
{  3, BUTTON_MECH_STRAFE_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X1},	
{  3, BUTTON_MECH_FORWARD,				ANALOGUE_MINUS,	 ANALOGUE_Y1},	
{  3, BUTTON_MECH_BACKWARD,				ANALOGUE_PLUS,   ANALOGUE_Y1},	
{  3, BUTTON_MECH_MORPH,				BUTTON_ONCE,	1},		
{  3, BUTTON_MECH_LANDING_JETS,			BUTTON_ON,		2},		

// Configuration 4
{  4, BUTTON_MECH_YAW_LEFT,	 			ANALOGUE_MINUS,  ANALOGUE_X1},	
{  4, BUTTON_MECH_YAW_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X1},	
{  4, BUTTON_MECH_PITCH_UP,				ANALOGUE_PLUS,	 ANALOGUE_Y1},  
{  4, BUTTON_MECH_PITCH_DOWN,			ANALOGUE_MINUS,  ANALOGUE_Y1},	
{  4, BUTTON_MECH_STRAFE_LEFT,			ANALOGUE_MINUS,  ANALOGUE_X2},	
{  4, BUTTON_MECH_STRAFE_RIGHT,			ANALOGUE_PLUS,   ANALOGUE_X2},	
{  4, BUTTON_MECH_FORWARD,				ANALOGUE_MINUS,	 ANALOGUE_Y2},	
{  4, BUTTON_MECH_BACKWARD,				ANALOGUE_PLUS,   ANALOGUE_Y2},	
{  4, BUTTON_MECH_MORPH,				BUTTON_ONCE,	1},		
{  4, BUTTON_MECH_LANDING_JETS,			BUTTON_ON,		2},		
	
{ END_CONTROL_MAPPINGS, -1, BUTTON_ON, -1 },
}; 


//******************************************************************************************
CPCController::CPCController(IController* to_control, int padnumber,int configuration, BOOL reverse_look_y_axis) :
  CController(to_control, padnumber,configuration,reverse_look_y_axis)
{
}


//******************************************************************************************
// ok get raw values out from LT and convert them to values which is platform
// indepedent. values must range from -1.0 to 1.0
float	CPCController::GetJoyAnalogueLeftX(int pad_number)
{
	float lx = (float)(LT.JoyState(pad_number)->lX);
	lx = lx /1000.0f ;
	return lx;
}

//******************************************************************************************
// ok get raw values out from LT and convert them to values which is platform
// indepedent. values must range from -1.0 to 1.0
float	CPCController::GetJoyAnalogueLeftY(int pad_number)
{

	float ly = (float)(LT.JoyState(pad_number)->lY);
	ly = ly /1000.0f ;
	return ly;
}


//******************************************************************************************
// ok get raw values out from LT and convert them to values which is platform
// indepedent. values must range from -1.0 to 1.0
float	CPCController::GetJoyAnalogueRightX(int pad_number)
{
	float vx = (float)(LT.JoyState(pad_number)->lZ);
	vx = vx / 1000.0f;
	return vx ;
}

//******************************************************************************************
// ok get raw values out from LT and convert them to values which is platform
// indepedent. values must range from -1.0 to 1.0
float	CPCController::GetJoyAnalogueRightY(int pad_number)
{
	float vy = (float)(LT.JoyState(pad_number)->lRz);
//	LOG.AddMessage("ray vy = %2.8f", vy) ;
	vy = vy - 32768.0f ;
	vy = vy / 32768.0f ;
	return vy ;
}



//******************************************************************************************
void	CPCController::RecordControllerState()
{
	mDataFile.Write(&mButtons1,sizeof(mButtons1));
	mDataFile.Write(&mButtons2,sizeof(mButtons2));
	mDataFile.Write(&mButtons3,sizeof(mButtons3));
	mDataFile.Write(&mAnaloguex1,sizeof(mAnaloguex2));
	mDataFile.Write(&mAnaloguey1,sizeof(mAnaloguex2));
	mDataFile.Write(&mAnaloguex2,sizeof(mAnaloguex2));
	mDataFile.Write(&mAnaloguey2,sizeof(mAnaloguex2));
}


//******************************************************************************************
void CPCController::ReadControllerState()
{
	mDataFile.Read(&mButtons1,sizeof(mButtons1));
	mDataFile.Read(&mButtons2,sizeof(mButtons2));
	mDataFile.Read(&mButtons3,sizeof(mButtons3));
	mDataFile.Read(&mAnaloguex1,sizeof(mAnaloguex2));
	mDataFile.Read(&mAnaloguey1,sizeof(mAnaloguex2));
	mDataFile.Read(&mAnaloguex2,sizeof(mAnaloguex2));
	mDataFile.Read(&mAnaloguey2,sizeof(mAnaloguex2));
	if (mDataFile.EndOfFile())
	{
		mDataFile.Close();
		mPlaying=false;
	}
}

#endif