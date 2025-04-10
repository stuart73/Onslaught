// PCController.h: interface for the PCController class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PCCONTROLLER_H
#define PCCONTROLLER_H

#include "Controller.h"
#include "DX.h"

// pc specific controllers


class CPCController : public CController
{
public:
	CPCController(IController* to_control,int padnumber, int configuration = 1, BOOL reverse_look_y_axis = FALSE) ;
	
protected:
 
	virtual int GetJoyButtonOnce(int pad_number, int button) { return LT.JoyButtonOnce(pad_number, button) ; }
	virtual int GetJoyButtonOn(int pad_number, int button) { return LT.JoyButtonOn(pad_number, button) ; }
	virtual int GetJoyButtonRelease(int pad_number, int button) { return LT.JoyButtonRelease(pad_number, button) ; }

	virtual BOOL	GetKeyOnce(int pad_number, int key) { return PLATFORM.KeyOnce(key) ; }
	virtual BOOL	GetKeyOn(int pad_number, int key) { return PLATFORM.KeyOn(key) ; }
	virtual void	DeviceSetVibration(float){}

	virtual float	GetJoyAnalogueLeftX(int pad_number);
	virtual float	GetJoyAnalogueLeftY(int pad_number);
	virtual	float	GetJoyAnalogueRightX(int pad_number);
	virtual float	GetJoyAnalogueRightY(int pad_number);

	virtual void RecordControllerState() ;
	virtual void ReadControllerState() ;
};

#endif // PCCONTROLLER_H
