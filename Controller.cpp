#include "common.h"

#include "controller.h"
#include "player.h"
#include "game.h"
//#include "frontend.h"
#include "debuglog.h"
#include "cliparams.h"
#include "reconnectinterface.h"

float CController::mLastTimeAnythingPressed = 0.f;
bool CController::mNonInteractiveSection = false;

#define INITIAL_REPEAT_DELAY 0.5f
#define REPEAT_DELAY 0.12f


int CController::mNumMappings= -1;


//******************************************************************************************
CController::CController(IController* to_control, int padnumber, int configuration, BOOL reverse_look_y_axis)
{
	SetToControl(to_control) ;
	mRecording=false;
	mPlaying=false;
	mReverseLookYAxis = reverse_look_y_axis;
	mPadNumber=padnumber;
	mPausedBeforeMap =FALSE ;
	mConfigurationNum = configuration;
	mLastRepeatTime = -1.0f;
	mNextDelayTime =0.1f ;
	mButtons1 = 0 ;
	mButtons2 = 0; 
	mButtons3 = 0 ;
	mButtons1Old =0 ;
	mButtons2Old =0 ;
	mButtons3Old = 0;
}
//******************************************************************************************
void CController::ResetInactivityTimer()
{
	mLastTimeAnythingPressed = PLATFORM.GetSysTimeFloat();
}

//******************************************************************************************
void CController::SetNonInteractiveSection(bool onoff)
{
	if (mNonInteractiveSection == onoff)
	{
		// nothing interesting going on.
		return;
	}

	if (onoff)
	{
		// We're entering a non-interactive section
		if (CLIPARAMS.mAttractMode)
		{
			// the timer keeps ticking in the background unaffected
		}
		else
		{
			// we replace the timer with an indication of how much timeout is left, so that we
			// can restart it when the FMV is finished.
			mLastTimeAnythingPressed = PLATFORM.GetSysTimeFloat() - mLastTimeAnythingPressed;
		}
	}
	else
	{
		// We're coming back out again.
		if (CLIPARAMS.mAttractMode)
		{
			// The timer has just carried on ticking in the background, so nothing to do here.
		}
		else
		{
			// We restart the timer from where it was before the non-interactive bit.
			mLastTimeAnythingPressed = PLATFORM.GetSysTimeFloat() - mLastTimeAnythingPressed;
		}
	}

	mNonInteractiveSection = onoff;
}

//******************************************************************************************
bool CController::InactivityMeansQuitGame()
{
	// this only happens in playable demo
	if (PLAYABLE_DEMO)
	{
		// if we're in something non-interactive, never quit game. Either the timer is paused or we're
		// waiting until the end of the FMV anyway.
		if (mNonInteractiveSection)
			return false;

		// or if this is all disabled, don't quit.
		if (CLIPARAMS.mInactiveTimeout <= 0)
			return false;

		// otherwise, we're just waiting for the timeout
		float time_passed = PLATFORM.GetSysTimeFloat() - mLastTimeAnythingPressed;
		if ((time_passed * 1000.f) > CLIPARAMS.mInactiveTimeout)
		{
			printf("Inactivity timeout hit (last=%f,now=%f,time passed=%f, to=%d)\n",mLastTimeAnythingPressed,PLATFORM.GetSysTimeFloat(),time_passed,CLIPARAMS.mInactiveTimeout);
			// time to go!
			return true;
		}

		return false;
	}
	else
	{
		// outside playable demo, you never quit because of inactivity.
		return false;
	}
}

//******************************************************************************************
void CController::StartRecording(char *filename)
{
	mRecording=true;
	mDataFile.InitFromMem(filename);
}

//******************************************************************************************

void CController::StartPlayback(char *filename)
{
	mPlaying=true;
	mDataFile.InitFromFile(filename);
}

//******************************************************************************************

CController::~CController()
{
	if (mPlaying || mRecording)
	{
		mDataFile.Close();
	}

	mToControlStack.DeleteAll() ;
}


//******************************************************************************************
void CController::Flush()
{
	// clear buttons in virtual controller

	// ### SRG (fixme ) stop doing this moho idea,  and use that binary block class
	mButtons1Old =mButtons1 ;
	mButtons2Old =mButtons2;
	mButtons3Old =mButtons3;

	mButtons1 =0 ;
	mButtons2 =0;
	mButtons3 =0;


	// get platform specific child to fill in virtual buttons
	DoMappings() ;
}


//******************************************************************************************
void CController::CalcNumMappings()
{
	int i =0 ;
	BOOL found = FALSE ;
	while (found==FALSE)
	{
		ControllerMaping* mapping =  &mMappings[i] ;
		if (mapping->configuration==END_CONTROL_MAPPINGS)
		{
			found = TRUE ;
		}
		else
		{
			i++;
		}
	}
	mNumMappings = i ;
	LOG.AddMessage("Controller: Number of mappings found = %d", mNumMappings) ;
}


//******************************************************************************************
BOOL CController::RepeatOccured(BOOL repeat_on, ControllerMaping* mapping)
{
	if (IsButtonOldSet(mapping->vc_button_action) == FALSE)
	{
		// setup intital repeat delay
		mNextDelayTime = INITIAL_REPEAT_DELAY;
		mLastRepeatTime = PLATFORM.GetSysTimeFloat() ;
		return TRUE;
	}
	else 
	{
		if (repeat_on)
		{
			mNextDelayTime = REPEAT_DELAY;
			return TRUE ;
		}
		else	
		{
			// make sure we still mark that we are in a repeat
			SetButton(mapping->vc_button_action) ;
		}
	}
	return FALSE ;
}


//******************************************************************************************

void CController::DoMappings()
{
	if (mNumMappings ==-1)
	{
		CalcNumMappings() ;
	}

	// get state of analogue part
	mAnaloguex1 = GetJoyAnalogueLeftX(mPadNumber);
	if (fabs(mAnaloguex1) < ANALOGUE_X_DEAD) mAnaloguex1 = 0.0f ;
	mAnaloguey1 = GetJoyAnalogueLeftY(mPadNumber);
	if (fabs(mAnaloguey1) < ANALOGUE_Y_DEAD) mAnaloguey1 = 0.0f ;
	mAnaloguex2 = GetJoyAnalogueRightX(mPadNumber);
	if (fabs(mAnaloguex2) < ANALOGUE_X_DEAD) mAnaloguex2 = 0.0f ;
	mAnaloguey2 = GetJoyAnalogueRightY(mPadNumber);
	if (fabs(mAnaloguey2) < ANALOGUE_Y_DEAD) mAnaloguey2 = 0.0f ;

	// JCL - now done in player
//	if (mReverseLookYAxis) mAnaloguey2 = -mAnaloguey2;

	if (mPlaying)
	{
		ReadControllerState() ;
	}

	// auto repeat stuff (note only 1 repeat button is accepted at once )
	float sys_time = PLATFORM.GetSysTimeFloat() ;
	BOOL repeat_on = FALSE ;
	if (PLATFORM.GetSysTimeFloat() - mLastRepeatTime  > mNextDelayTime)
	{
		mLastRepeatTime = sys_time ;
		repeat_on = TRUE ;
	}


	mPausedBeforeMap = GAME.IsPaused() ;

	//map buttons from platform specific to buttons in virtual controller
	for (int i=0; i < mNumMappings; i++)
	{
		ControllerMaping* mapping =  &mMappings[i] ;

		// check that this mapping is for the current configuration
		if (mapping->configuration != mConfigurationNum && mapping->configuration !=-1) continue ;

		float ana_val = 0.0f;

		if ((!mPlaying) && (IsButtonSet(mapping->vc_button_action) == FALSE))
		{
			switch (mapping->push_type)
			{
				case BUTTON_ONCE:	
				{
					if (GetJoyButtonOnce(mPadNumber, mapping->joy_button) == TRUE)
					{
						SendButtonAction(mapping->vc_button_action, 0.0) ;
					}
					break ;
				}

				case BUTTON_ON:
				{
					if (GetJoyButtonOn(mPadNumber, mapping->joy_button) == TRUE)
					{
						SendButtonAction(mapping->vc_button_action, 0.0f) ;
					}
					break;
				}

				case BUTTON_REPEAT:
				{
					if (GetJoyButtonOn(mPadNumber, mapping->joy_button) == TRUE)
					{
						if (RepeatOccured(repeat_on, mapping)) 	
						{
							// send out button action
							 SendButtonAction(mapping->vc_button_action, 0.0f) ;
						}
					}
					
					break;
				}

				case BUTTON_RELEASE:
				{
					if (GetJoyButtonRelease(mPadNumber, mapping->joy_button) == TRUE)
					{
						SendButtonAction(mapping->vc_button_action, 0.0f) ;
					}
					break ;
				}

				case ANALOGUE_PLUS:
				{
					ana_val = GetAnalogueValueForButton(mapping->joy_button) ;
					if (ana_val > 0.0f) 
					{
						SendButtonAction(mapping->vc_button_action, ana_val) ;
					}
					break;
				}

				case ANALOGUE_MINUS:
				{
					ana_val = GetAnalogueValueForButton(mapping->joy_button) ;
					if (ana_val < 0.0f) 
					{
						SendButtonAction(mapping->vc_button_action, ana_val) ;
					}
					break ;
				}


				case ANALOGUE_PLUS_ACT_AS_BUTTON_REPEAT:
				{
					if(	GetAnalogueValueForButton(mapping->joy_button) >ANALOGUE_ACT_AS_DIGITAL_THRESHOLD)
					{
						if (RepeatOccured(repeat_on, mapping))
						{
							// send out button action
							SendButtonAction(mapping->vc_button_action, ana_val) ;
						}
					}

					break ;
				}

				case ANALOGUE_MINUS_ACT_AS_BUTTON_REPEAT:
				{
					if (GetAnalogueValueForButton(mapping->joy_button) < -ANALOGUE_ACT_AS_DIGITAL_THRESHOLD)
					{
						if (RepeatOccured(repeat_on, mapping))
						{
							// send out button action
							SendButtonAction(mapping->vc_button_action, ana_val) ;
						}
					}

					break ;
				}

				case KEY_ONCE:
				{
					if (GetKeyOnce(mPadNumber, mapping->joy_button) == TRUE)
					{
						SendButtonAction(mapping->vc_button_action, ana_val) ;
					}
					break ;
				}

				case KEY_ON:
				{
					if (GetKeyOn(mPadNumber, mapping->joy_button) == TRUE)
					{
						SendButtonAction(mapping->vc_button_action, ana_val) ;
					}
					break ;
				}



				default:
				{
					LOG.AddMessage("FATAL ERROR: Uknown 'push_type' in controller");
					break;
				}
			}
		}

		// playing
		if (mPlaying && IsButtonSet(mapping->vc_button_action) == TRUE)
		{
			ana_val = GetAnalogueValueForButton(mapping->joy_button) ;
			SendButtonAction(mapping->vc_button_action, ana_val);

			// make sure button can only happen once per flush (i.e. if there is two mappings to it)
			UnsetButton(mapping->vc_button_action);
		}
	}

	// hmmm
#ifdef E3BUILD
	if (IsButtonSet(BUTTON_LEFT1_FOR_TOGGLE) &&
		IsButtonSet(BUTTON_RIGHT1_FOR_TOGGLE) &&
		mAnaloguex1 < -0.6f &&
		mAnaloguex2 >  0.6f)
#else
	if (IsButtonSet(BUTTON_LEFT1_FOR_TOGGLE) &&
		IsButtonSet(BUTTON_RIGHT1_FOR_TOGGLE) )
#endif
	{
		SendButtonAction(BUTTON_FRONTEND_CHEAT, 0.0f);
	}

	if (mRecording)
	{
		RecordControllerState() ;
	}

}


//******************************************************************************************
float CController::GetAnalogueValueForButton(int button)
{
	switch (button)
	{
		case ANALOGUE_X1: return mAnaloguex1 ;
		case ANALOGUE_X2: return mAnaloguex2;
		case ANALOGUE_Y1: return mAnaloguey1;
		case ANALOGUE_Y2: return mAnaloguey2;
	}
	LOG.AddMessage("Error: Unknown or invalid button number in call to 'GetAnalogueValueForButton'");
	return 0.0f ;
}


//******************************************************************************************

IController *CController::GetToControl()
{
	return  mToControlStack.First()->ToRead();
}

//******************************************************************************************
void	CController::SendButtonAction(int button, float ana_val)
{
	SetButton(button) ;

	if (mToControlStack.First() == NULL)
	{
		LOG.AddMessage("Nothing to Control !!") ;
		return ;
	}

	IController* to_send = mToControlStack.First()->ToRead() ;

	// A button has been pressed! Let's reset our counting of inactivity
	ResetInactivityTimer();

	if (to_send == FALSE) return;

	// resolve hardwired debug buttons

	if (button < START_ACTION_BUTTONS)
	{
		if (to_send->GetControlType()!=CONTROL_FRONTEND)
		{
			GAME.ReceiveButtonAction(this,button, ana_val) ;
		}
		else
			to_send->ReceiveButtonAction(this,button, ana_val) ;
	}
	else
	{
		if ((RECONNECT_INTERFACE[0].GetState() != CReconnectInterface::OK) ||
			(RECONNECT_INTERFACE[1].GetState() != CReconnectInterface::OK))
		{
			if ((to_send != &RECONNECT_INTERFACE[0]) &&
				(to_send != &RECONNECT_INTERFACE[1]))
				return;
		}

		if ((mPausedBeforeMap == FALSE) || (to_send->CanBeControlledWhenInPause() == TRUE ))
		{
			to_send->ReceiveButtonAction(this,button, ana_val) ;
		}

	}
}


//******************************************************************************************
void		CController::SetToControl(IController* to_control)
{
	CActiveReader<IController>* to_control_r = new (MEMTYPE_CONTROLLER) CActiveReader<IController>(to_control);
	mToControlStack.Add(to_control_r) ;
}


//******************************************************************************************
void	CController::SetReverseLookYAxis(BOOL val) 
{ 
	mReverseLookYAxis = val ; 

	// what player ownes this controller ??
	if (GAME.GetController(0) && GAME.GetController(0)== this)
	{
		CAREER.SetInvertYAxis(0, val) ;
	}

	if (GAME.GetController(1) && GAME.GetController(1)== this)
	{
		CAREER.SetInvertYAxis(1, val) ;
	}
}


//******************************************************************************************
void		CController::RelinquishControl()
{
	CActiveReader<IController>* first = mToControlStack.First() ;
	if (first)
	{
		mToControlStack.Remove(first) ;
		delete first;
	}
	else
	{
		LOG.AddMessage("FATAL ERROR: Controller stack empty to call to 'RelinquishControl'") ;
		return ;
	}

	first = mToControlStack.First() ;
	if (first == NULL)
	{
		LOG.AddMessage("FATAL ERROR: stack empty to call to 'RelinquishControl'") ;
		return ;
	}

}

void CController::SetVibration(float inValue, int player)
{
	ASSERT(player == 0 || player == 1);

	if (GAME.GetGameState() != GAME_STATE_PLAYING)
	{
		if (inValue != 0.f)
			return;
	}

	if (CAREER.GetVibration(player)) DeviceSetVibration(inValue);
	else							 DeviceSetVibration(0.f    );
}
