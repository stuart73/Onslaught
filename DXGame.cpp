#include	"Common.h"

#ifdef _DIRECTX

#include	"Game.h"
#include	"Camera.h"
#include	"Engine.h"
#include	"Profile.h"
#include	"Map.h"
#include	"EventManager.h"
#include	"SoundManager.h"
#include	"debuglog.h"
#include    "console.h"
#include	"debris.h"
#include    "scheduledevent.h"
#include	"DebugText.h"
#include	"music.h"
#include	"spriterenderer.h"
#include	"renderinfo.h"
#include	"Player.h"
#include    "World.h"
#include	"CLIParams.h"
#include    "text.h"



//*******************************************************************************
CDXGame		GAME;

void CDXGame::DumpTimeRecords()
{
#ifdef DEBUG_TIMERECORDS
	FILE *f=fopen("c:\\timerec.txt","wt");
	int r=mCurrentTR;
	int oldr=mCurrentTR;
	char buffer[1024],tbuf[1024];

	sprintf(buffer,"Base time\tStart offset\tTime taken\tRenders\tRender 1f\tRender 2f\tRender 3f\tRender 4f\tFrame length\tFPS rate\tStatus\n");
	fwrite(buffer,strlen(buffer),1,f);

	for (int i=0;i<TIMERECORDS;i++)
	{	
		sprintf(buffer,"%f\t%f\t%f\t%d\t",
				mTimeRecord[r].basetime,
			    mTimeRecord[r].starttimeoffset,mTimeRecord[r].timetaken,
				mTimeRecord[r].renders);
		if (mTimeRecord[r].renders>0)
			sprintf(tbuf,"%f\t",mTimeRecord[r].renderfraction[0]+(1-mTimeRecord[oldr].renderfraction[mTimeRecord[oldr].renders-1]));
		else
			sprintf(tbuf,"-\t\t");
		strcat(buffer,tbuf);
		if (mTimeRecord[r].renders>1)
			sprintf(tbuf,"%f\t",mTimeRecord[r].renderfraction[1]-mTimeRecord[r].renderfraction[0]);
		else
			sprintf(tbuf,"-\t\t");
		strcat(buffer,tbuf);
		if (mTimeRecord[r].renders>2)
			sprintf(tbuf,"%f\t",mTimeRecord[r].renderfraction[2]-mTimeRecord[r].renderfraction[1]);
		else
			sprintf(tbuf,"-\t\t");
		strcat(buffer,tbuf);
		if (mTimeRecord[r].renders>3)
			sprintf(tbuf,"%f\t",mTimeRecord[r].renderfraction[3]-mTimeRecord[r].renderfraction[2]);
		else
			sprintf(tbuf,"-\t\t");
		strcat(buffer,tbuf);
		
		sprintf(tbuf,"%f\t%f\t%s\n",
				mTimeRecord[r].framelength,
				mTimeRecord[r].fps,mTimeRecord[r].status);
		strcat(buffer,tbuf);
		
		fwrite(buffer,strlen(buffer),1,f);
		
		oldr=r;
		r++;
		if (r>=TIMERECORDS)
			r=0;		
	}

	fclose(f);

	f=fopen("c:\\renderrec.txt","wt");
	r=mCurrentRR;
	oldr=mCurrentRR;
	
	sprintf(buffer,"End time\tEnd time delta\tTime taken\tFrame code\tRender number\tDelta ratio\tIdeal DR\n");
	fwrite(buffer,strlen(buffer),1,f);
	
	for (i=0;i<TIMERECORDS;i++)
	{	
		sprintf(buffer,"%f\t%f\t%f\t%f\t%d\t%f\t%f\n",
			mRenderRecord[r].renderendtime,
			mRenderRecord[r].renderendtime-mRenderRecord[oldr].renderendtime,
			mRenderRecord[r].renderlength,
			mRenderRecord[r].renderframetime,
			mRenderRecord[r].rendernumber,
			(mRenderRecord[r].renderendtime-mRenderRecord[oldr].renderendtime)/(mRenderRecord[r].renderframetime-mRenderRecord[oldr].renderframetime),
			(mRenderRecord[r].renderstarttime-mRenderRecord[oldr].renderstarttime)/(mRenderRecord[r].renderframetime-mRenderRecord[oldr].renderframetime));

		fwrite(buffer,strlen(buffer),1,f);
		
		oldr=r;
		r++;
		if (r>=TIMERECORDS)
			r=0;		
	}
	
	fclose(f);
#endif
}

void CDXGame::AddTimeStatus(char *s)
{
#ifdef DEBUG_TIMERECORDS
	if ((strlen(mTimeRecord[mCurrentTR].status)+strlen(s))<255)
	{
		strcat(mTimeRecord[mCurrentTR].status,s);
		strcat(mTimeRecord[mCurrentTR].status," ");
	}
	else
	{
		if (strlen(mTimeRecord[mCurrentTR].status)<250)
			strcat(mTimeRecord[mCurrentTR].status,"...");
	}
#endif
}

//*******************************************************************************

/*

	if (mFixedFrameRate)
	{
		float time = (float)timeGetTime() * 0.001f;
		if (time - mLastUpdateTime > MAX_UPDATE_TIME) 
		{
			mLastUpdateTime = time ;
			GAME.Update() ;
		}
	}
	else
	{
		GAME.Update() ;
	}

*/

#endif