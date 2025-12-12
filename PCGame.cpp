#include	"Common.h"

#if TARGET == PC

//#include	"DX.h"
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
#include	"wall.h"
#include    "scheduledevent.h"
#include	"DebugText.h"
#include	"music.h"
#include	"spriterenderer.h"
#include	"renderinfo.h"
#include	"Player.h"
#include    "World.h"

//*******************************************************************************
CPCGame		GAME;

//*******************************************************************************
void	CPCGame::DrawGameStuff()
{
	//*****************************************************
	//** Screen Grab?
	static	BOOL grabbing	= FALSE;

	if(PLATFORM.KeyOnce(VK_F8))
		grabbing = !grabbing;

	if(grabbing)
	{
		static int frameno=0;
		if(frameno==0) CreateDirectory(ToTCHAR("grabs"),NULL);

		// Skip existing files

		bool framenumok=false;

		while (!framenumok)
		{
			char name[32];
			sprintf(name,"grabs\\scr%.4d.tga",frameno);
			FILE *f=fopen(name,"r");
			if (f)
			{
				fclose(f);
				frameno++;
			}
			else
				framenumok=TRUE;
		}

		LT.DumpScreen(frameno++);
	}

	//*****************************************************
	//** Assorted Debug stuff 

	DBT.Out("World %d,  Time = %2.2f, Event list size = %d(%d) (proc last frame=%d), Num Things = %d, ",GetCurrentLevel(), EVENT_MANAGER.GetTime(), EVENT_MANAGER.TotalEvents(), CScheduledEvent::GetNumCreated(), EVENT_MANAGER.GetNumEventsProcessedInLastUpdate(), WORLD.GetThingNB().Size());

	if (mFixedFrameRate)
	{
		DBT.Out(", (fixed rate gc) %2.2f FPS ", PLATFORM.GetFPS());
	}
	else
	{
		DBT.Out(" %2.2f FPS ", PLATFORM.GetFPS());
	}

	CPlayer* p = GetPlayer(0) ;
	if (p)
	{
		if (p->IsGod())
		{
			DBT.Out("\nGOD mode Score = %0d", mScore);
		}
		else
		{
			CBattleEngine* player_unit = p->GetBattleEngine() ;
			{
				if(player_unit)
				{
					DBT.Out("\n Health = %2.2f Energy = %2.2f Score = %0d", player_unit->GetLife() , player_unit->GetEnergy(), mScore);
					
					if (player_unit->IsAugActive())
						DBT.Out("\n Aug Bar = %2.2f Aug Active", player_unit->GetAugValue());
					else
						DBT.Out("\n Aug Bar = %2.2f", player_unit->GetAugValue());
				}
			}
		}
	}

    PLATFORM.Font( FONT_DEBUG )->DrawText( 0, 0, 0xffffff80, ToWCHAR(DBT.GetPtrAndReset()), 0L );
	DBT.GetPtrAndReset();
	LOG.Render();

	//!JCL-MAP
/*	if (!(MAP.GetGenerator()->AreShadowsValid()))
	{
		PLATFORM.DrawDebugText( 451, 461, 0xff000000, "Shadows out of date.", 0L );
		PLATFORM.DrawDebugText( 450, 460, 0xff7f7f7f, "Shadows out of date.", 0L );
	}*/

	// more hacks

	if ((mGameState==GAME_STATE_LEVEL_LOST) || (mGameState==GAME_STATE_LEVEL_WON))
	{
		mWinLoseScreenAlpha+=16;
		if (mWinLoseScreenAlpha>0xA0)
			mWinLoseScreenAlpha=0xA0;			

		DWORD alpha=mWinLoseScreenAlpha<<24;

		RENDERINFO.SetFogEnabled(false);
		if (mGameState==GAME_STATE_LEVEL_WON)
			CSPRITERENDERER::DrawColouredSprite(0,0,0.001f,mWinScreen,alpha | 0x00FFFFFF,(640.0f/1024.0f),(480.0f/512.0f));
		else
			CSPRITERENDERER::DrawColouredSprite(0,0,0.001f,mLoseScreen,alpha | 0x00FFFFFF,(640.0f/1024.0f),(480.0f/512.0f));

		int x,y;
		char buffer[256];

		x=100;
		y=100;

		CPlayer *p=GetPlayer(0);

		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x,(float) y,alpha | 0x00FFFFFF,ToWCHAR("Total mission time :"));
		sprintf(buffer,"%.2fs",((float) p->GetStat(PS_TIMEASJET)+p->GetStat(PS_TIMEASWALKER))/20.0f);
		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x+400,(float) y,alpha | 0x00FFFFFF,ToWCHAR(buffer));
		y+=32;		

		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x,(float) y,alpha | 0x00FFFFFF,ToWCHAR("Time in jet mode :"));
		sprintf(buffer,"%.2fs",((float) p->GetStat(PS_TIMEASJET))/20.0f);
		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x+400,(float) y,alpha | 0x00FFFFFF,ToWCHAR(buffer));
		y+=32;

		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x,(float) y,alpha | 0x00FFFFFF,ToWCHAR("Time in walker mode :"));
		sprintf(buffer,"%.2fs",((float) p->GetStat(PS_TIMEASWALKER))/20.0f);
		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x+400,(float) y,alpha | 0x00FFFFFF,ToWCHAR(buffer));
		y+=32;

		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x,(float) y,alpha | 0x00FFFFFF,ToWCHAR("Rounds fired :"));
		sprintf(buffer,"%d",p->GetStat(PS_ROUNDSFIRED));
		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x+400,(float) y,alpha | 0x00FFFFFF,ToWCHAR(buffer));
		y+=32;

		if (p->GetStat(PS_ROUNDSFIRED)>0)
		{
			PLATFORM.Font( FONT_NORMAL )->DrawText((float) x,(float) y,alpha | 0x00FFFFFF,ToWCHAR("Accuracy :"));
			sprintf(buffer,"%.2f%%",((float) p->GetStat(PS_ROUNDSHIT)*100.0f) / ((float) p->GetStat(PS_ROUNDSFIRED)));
			PLATFORM.Font( FONT_NORMAL )->DrawText((float) x+400,(float) y,alpha | 0x00FFFFFF,ToWCHAR(buffer));
			y+=32;
		}

		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x,(float) y,alpha | 0x00FFFFFF,ToWCHAR("Damage taken :"));
		sprintf(buffer,"%.2f",((float) p->GetStat(PS_DAMAGETAKEN)) / 256.0f);
		PLATFORM.Font( FONT_NORMAL )->DrawText((float) x+400,(float) y,alpha | 0x00FFFFFF,ToWCHAR(buffer));
		y+=32;

		if (p->GetStat(PS_CHEATED)>0)
		{
			PLATFORM.Font( FONT_NORMAL )->DrawText((float) x,(float) y,alpha | 0x00FF0000,ToWCHAR("CHEATER!"));
			y+=32;
		}		
		
		RENDERINFO.SetFogEnabled(true);		
	}

//	DBT.Out("\nFract=%f - time=%f, basetime=%f, emt=%f\n",GetFrameRenderFraction(),mFrameTime,mBaseTime,EVENT_MANAGER.GetTime());
}

void CPCGame::DumpTimeRecords()
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

void CPCGame::AddTimeStatus(char *s)
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