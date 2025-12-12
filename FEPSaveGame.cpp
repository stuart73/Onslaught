#include	"Common.h"
#include	"FEPSaveGame.h"

#include	"Frontend.h"
#include	"FEMessBox.h"
#include	"Renderinfo.h"
#include	"SpriteRenderer.h"
#include	"MemoryCard.h"
#include	"career.h"
#include	"cliparams.h"

//*********************************************************************************

BOOL	CFEPSaveGame::Init()
{
	mCX = 0;
	mCY = 0;

	return TRUE;
}

//*********************************************************************************
void	CFEPSaveGame::Shutdown()
{
}

//*********************************************************************************
void	CFEPSaveGame::ButtonPressed(SINT button,float val)
{
	switch (button)
	{
	case BUTTON_FRONTEND_MENU_UP:
		mCY --;
		if (mCY < 0)
			mCY = 0;
		FRONTEND.PlaySound(FES_MOVE);
		break;

	case BUTTON_FRONTEND_MENU_DOWN:
		mCY ++;
		if (mCY > 0xFF)
			mCY = 0xFF;
		FRONTEND.PlaySound(FES_MOVE);
		break;

	case BUTTON_FRONTEND_MENU_LEFT:
		mCX --;
		if (mCX < 0)
			mCX = 0;
		FRONTEND.PlaySound(FES_MOVE);
		break;

	case BUTTON_FRONTEND_MENU_RIGHT:
		mCX ++;
		if (mCX > 0xFF)
			mCX = 0xFF;
		FRONTEND.PlaySound(FES_MOVE);
		break;

	case BUTTON_FRONTEND_MENU_SELECT:
		FRONTEND.PlaySound(FES_SELECT);
		break;

	case BUTTON_FRONTEND_MENU_BACK:
		FRONTEND.PlaySound(FES_BACK);
		FRONTEND.SetPage(FEP_DEVSELECT, 0);
		break;
	};
}

//*********************************************************************************

void	CFEPSaveGame::Process(EFEPState state)
{
	if (state != FEPS_INACTIVE)
		MEMORYCARD.Update();
		
	if ((state==FEPS_ACTIVE) && (!FEMESSBOX.BeingDisplayed()))
	{		
#if 0 // Jan's using this to automatically create hundreds of savegames
		// Attempt to kick off the save game process
		while (1)
		{
			CreateSave();
			
			int addpos = 0;
			
			// implement an adder! what a man.
			while(1)
			{
				if (mSaveGameName[addpos] == 'z')
				{
					mSaveGameName[addpos] = 'a';
					
					addpos++;
				}
				else
				{
					// We can just increment this.
					mSaveGameName[addpos]++;
					
					// and we're done.
					break;
				}
			}
		}
#else
		CreateSave();
#endif
	}

	// Are there any pending results from messagebox questions that we care about?
	if ( FEMESSBOX.BeingDisplayed() )
	{
		switch(FEMESSBOX.GetQuestion())
		{
		default:
			break;

		case FEMB_Q_OVERWRITE:
			// we're asking if they want to overwrite an existing savegame.
			switch (FEMESSBOX.GetFinalChoice())
			{
			case FEMB_YES:
				// Well, so much for that savegame! Delete it and try saving again.

				if (mExistingSave!=-1)
				{
#if TARGET == XBOX
					// let's get the name of the card to which we're trying to delete
					MEMORYCARD.GetCardOrPreviousCardName2(FRONTEND.GetMemoryCardNumber(), FRONTEND.mFEPSaveGame.mSavedToCardName);
#endif
	
					// ***TEXT
					FRONTEND.StartMCOperation(FRONTEND.mFrontEndText.GetText(FEX_DELETING,FRONTEND.GetMemoryCardNumber()
#if TARGET == PS2
						+1
#endif
						));
					ASSERT(mExistingSaveName[0]);
					MEMORYCARD.DeleteSave(FRONTEND.GetMemoryCardNumber(),mExistingSave,mExistingSaveName);
					FRONTEND.EndMCOperation();

					mExistingSave=-1;					
				}
				else
				{
					ASSERT(!mExistingSaveName[0]);
				}

				// and kick the process off again.
				CreateSave();
				break;

			case FEMB_NO:
				// Go back to choosing a save game name.
				mExistingSave=-1;
				mExistingSaveName[0] = 0;
				FRONTEND.SetSaveMode(TRUE);
				FRONTEND.SetPage(FEP_VIRTUAL_KEYBOARD, 20);
				break;
			}
			break;
		}
	}

	RENDERINFO.SetFogEnabled(false);
}

//*********************************************************************************
void	CFEPSaveGame::RenderPreCommon(float transition, EFrontEndPage dest)
{
	FRONTEND.DrawStandardVideoBackground(transition, 0x3fffffff, dest);
}
//*********************************************************************************
void	CFEPSaveGame::Render(float transition, EFrontEndPage dest)
{
	FRONTEND.DrawSlidingTextBordersAndMask(transition, dest);

	//*************************************************************************
	// Load game rendering, selection of device, etc.

	//***********************************************
	//** Help Text. But this page has nothing on it but the "save succeeded" dialog box. So no help text.

	//***********************************************
	//** Title Bar
	FRONTEND.DrawTitleBar(FET.GetText(FEX_SAVEGAME2), transition, dest);

	//*************************************************************************
	// BEA logo
	float tr = RangeTransition(transition, 0.75f, 1.f);
	FRONTEND.GetCommonPage()->RenderSmallBEALogo(tr);

	FRONTEND.GetCommonPage()->RenderHelpText(FEH_CONTINUE, transition);
}

//*********************************************************************************

void	CFEPSaveGame::TransitionNotification(EFrontEndPage from)
{
	mStartTime = PLATFORM.GetSysTimeFloat() + 2.f;
}

//*********************************************************************************

void CFEPSaveGame::RemovedMUWhinge(EFETextHack reason)
{
	WCHAR whinge[200];
	WCHAR name[200];
	wcscpy( whinge, FET.GetText(reason, FRONTEND.GetMemoryCardNumber()
#if TARGET == PS2
		+1
#endif
		));

	// create the message. Luckily it takes a local copy of the text
	FEMESSBOX.Kill();
	FEMESSBOX.Create(320.0f, 240.0f, 400.0f, FO(100), whinge, PLATFORM.Font(FONT_SMALL), 0, 10, FALSE, FEMB_OPTION_A_CONTINUE, FALSE);
	FEMESSBOX.SetOnDismiss(FEP_DEVSELECT, 0);
}

//*********************************************************************************
//*********************************************************************************

// Initialise the data and variables required to save the game, and kick off the
// save process

static void ask_if_you_want_to_overwrite()
{
	FEMESSBOX.Create(320.0f, 240.0f, 300.0f, FO(100), FET.GetText(FEX_OVERWRITE_SAVEGAME), PLATFORM.Font(FONT_SMALL), 0, 10, FALSE, FEMB_OPTION_YESNO, FEMB_Q_OVERWRITE);
}

//*********************************************************************************

void CFEPSaveGame::AskIfYouWantToDelete(BOOL career_in_progress, BOOL because_4096, BOOL no_space_for_bea)
{
	UINT question;
	WCHAR total_message[256];
	total_message[0] = 0;

	// We're only allowed to go to the dashboard if we've got no savegame info.
	// Though in truth, if we've got savegame info the memory card really shouldn't be full, because we're autosaving.
	// This could happen in you switch mem cards during a game i guess

#if TARGET == XBOX
	// even though it's fucking obvious because of the big red box round the currently highlighted device, I'll even say
	// what card it is.
	MEMORYCARD.GetCardName2(FRONTEND.GetMemoryCardNumber(), total_message);
	if (!total_message[0])
	{
		// it doesn't have a cool name. Give it a crap one.
		MEMORYCARD.GetCardName(FRONTEND.GetMemoryCardNumber(), total_message);
	}

	wcscat(total_message, ToWCHAR("                                           ")); // word wrap doesn't understand \n but does swallow spaces

	// Oh dear it all gets very complex.
	if (no_space_for_bea)
	{
		// The card's full, but there's no bea savegames to delete. So the only option is to go to the dashboard.
		// BUT, of course, that's not the end of the story, if we've got a career going to the dashboard would be bad.

		wcscat(total_message, FET.GetText(FEX_NO_SPACE_DASH));
		question = FEMB_Q_DASHBOARD;

		if (career_in_progress)
		{
			wcscat(total_message, ToWCHAR(" "));
			wcscat(total_message, FET.GetText(FEX_THAT_WILL_KILL_CAREER));
		}
		else
		{
			// no career in progress, so going to the dashboard is no problem.
		}
	}
	else
	{
		// we've got some of our own savegames to delete. Let's give them the option to delete these savegames.
		// But how many?
		if (because_4096)
		{
			wcscat(total_message, FET.GetText(FEX_4096_DELETE));
		}
		else
		{
			wcscat(total_message, FET.GetText(FEX_NO_SPACE_THERE));
		}

		question = FEMB_Q_DELETE_SAVES;
	}
#else
	if (because_4096) wcscat(total_message, FET.GetText(FEX_TOOMANYSAVES_PS2,FRONTEND.GetMemoryCardNumber()+1));
	else			  wcscat(total_message, FET.GetText(FEX_NOSPACE_WITHSAVES_PS2,FRONTEND.GetMemoryCardNumber()+1));

	question = FEMB_Q_DELETE_SAVES;
#endif

	FEMESSBOX.Create(320.0f, 240.0f, 300.0f, FO(100), total_message, PLATFORM.Font(FONT_SMALL), 0, 10, FALSE, FEMB_OPTION_YESNO, question);
}

//*********************************************************************************

void	CFEPSaveGame::CreateSave()
{
	ASSERT(mSaveGameName[0]);

	BOOL present,formatted;
	int bytesfree,bytestotal;
	int numsaves;

#if TARGET == XBOX
	// let's get the name of the card to which we're trying to save
	MEMORYCARD.GetCardOrPreviousCardName2(FRONTEND.GetMemoryCardNumber(), mSavedToCardName);
#endif
	
	MEMORYCARD.Update();

	MEMORYCARD.GetCardInfo(FRONTEND.GetMemoryCardNumber(),&present,&formatted,&bytesfree,&bytestotal);
	MEMORYCARD.GetNumSaves(FRONTEND.GetMemoryCardNumber(),&numsaves);

	if ((!present) || (!formatted))
	{
		// Card removed or invalid, so we'll return to device selection. But whinge first.
		RemovedMUWhinge(FEX_REMOVEDSAVE);
		return;
	}

	// Are we allowed to overwrite an existing savegame? This will happily reset the counter if we're not autosaving as well.
	bool memory_card_changed = MEMORYCARD.CardBeenUnpluggedSinceLastTimeIAsked(FRONTEND.GetMemoryCardNumber());

	if (memory_card_changed && FRONTEND.GetAutoSave() == FRONTEND.AUTO_SAVE_NORMAL)
	{
		// Oh dear, the memory card has changed. Refuse all this saving business, and go to selecting location.
		FEMESSBOX.Create(320.0f, 240.0f, 300.0f, FO(100), FET.GetText(FEX_AUTOSAVE_SCARED), PLATFORM.Font(FONT_SMALL), 0, 10, FALSE, FEMB_OPTION_OK, FALSE);
		FRONTEND.SetAutoSave(FRONTEND.AUTO_SAVE_NOT);
		FEMESSBOX.SetOnDismiss(FEP_DEVSELECT, 50);
		return;
	}

	bool allowed_overwrite;
	
	ASSERT(FRONTEND.GetAutoSave() != FRONTEND.AUTO_SAVE_PRETEND && "Jan says if we're pretending to autosave we shouldn't be saving.");

	if (FRONTEND.GetAutoSave() == FRONTEND.AUTO_SAVE_NORMAL)
	{
		// we're only allowed to overwrite if the memory card hasn't been unplugged
		allowed_overwrite = !memory_card_changed;
	}
	else
	{
		// it's not an autosave, so overwriting is naughty.
		ASSERT(FRONTEND.GetAutoSave() == FRONTEND.AUTO_SAVE_NOT);
		allowed_overwrite = FALSE;
	}

	// Check for available space (again)

	int savesize;

	MEMORYCARD.GetSaveSize(CAREER.SizeOfSaveGame(),&savesize);

	if (((bytesfree<savesize) || (numsaves>4095)) && (!allowed_overwrite))
	{
		// No space available - complain

		// If there's no saves but no space, it must be other people's saves that are taking up the space
		AskIfYouWantToDelete(CAREER.InProgress(), (numsaves>4095), numsaves == 0);
		return;
	}

	// Let's remember if this game actually had any game in it.
	// We have to remember it now because it's edited by the act of saving the game.
	mCareerWasInProgress = CAREER.InProgress();
	
	// Check to see if the save already exists

	int savenumber=-1;

	MEMORYCARD.GetNumSaves(FRONTEND.GetMemoryCardNumber(),&numsaves);

	for (int i=0;i<numsaves;i++)
	{
		WCHAR savename[256];

		MEMORYCARD.GetSaveName(FRONTEND.GetMemoryCardNumber(),i,savename);
			
		if (wcscmp(mSaveGameName,savename)==0)
		{
			savenumber=i;
		}
	}

	if ((savenumber!=-1) && (!allowed_overwrite))
	{
		// File already exists and we weren't allowed to overwrite.
		
		FEMESSBOX.Kill();
		
		mExistingSave=savenumber;
		wcscpy(mExistingSaveName, mSaveGameName);
		
		ask_if_you_want_to_overwrite();

		return;
	}

	// ***TEXT
	EFETextHack text = FRONTEND.GetMemoryCardNumber() == -1? FEX_SAVING_HD: FEX_SAVING;

	FRONTEND.StartMCOperation(FRONTEND.mFrontEndText.GetText(text,FRONTEND.GetMemoryCardNumber()
#if TARGET == PS2
		+1
#endif
		));
	
	int res=MEMORYCARD.CreateSave(FRONTEND.GetMemoryCardNumber(),mSaveGameName,&savenumber, allowed_overwrite);

	FRONTEND.EndMCOperation();

	if (res==MCE_FILEEXISTS)
	{
		// File already exists and we weren't allowed to overwrite.

		FEMESSBOX.Kill();

		mExistingSave=savenumber;
		wcscpy(mExistingSaveName, mSaveGameName);

		ask_if_you_want_to_overwrite();
	}
	else if (res==MCE_SUCCESS)
	{

		// Save file creation succeeded - write data

		int buffersize=CAREER.SizeOfSaveGame();
		BYTE *buffer=new (MEMTYPE_SCRATCHPAD) BYTE[buffersize];
		
		CAREER.Save((char *)buffer);

		res=MEMORYCARD.WriteSave(FRONTEND.GetMemoryCardNumber(),savenumber,mSaveGameName,buffer,buffersize);

		FEMESSBOX.Kill();		

		if (res==MCE_SUCCESS)
		{
			// Save complete!

			// Did we save a career in progress or just the blank start?
			EFETextHack reason;
			if (mCareerWasInProgress) reason = FEX_SAVEDONE;
			else					  reason = FEX_FRESHSAVEDONE;
			
			FEMESSBOX.Kill();
			FEMESSBOX.Create(320.f, 240.f, 400, FO(100), FET.GetText(reason), PLATFORM.Font(FONT_SMALL), 0, 10, FALSE, FEMB_OPTION_OK, FALSE);			
			FEMESSBOX.SetOnDismiss(FRONTEND.GetSuccessPage(), FRONTEND.GetSuccessTransTime());

			// and from now on we can use this slot.
			FRONTEND.SetAutoSave(CFrontEnd::AUTO_SAVE_NORMAL);

			// and this means that we've finally used the data in the virtual keyboard.
			FRONTEND.mFEPVirtualKeyboard.m_Fresh = false;
		}
		else if (res==MCE_NOFILE)
		{
			// No file (card removed)

			FEMESSBOX.Kill();
			RemovedMUWhinge(FEX_REMOVEDSAVE);
		}
		else if (res == MCE_CARDFULL)
		{
			// oh dear there goes the card space. We'd better lose the partial save though.
			MEMORYCARD.DeleteSave(FRONTEND.GetMemoryCardNumber(),savenumber,mSaveGameName);
			FEMESSBOX.Kill();
			int num_saves;
			MEMORYCARD.GetNumSaves(FRONTEND.GetMemoryCardNumber(), &num_saves);
			AskIfYouWantToDelete(mCareerWasInProgress, FALSE, num_saves == 0);
		}
		else
		{
			// Generic save error

			WCHAR msg[256];

			wcscpy( msg, FRONTEND.mFrontEndText.GetText(FEX_SAVEERROR,FRONTEND.GetMemoryCardNumber()
#if TARGET == PS2
				+1
#endif
				));
			
			FEMESSBOX.Kill();
			FEMESSBOX.Create(320.f, 240.f, 400, FO(100),msg, PLATFORM.Font(FONT_SMALL), 0, 10, FALSE, FEMB_OPTION_A_CONTINUE, FALSE);
			FRONTEND.SetPage(FEP_DEVSELECT, 0);
		}

		SAFE_DELETE(buffer);
	}
	else if (res==MCE_CARDFULL || res==MCE_TOO_MANY_SAVES)
	{
		// We're out of space! Better get someone to clear the card.
		if (savenumber != -1) 
			MEMORYCARD.DeleteSave(FRONTEND.GetMemoryCardNumber(),savenumber,mSaveGameName);
		FEMESSBOX.Kill();
		int num_saves;
		MEMORYCARD.GetNumSaves(FRONTEND.GetMemoryCardNumber(), &num_saves);
		AskIfYouWantToDelete(mCareerWasInProgress, res == MCE_TOO_MANY_SAVES, num_saves == 0);
	}
	else
	{
		// Save failed for some reason

		FEMESSBOX.Kill();

		MEMORYCARD.Update();

		// Was the card removed?

		MEMORYCARD.GetCardInfo(FRONTEND.GetMemoryCardNumber(),&present,&formatted,NULL,NULL);

		if ((!present) || (!formatted))
		{
			RemovedMUWhinge(FEX_REMOVEDSAVE);
		}
		else
		{
			WCHAR msg[256];
			
			wcscpy( msg, FRONTEND.mFrontEndText.GetText(FEX_SAVEERROR,FRONTEND.GetMemoryCardNumber()
#if TARGET == PS2
				+1
#endif
				));

			FEMESSBOX.Kill();
			FEMESSBOX.Create(320.f, 240.f, 400, FO(100), msg, PLATFORM.Font(FONT_SMALL), 0, 10, FALSE, FEMB_OPTION_A_CONTINUE, FALSE);
			FRONTEND.SetPage(FEP_DEVSELECT, 0);
		}
	}
}

char cheatname[4][256];

BOOL CFEPSaveGame::IsCheatActive(int cheatno) 
{
	char mungedname[256];

	strcpy(cheatname[0],"105770Y2");			// Unlock all goodies
	strcpy(cheatname[1],"!EVAH!");				// Unlock all levels (Wai! Rampant UKG-isms!)
	strcpy(cheatname[2],"V3R5ION");				// Show version number
	strcpy(cheatname[3],"B4K42");				// God mode available

	strcpy(mungedname,FromWCHAR(mSaveGameName));

#if TARGET==PS2
	if (CLIPARAMS.mDevKit)
		return(TRUE);
#endif

	if (strstr(mungedname,cheatname[cheatno])!=NULL)
		return(TRUE);
	else
		return(FALSE);
}
