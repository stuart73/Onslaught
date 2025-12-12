
#include "common.h"
#include "activereader.h"
#include "debuglog.h"
#include "Event.h"

//*************************************************************************************************
void CGenericActiveReader::SetReader(CMonitor* to_read)
{
	if (to_read == mToRead) return ;

	if (mToRead)
	{
		mToRead->RemoveDeletionEvent(this) ;
	}
	mToRead = to_read ; 
	if (mToRead)
	{
		MEM_MANAGER.DoesExist(mToRead) ;
		mToRead->AddDeletionEvent(this);
	}
}
