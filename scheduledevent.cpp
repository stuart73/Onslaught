// Scheduled_event.cpp: implementation of the Scheduled_event class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
#include "scheduledevent.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int CScheduledEvent::mNumCreated = 0 ;



//******************************************************************************************
void CScheduledEvent::Set(int event_num, const float& time, CMonitor* to_call, CMonitor* data)
{
	CEvent::SetNum(event_num);
	mTime = time;
	mToCall.SetReader(to_call),
	mBeingReused = 0 ;
	mData.SetReader(data) ;
}

 


 //******************************************************************************************
CScheduledEvent::~CScheduledEvent()
{
	mNumCreated--;
}


