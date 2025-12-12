// XBox memory card access

#include "Common.h"

#if TARGET==XBOX

#include "MemoryCard.h"
#include "resourceaccumulator.h"
#include "d3d8.h"
#include "texture.h"
#include "xboxdx.h"
#include "xgraphics.h"
#include "xbox.h"
#include "frontend.h"

#define SAVE_FILE_NAME "SAVE.DAT"
#define MAX_NAME_LEN 40

#define CLAMP_SIZE(n) {if ((n) > 50000 * XBOX_BLOCK_SIZE) (n) = 50000 * XBOX_BLOCK_SIZE;}

//---------------------------------------------------------------------------

class CXBoxSaveHeader
{
public:
	DWORD				mMagic;
	DWORD				mVersion;
	XCALCSIG_SIGNATURE	mSig;
	void				CreateHeader(BYTE *data,DWORD datasize);
	BOOL				CheckHeader(BYTE *data,DWORD datasize);
};

//---------------------------------------------------------------------------

void CXBoxSaveHeader::CreateHeader(BYTE *data,DWORD datasize)
{
	HANDLE sigh=XCalculateSignatureBegin(0);
	XCalculateSignatureUpdate(sigh,data,datasize);
	XCalculateSignatureEnd(sigh,&mSig);

	mMagic=MKID("NEKO");
	mVersion=100;
}

//---------------------------------------------------------------------------

BOOL CXBoxSaveHeader::CheckHeader(BYTE *data,DWORD datasize)
{
	if (mMagic!=MKID("NEKO"))
		return(FALSE);

	if (mVersion!=100)
		return(FALSE);

	XCALCSIG_SIGNATURE sig;
	
	HANDLE sigh=XCalculateSignatureBegin(0);
	XCalculateSignatureUpdate(sigh,data,datasize);
	XCalculateSignatureEnd(sigh,&sig);

	if (memcmp(&sig,&mSig,sizeof(mSig))!=0)
		return(FALSE);
	
	return(TRUE);
}

//---------------------------------------------------------------------------

void CXBoxMemoryCard::InitialiseOnce()
{
	// this state has to persist across levels so isn't reset.
	memset(mLastAskedHash, 0, sizeof(mLastAskedHash));

	for (int i=0;i<TOTAL_XBOX_CARDS;i++)
	{
		mCardNeedsUpdating[i]=MODULE_INIT;
		for (int j=0;j<MAX_MEMORY_CARD_SAVES;j++)
		{
			mCardSaveName[i][j]=NULL;
		}
	}

	mRememberedInsertedCards = mRememberedRemovedCards = 0;
	mCurrentDeviceBitmap=0;
}

//---------------------------------------------------------------------------

void CXBoxMemoryCard::Initialise()
{
	mSaveImage=CTEXTURE::GetTextureByName("FrontEnd\\v2\\FE_XB_SaveGame.tga",TEXFMT_UNKNOWN,TEX_NORMAL,1);
}

//---------------------------------------------------------------------------

void CXBoxMemoryCard::Shutdown()
{
	SAFE_RELEASE(mSaveImage);
	for (int i=0;i<TOTAL_XBOX_CARDS;i++)
	{
		for (int j=0;j<MAX_MEMORY_CARD_SAVES;j++)
		{
			SAFE_DELETE_ARRAY(mCardSaveName[i][j]);
		}
	}
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::GetNumCards(int *num)
{
	*num=8;
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

BOOL CXBoxMemoryCard::IsHDDAvailable()
{
	return(TRUE);
}

//---------------------------------------------------------------------------

int	CXBoxMemoryCard::GetCardType(int card,int *cardtype)
{
	if (card==-1)
		*cardtype=MCT_HDD;
	else
		*cardtype=MCT_CARD;

	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

DWORD CXBoxMemoryCard::GetPort(int card)
{
	if ((card==0) || (card==1))
		return(XDEVICE_PORT0);
	else if ((card==2) || (card==3))
		return(XDEVICE_PORT1);
	else if ((card==4) || (card==5))
		return(XDEVICE_PORT2);
	else
		return(XDEVICE_PORT3);
}

//---------------------------------------------------------------------------

DWORD CXBoxMemoryCard::GetSlot(int card)
{
	if ((card==0) || (card==2) || (card==4) || (card==6))
		return(XDEVICE_TOP_SLOT);
	else 
		return(XDEVICE_BOTTOM_SLOT);
}

//---------------------------------------------------------------------------

void CXBoxMemoryCard::UpdateCardSaveInfo(int card,char drive)
{
	char path[16];
	sprintf(path,"%c:\\",drive);
	XGAME_FIND_DATA data;

	mCardSaves[card]=0;

	HANDLE h=XFindFirstSaveGame(path,&data);

	if (h==INVALID_HANDLE_VALUE)
	{
		// Could not enumerate for some reason, or no games found
		return;
	}
	else
	{
		do
		{
			if (!mCardSaveName[card][mCardSaves[card]])
				mCardSaveName[card][mCardSaves[card]]=new (MEMTYPE_MEMORYCARD) WCHAR[MAX_NAME_LEN];

			wcsncpy(mCardSaveName[card][mCardSaves[card]],data.szSaveGameName, MAX_NAME_LEN - 1);
			mCardSaves[card]++;

			CONSOLE.PingLoadingScreen();
		}
		while (XFindNextSaveGame(h,&data));

		XFindClose(h);
	}
}

//---------------------------------------------------------------------------

void CXBoxMemoryCard::GetMountedCardFreeAndTotalBytes(int card, char drive)
{
	if (card == -1) card = TOTAL_XBOX_CARDS-1;

	ULARGE_INTEGER freebytes,totalbytes;

	char dirname[16];
	sprintf(dirname,"%c:\\",drive);
	
	GetDiskFreeSpaceEx(dirname,&freebytes,&totalbytes,NULL);

	int sfreebytes,stotalbytes;

	if (freebytes.HighPart!=0 || freebytes.LowPart > 0x7ffffff)
		sfreebytes=0x7FFFFFFF;
	else
		sfreebytes=freebytes.LowPart;

	if (totalbytes.HighPart!=0 || totalbytes.LowPart > 0x7ffffff)
		stotalbytes=0x7FFFFFFF;
	else
		stotalbytes=totalbytes.LowPart;

	CLAMP_SIZE(stotalbytes);
	CLAMP_SIZE(sfreebytes );

	mCardFormatted[card]=TRUE;
	mCardBytesTotal[card]=stotalbytes;
	mCardBytesFree[card]=sfreebytes;
}

//---------------------------------------------------------------------------

void CXBoxMemoryCard::UpdateCardInfo(int card)
{
	if (card==-1)
		card=(TOTAL_XBOX_CARDS-1);

	if (card==(TOTAL_XBOX_CARDS-1))
	{
		// Hard drive

		mCardPresent[card]=TRUE;
		mCardFormatted[card]=TRUE;
		mCardNeedsUpdating[card]=NO;
		wcscpy(mCardName[card],ToWCHAR(""));

		ULARGE_INTEGER freebytes,totalbytes;
		
		char dirname[16];
		sprintf(dirname,"U:\\");
		
		GetDiskFreeSpaceEx(dirname,&freebytes,&totalbytes,NULL);
		
		int sfreebytes,stotalbytes;
		
		if (freebytes.HighPart!=0 || freebytes.LowPart > 0x7fffffff)
			sfreebytes=0x7FFFFFFF;
		else
			sfreebytes=freebytes.LowPart;
		
		if (totalbytes.HighPart!=0 || totalbytes.LowPart > 0x7fffffff)
			stotalbytes=0x7FFFFFFF;
		else
			stotalbytes=totalbytes.LowPart;

		CLAMP_SIZE(stotalbytes);
		CLAMP_SIZE(sfreebytes );

		mCardBytesTotal[card]=stotalbytes;
		mCardBytesFree[card]=sfreebytes;

		UpdateCardSaveInfo(card,'U');
	}
	else
	{
		// Memory card

		mCardNeedsUpdating[card]=NO;

		DWORD connectedcards=mCurrentDeviceBitmap;

		if (card==0)
			connectedcards&=XDEVICE_PORT0_TOP_MASK;
		else if (card==1)
			connectedcards&=XDEVICE_PORT0_BOTTOM_MASK;
		else if (card==2)
			connectedcards&=XDEVICE_PORT1_TOP_MASK;
		else if (card==3)
			connectedcards&=XDEVICE_PORT1_BOTTOM_MASK;
		else if (card==4)
			connectedcards&=XDEVICE_PORT2_TOP_MASK;
		else if (card==5)
			connectedcards&=XDEVICE_PORT2_BOTTOM_MASK;
		else if (card==6)
			connectedcards&=XDEVICE_PORT3_TOP_MASK;
		else if (card==7)
			connectedcards&=XDEVICE_PORT3_BOTTOM_MASK;

		if (connectedcards==0)
		{
			// Not present

			mCardPresent[card]=FALSE;
			mCardFormatted[card]=FALSE;
			mCardBytesTotal[card]=0;
			mCardBytesFree[card]=0;
			mCardSaves[card]=0;

			// but we leave the old name in, in case someone wants it
		}
		else
		{
			// Present

			mCardPresent[card]=TRUE;
			
			char drive;
			
			DWORD res=XMountMU(GetPort(card),GetSlot(card),&drive);
			
			if (res!=ERROR_SUCCESS)
			{
				// Mount failed
				
				if (res==ERROR_DISK_FULL)
				{
					// MU full. Because of TCR 1.7-2-22 let's just pretend it's crap. Even though we've written all the code to allow
					// the user to go to the Dash and sort it out.

					// The TCR sentence is:
					// Xbox MUs that fail to mount must be visually distinct in storage selection UI and
					// must not be player selectable as a read/write target.

					mCardFormatted[card]=FALSE;
					mCardBytesTotal[card]=0;
					mCardBytesFree[card]=0;
					mCardSaves[card]=0;		
					wcscpy(mCardName[card],ToWCHAR(""));
					
				}
				else if (res==ERROR_DEVICE_NOT_CONNECTED)
				{
					// Removed
					mCardPresent[card]=FALSE;
					mCardFormatted[card]=FALSE;
					mCardBytesTotal[card]=0;
					mCardBytesFree[card]=0;
					mCardSaves[card]=0;	

					// but we leave the old name in, in case someone wants it
				}
				else if (res==ERROR_ALREADY_ASSIGNED)
				{
					// Eh? Oh dear...
					TRACE("Warning : XMount() retured ERROR_ALREADY_ASSIGNED!\n");
					mCardPresent[card]=FALSE;
					mCardFormatted[card]=FALSE;
					mCardBytesTotal[card]=0;
					mCardBytesFree[card]=0;		
					mCardSaves[card]=0;	

					// but we leave the old name in, in case someone wants it
				}
				else
				{
					char buf[256];
					sprintf(buf,"Error %d\n",res);
					TRACE(buf);
					// Unformatted or otherwise unusable
					mCardPresent[card]=TRUE;
					mCardFormatted[card]=FALSE;
					mCardBytesTotal[card]=0;
					mCardBytesFree[card]=0;		
					mCardSaves[card]=0;
					wcscpy(mCardName[card],ToWCHAR(""));
				}
			}
			else
			{
				// Mount succeeded

				GetMountedCardFreeAndTotalBytes(card, drive);
				
				// Get the user name for this MU

				if (XMUNameFromDriveLetter(drive,mCardName[card],MAX_MUNAME)!=ERROR_SUCCESS)
				{
					wcscpy(mCardName[card],ToWCHAR(""));
				}

				// Some of the characters in the memory card names aren't valid in our font because we've nicked them for special characters such
				// as joypad buttons.
				for (WCHAR *walker = mCardName[card]; *walker; walker++)
				{
					for (const WCHAR *lookup_walker = CBitmapFont::HIJACKED_FONT_ENTRIES; *lookup_walker; lookup_walker++)
					{
						if (*walker == *lookup_walker)
						{
							// this one here is an example.
							// Let's turn it into a null character.
							*walker = 0xdead; // pick any old value, as long as it's not in our font so the font draw will come up with the null character

							// and don't keep looking
							break;
						}
					}
				}

				UpdateCardSaveInfo(card,drive);
								
				if (XUnmountMU(GetPort(card),GetSlot(card))!=ERROR_SUCCESS)
				{
					TRACE("Warning : XUnmountMU failed!\n");
				}
			}
		}
	}
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::GetCardInfo(int card,BOOL *present,BOOL *formatted,int *bytesfree,int *bytestotal)
{
	int cardindex=card;
	if (card==-1)
		cardindex=TOTAL_XBOX_CARDS-1;

	if (present)
		*present=mCardPresent[cardindex];
	if (formatted)
		*formatted=mCardFormatted[cardindex];
	if (bytesfree)
		*bytesfree=mCardBytesFree[cardindex];
	if (bytestotal)
		*bytestotal=mCardBytesTotal[cardindex];	
	
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------
int CXBoxMemoryCard::GetCardName(int card,WCHAR *buffer)
{
	if (card==-1)
		wcscpy(buffer,FET.GetText(EFETextHack(FEX_XBOX_HARD_DISK)));
	else
		wcscpy(buffer,FET.GetText(EFETextHack(FEX_MU_1A + card)));
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------
int CXBoxMemoryCard::GetCardName2(int card,WCHAR *buffer)
{
	buffer[0] = 0;

	if (card != -1)
	{
		// mCardName may have something in it even if the card isn't present.
		// this will be the name of the previous
		// memory card, you can get at that with GetCardOrPreviousCardName2
		if (mCardPresent[card] && mCardName[card][0])
		{
			wcscpy(buffer,ToWCHAR(" ("));
			wcscat(buffer,mCardName[card]);
			wcscat(buffer,ToWCHAR(")"));
		}
		else
		{
		}
	}
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------
int CXBoxMemoryCard::GetCardOrPreviousCardName2(int card,WCHAR *buffer)
{
	buffer[0] = 0;

	if (card==-1)
		wcscpy(buffer,FET.GetText(EFETextHack(FEX_XBOX_HARD_DISK)));
	else
	{
		if (mCardName[card][0])
		{
			wcscpy(buffer,mCardName[card]);
		}
		else
		{
			wcscpy(buffer,FET.GetText(EFETextHack(FEX_MU_1A + card)));
		}
	}

	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::Format(int card)
{
	SASSERT(0,"CXBoxMemoryCard::Format() - Not appropriate on XBOX, you shouldn't get here.");
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::Unformat(int card)
{
	SASSERT(0,"CXBoxMemoryCard::Unformat() - Not appropriate on XBOX, you shouldn't get here.");
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::GetNumSaves(int card,int *num)
{
	int cardindex=card;
	if (card==-1)
		cardindex=TOTAL_XBOX_CARDS-1;
	*num=mCardSaves[cardindex];
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

int	CXBoxMemoryCard::GetSaveName(int card,int save,WCHAR *name)
{
	int cardindex=card;
	if (card==-1)
		cardindex=TOTAL_XBOX_CARDS-1;
	wcscpy(name,mCardSaveName[cardindex][save]);
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::WriteSaveImage(char *directory,CDXTexture *image)
{
    // Determine size of incoming texture
    D3DSURFACE_DESC SrcDesc;
    HRESULT hr = image->GetTexture()->GetLevelDesc( 0, &SrcDesc );

    if (FAILED(hr))
        return(MCE_FAILURE);
	
    // Generate a 64x64 DXT1 surface
    LPDIRECT3DSURFACE8 pDxt1GameImageSurface = NULL;
    hr = LT.GetDevice()->CreateImageSurface( 64, 64, D3DFMT_DXT1,&pDxt1GameImageSurface );

    if (FAILED(hr))
        return(MCE_FAILURE);
	
    // Copy the source into the DXT1 surface
    LPDIRECT3DSURFACE8 pSrcSurface = NULL;
    hr = image->GetTexture()->GetSurfaceLevel( 0, &pSrcSurface );
    hr = D3DXLoadSurfaceFromSurface( pDxt1GameImageSurface, NULL, NULL, pSrcSurface, NULL, NULL, D3DX_DEFAULT, D3DCOLOR( 0 ) );
    SAFE_RELEASE(pSrcSurface);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pDxt1GameImageSurface);
        return(MCE_FAILURE);
    }
	
    // Generate the save game image name
    CHAR strMetaDataFile[MAX_PATH];
    lstrcpyA(strMetaDataFile,directory);
    lstrcatA(strMetaDataFile,"saveimage.xbx");
	
    // Write the compressed texture out to an .XPR file
    hr=XGWriteSurfaceOrTextureToXPR( pDxt1GameImageSurface, strMetaDataFile, TRUE );
	
    SAFE_RELEASE(pDxt1GameImageSurface);
	
    if (FAILED(hr))
        return(MCE_FAILURE);
	
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

static int count_num_files(char drive)
{
	char path[16];
	sprintf(path, "%c:\\*.*", drive);

	WIN32_FIND_DATA data;

	HANDLE h = FindFirstFile(path, &data);

	int num_files = 0;

	while (h != INVALID_HANDLE_VALUE)
	{
		num_files++;

		if (!FindNextFile(h, &data)) break;
	}

	FindClose(h);

	return num_files;
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::CountNumHardDriveFiles()
{
	return count_num_files('U');
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::CreateSave(int card,WCHAR *name,int *save,BOOL allowed_overwrite)
{
	if (save)
		*save=-1;

	// Check to see if a save with that name already exists

	int cardindex=card;
	if (card==-1)
		cardindex=TOTAL_XBOX_CARDS-1;

	if (!allowed_overwrite)
	{
		for (int i=0;i<mCardSaves[cardindex];i++)
		{
			if (wcsicmp(mCardSaveName[cardindex][i],name)==0)
			{
				if (save)
					*save=i; // Return number of existing save
				
				return(MCE_FILEEXISTS);
			}
		}
	}

	char drive;
	DWORD res;

	if (card!=-1)
	{
		res=XMountMU(GetPort(card),GetSlot(card),&drive);
	}
	else
	{
		// Hard drive
		res=ERROR_SUCCESS;
		drive='U';
	}

	BOOL ok=TRUE;
	
	if (res!=ERROR_SUCCESS)
	{
		// Generic failure
		return(MCE_FAILURE);
	}
	else
	{
		char path[16];
		sprintf(path,"%c:\\",drive);

		char gamedir[256];

		switch(XCreateSaveGame(path,name,allowed_overwrite? OPEN_ALWAYS: CREATE_NEW,0,gamedir,256))
		{
		case ERROR_DISK_FULL:
			if (card!=-1)
				XUnmountMU(GetPort(card), GetSlot(card));
			return MCE_CARDFULL;

		case ERROR_SUCCESS:
			// that's fine.
			break;

		case ERROR_ALREADY_EXISTS:
			// Oh no can't overwrite!
			ASSERT(!allowed_overwrite); // otherwise why would we get this error?
			if (card!=-1)
				XUnmountMU(GetPort(card), GetSlot(card));
			return MCE_FILEEXISTS;

		case ERROR_CANNOT_MAKE:
			// couldn't even begin to make the game. Why's that?
			{
				if (card!=-1)
					XUnmountMU(GetPort(card), GetSlot(card));

				// we have to check for too many files, strangely.
				int num_saves;
				GetNumSaves(card, &num_saves);
				if (num_saves == MAX_MEMORY_CARD_SAVES)
				{
					// Well, that's a simple and quick check.
					return MCE_CARDFULL;
				}
				
				// But of course, the cert tools don't do it that way, they don't create
				// valid savegames. So we'd better just enumerate
				if (count_num_files(drive) >= MAX_MEMORY_CARD_SAVES)
				{
					// That's why.
					return MCE_TOO_MANY_SAVES;
				}

				// not a clue.
				return MCE_FAILURE;
			}
			break;

		default:
			if (card!=-1)
				XUnmountMU(GetPort(card), GetSlot(card));
			return MCE_FAILURE;
		}

		// Write savegame image to new save
		
		if (mSaveImage)
			WriteSaveImage(gamedir,mSaveImage);
		else
		{
			TRACE("Warning : Save game icon was not available!\n");
		}

		// and we'd better update the free space display on the card.
		GetMountedCardFreeAndTotalBytes(card, drive);

		if (card!=-1)
		{
			if (XUnmountMU(GetPort(card),GetSlot(card))!=ERROR_SUCCESS)
			{
				TRACE("Warning : XUnmountMU failed!\n");
			}
		}

		// Update this cardindex. Actually we don't really care about the cardindex info,
		// we just wanted a new save game number. So let's just invent one.

		// Actually, this is a lie. First we want to check whether our savegame is in fact a duplicate.
		for (int ii=0;ii<mCardSaves[cardindex];ii++)
		{
			if (wcsicmp(mCardSaveName[cardindex][ii],name)==0)
			{
				// Aha! The save already exists.
				if (save)
					*save=ii; // Return number of existing save

				goto save_found;
			}
		}

		// If we've got here, the save is new.
		if (mCardSaves[cardindex] == MAX_MEMORY_CARD_SAVES - 1)
		{
			// We've got no space.
			// But we'll have stored memory.
			// Let's just replace one in our list.
			*save = MAX_MEMORY_CARD_SAVES - 1;

		}
		else
		{
			// Stick it on the end.
			*save = mCardSaves[cardindex]++;

			// we might not have the memory allocated.
			if (!mCardSaveName[cardindex][*save])
				mCardSaveName[cardindex][*save]=new (MEMTYPE_MEMORYCARD) WCHAR[MAX_NAME_LEN];
		}

		// copy it in.
		wcsncpy(mCardSaveName     [cardindex][*save], name   , MAX_NAME_LEN - 1);

save_found:;
	}

	if (ok)
		return(MCE_SUCCESS);
	else
		return(MCE_FAILURE);
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::DeleteSave(int card,int save,WCHAR *name)
{
	int cardindex=card;
	if (card==-1)
		cardindex=TOTAL_XBOX_CARDS-1;
	
	char drive;
	DWORD res;

	if (mCardSaves[cardindex]<=save)
	{
		return(MCE_NOFILE);
	}
	
	if (card!=-1)
	{
		res=XMountMU(GetPort(card),GetSlot(card),&drive);
	}
	else
	{
		// Hard drive
		res=ERROR_SUCCESS;
		drive='U';
	}

	BOOL ok=TRUE;

	if (res!=ERROR_SUCCESS)
	{
		// Generic failure
		return(MCE_FAILURE);
	}
	else
	{
		char path[16];
		sprintf(path,"%c:\\",drive);

		// Let's verify the shape of the data structures.
		ASSERT(!wcscmp(mCardSaveName[cardindex][save], name));

		if (XDeleteSaveGame(path,mCardSaveName[cardindex][save])!=ERROR_SUCCESS)
		{
			ok=FALSE;
		}

		if (card!=-1)
		{
			if (XUnmountMU(GetPort(card),GetSlot(card))!=ERROR_SUCCESS)
			{
				TRACE("Warning : XUnmountMU failed!\n");
			}
		}

		// Update this card

		UpdateCardInfo(card);
	}

	if (ok)
		return(MCE_SUCCESS);
	else
		return(MCE_FAILURE);
}

//---------------------------------------------------------------------------

static FILETIME last_creation_time;

int CXBoxMemoryCard::WriteSave(int card,int save,WCHAR *name,void *data,int datasize)
{
	int cardindex=card;
	if (card==-1)
		cardindex=TOTAL_XBOX_CARDS-1;
	
	char drive;
	DWORD res;

	if (mCardSaves[cardindex]<=save)
	{
		return(MCE_NOFILE);
	}
	
	if (card!=-1)
	{
		res=XMountMU(GetPort(card),GetSlot(card),&drive);
	}
	else
	{
		// Hard drive
		res=ERROR_SUCCESS;
		drive='U';
	}
	
	if (res!=ERROR_SUCCESS)
	{
		// Generic failure
		return(MCE_FAILURE);
	}
	else
	{
		res=MCE_SUCCESS;

		// we don't save the path any more, because it takes lots of memory.
	
		// We just get the directory of the save from the save game name
		char root_path[4] = "x:\\";
		char path[MAX_PATH];
		root_path[0] = drive;
		if (XCreateSaveGame(root_path, name, OPEN_ALWAYS, 0, path, MAX_PATH) != ERROR_SUCCESS)
		{
			// Something strange has happened.
			ASSERT(0);
			res = MCE_FAILURE;
		}
		else
		{
			strcat(path,SAVE_FILE_NAME);

			// Sometimes we may overwrite an existing save, so use OPEN_ALWAYS
			// We assume that code deciding whether this is a good idea or not has happened elsewhere
			HANDLE h=CreateFile(path,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

			if (h==INVALID_HANDLE_VALUE)
			{
				// Opening file failed

				if (GetLastError()==ERROR_ALREADY_EXISTS) // Check for "file exists" error
					res=MCE_FILEEXISTS;
				else
					res=MCE_FAILURE;
			}
			else
			{
				CXBoxSaveHeader header;
				
				header.CreateHeader((BYTE *) data,datasize);

				DWORD written;

				if (!WriteFile(h,&header,sizeof(header),&written,NULL))
				{
					// Write failed. But why?
					if (GetLastError() == ERROR_DISK_FULL) res = MCE_CARDFULL;
					else								   res = MCE_FAILURE ;
				}
				else
				{
					if (written!=sizeof(header))
					{
						// Out of space
						
						res=MCE_CARDFULL;
					}
					else
					{
						if (!WriteFile(h,data,datasize,&written,NULL))
						{
							// Write failed
							
							res=MCE_FAILURE;
						}
						else
						{
							if (written!=datasize)
							{
								// Out of space
								
								res=MCE_CARDFULL;
							}
						}
					}
				}
						
				GetFileTime(h, &last_creation_time, NULL, NULL);
				
				CloseHandle(h);
			}
		}

		// and we'd better update the free space display on the card.
		GetMountedCardFreeAndTotalBytes(card, drive);

		if (card!=-1)
		{
			if (XUnmountMU(GetPort(card),GetSlot(card))!=ERROR_SUCCESS)
			{
				TRACE("Warning : XUnmountMU failed!\n");
			}
		}
		
		// Don't update this card
	}

	return(res);
}

//---------------------------------------------------------------------------

int CXBoxMemoryCard::ReadSave(int card,int save,WCHAR *name,void *data,int datasize,int *bytesread)
{
	int cardindex=card;
	if (card==-1)
		cardindex=TOTAL_XBOX_CARDS-1;
	
	char drive;
	DWORD res;
	
	if (mCardSaves[cardindex]<=save)
	{
		return(MCE_NOFILE);
	}
	
	if (card!=-1)
	{
		res=XMountMU(GetPort(card),GetSlot(card),&drive);
	}
	else
	{
		// Hard drive
		res=ERROR_SUCCESS;
		drive='U';
	}	
	
	if (res!=ERROR_SUCCESS)
	{
		// Generic failure
		return(MCE_FAILURE);
	}
	else
	{
		res=MCE_SUCCESS;
		
		// this had better be right.
		ASSERT(!wcscmp(mCardSaveName[cardindex][save], name));

		// Get the directory of the save.
		char root_path[4] = "x:\\";
		char path[MAX_PATH];
		root_path[0] = drive;
		DWORD creation_success = XCreateSaveGame(root_path, name, OPEN_EXISTING, 0, path, MAX_PATH);
		if (creation_success != ERROR_SUCCESS)
		{
			// oh dear.
			if (creation_success == ERROR_DEVICE_NOT_CONNECTED)
			{
				// he's whipped the card out with ninja-like speed.
				res = MCE_NOCARD;
			}
			else
			{
				// something or other.
				res = MCE_FAILURE;
			}
		}
		else
		{
			// This is the file within the savegame directory
			strcat(path,SAVE_FILE_NAME);
			
			HANDLE h=CreateFile(path,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			
			if (h==INVALID_HANDLE_VALUE)
			{
				// Opening file failed

				DWORD err=GetLastError();
				
				if ((err==ERROR_FILE_NOT_FOUND) || (err==ERROR_PATH_NOT_FOUND))
					res=MCE_CORRUPT; // No file here == corrupt save
				else
					res=MCE_FAILURE;
			}
			else
			{
				DWORD read;

				CXBoxSaveHeader header;

				if (!ReadFile(h,&header,sizeof(CXBoxSaveHeader),&read,NULL))
				{
					// Read failed
					
					res=MCE_FAILURE;
				}
				else
				{
					// We must read the rest of the data to check the header on it.
					if (!ReadFile(h,data,datasize,&read,NULL))
					{
						// Read failed
						
						res=MCE_FAILURE;
					}
					else
					{
						*bytesread=read;

						// now check the sig.
						if (!header.CheckHeader((BYTE *) data,datasize))
						{
							// Header corrupt
							res=MCE_CORRUPT;
						}

						// And also should check for data being appended.
						// Simply try loading one extra byte.
						char byte;
						DWORD bytes_read;
						if (ReadFile(h, &byte, 1, &bytes_read, NULL) && bytes_read != 0)
						{
							// Oh dear there's something been added.
							res = MCE_CORRUPT;
						}
					}
				}		

				GetFileTime(h, &last_creation_time, NULL, NULL);

				CloseHandle(h);
			}
		}

		if (card!=-1)
		{
			if (XUnmountMU(GetPort(card),GetSlot(card))!=ERROR_SUCCESS)
			{
				TRACE("Warning : XUnmountMU failed!\n");
			}
		}
		
		// Update this card. But why? So don't, it just slows things down.
	}
	
	return(res);
}

//---------------------------------------------------------------------------

int	CXBoxMemoryCard::GetSaveSize(int datasize,int *savesize)
{
	*savesize=datasize+sizeof(CXBoxSaveHeader);

	*savesize+=(2*XBOX_BLOCK_SIZE); // Size of icon file (erk?)

	// But we really want to always have 5 blocks.
	int bigsize = XBOX_BLOCK_SIZE * 4 + XBOX_BLOCK_SIZE / 2;

	if (*savesize <= bigsize)
	{
		*savesize = bigsize;
	}
	else
	{
		// oh dear, the savegame has got bigger than the text we put up.
		ASSERT(0);
		
		// but that's not going to kill anyone, so play on.
	}
	
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

#define	RADIX		10

void CXBoxMemoryCard::FormatInt( int value, char *number )
{
	// Format normally; no separators here
    _itoa( value, number, RADIX );
	
    // Determine number of separators to insert, if any
    UINT	len = lstrlenA( number );
    UINT	separators = ( len - 1 ) / 3;
    if( separators <= 0 )
        return;
	
    // !MT! Need to do this based on language
	char separator = ',';
	
    // Insert separators in place via backward walk
    char *dest = number + len + separators;
    char *src = number + len;
    for( UINT ii = 0; ii < len; ii++, --dest, --src )
    {
        *dest = *src;
        if( ii && ( ii % 3 == 0 ) )
        {
            --dest;
            *dest = separator;
        }
    }
}

//---------------------------------------------------------------------------

int	CXBoxMemoryCard::MakeHumanReadableSize(int bytes,WCHAR *buffer)
{
	char buf[256];

	int blocks=bytes/XBOX_BLOCK_SIZE;

	if (blocks<50000)
	{
		FormatInt(blocks,buf);
	}
	else
	{
		if (TEXT_DB.GetLanguage() == LANG_SPANISH)
			strcpy(buf,"+50000");
		else
			strcpy(buf,"50000+");
	}

//	strcat(buf," blocks");
	
	wcscpy(buffer,ToWCHAR(buf));
	
	return(MCE_SUCCESS);
}

//---------------------------------------------------------------------------

// this returns whether anything cards have been inserted but doesn't do anything else.
// Magically however state is hidden behind the scenes so that the rest of the memory card
// interface to the game is unaffected.
BOOL CXBoxMemoryCard::PeekAtInsertions()
{
	if (PLAYABLE_DEMO) return false;

	DWORD insertedcards,removedcards;
		
	XGetDeviceChanges(XDEVICE_TYPE_MEMORY_UNIT,&insertedcards,&removedcards);

	// let's remember these insertions/removals for the sake of the Update method.
	mRememberedInsertedCards |= insertedcards;
	mRememberedRemovedCards  |=  removedcards;

	return mRememberedInsertedCards;
}

//---------------------------------------------------------------------------

// now returns true if anything happened
bool CXBoxMemoryCard::Update()
{
	if (PLAYABLE_DEMO) return false;

	bool retval = false;

	DWORD insertedcards,removedcards;
		
	XGetDeviceChanges(XDEVICE_TYPE_MEMORY_UNIT,&insertedcards,&removedcards);

	insertedcards |= mRememberedInsertedCards;
	 removedcards |= mRememberedRemovedCards ;

	if (insertedcards | removedcards) // sorry the fact either | or || (pun intended) would work is delightful.
	{
		if ((insertedcards & XDEVICE_PORT0_TOP_MASK)!=0)
			mCardNeedsUpdating[0]=PLUGGED;
		if ((insertedcards & XDEVICE_PORT0_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[1]=PLUGGED;
		if ((insertedcards & XDEVICE_PORT1_TOP_MASK)!=0)
			mCardNeedsUpdating[2]=PLUGGED;
		if ((insertedcards & XDEVICE_PORT1_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[3]=PLUGGED;
		if ((insertedcards & XDEVICE_PORT2_TOP_MASK)!=0)
			mCardNeedsUpdating[4]=PLUGGED;
		if ((insertedcards & XDEVICE_PORT2_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[5]=PLUGGED;
		if ((insertedcards & XDEVICE_PORT3_TOP_MASK)!=0)
			mCardNeedsUpdating[6]=PLUGGED;
		if ((insertedcards & XDEVICE_PORT3_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[7]=PLUGGED;

		if ((removedcards & XDEVICE_PORT0_TOP_MASK)!=0)
			mCardNeedsUpdating[0]=UNPLUGGED;
		if ((removedcards & XDEVICE_PORT0_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[1]=UNPLUGGED;
		if ((removedcards & XDEVICE_PORT1_TOP_MASK)!=0)
			mCardNeedsUpdating[2]=UNPLUGGED;
		if ((removedcards & XDEVICE_PORT1_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[3]=UNPLUGGED;
		if ((removedcards & XDEVICE_PORT2_TOP_MASK)!=0)
			mCardNeedsUpdating[4]=UNPLUGGED;
		if ((removedcards & XDEVICE_PORT2_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[5]=UNPLUGGED;
		if ((removedcards & XDEVICE_PORT3_TOP_MASK)!=0)
			mCardNeedsUpdating[6]=UNPLUGGED;
		if ((removedcards & XDEVICE_PORT3_BOTTOM_MASK)!=0)
			mCardNeedsUpdating[7]=UNPLUGGED;		

		// Update device bitmap
		// We do removals first just in case something's been removed then re-inserted since we last looked.
		mCurrentDeviceBitmap&=~removedcards;
		mCurrentDeviceBitmap|=insertedcards;

		// And get rid of the remembered stuff.
		mRememberedInsertedCards = 0;
		mRememberedRemovedCards  = 0;
	}

	for (int i=0;i<TOTAL_XBOX_CARDS;i++)
		if (mCardNeedsUpdating[i])
		{
			// update
			retval = true;
			UpdateCardInfo(i);
		}

	return retval;
}

//---------------------------------------------------------------------------

char CXBoxMemoryCard::MountGetDriveLetterAndUnmount(int card)
{
	char drive = 'U';

	if (card == -1 || card == TOTAL_XBOX_CARDS - 1) return drive;
	
	DWORD res=XMountMU(GetPort(card),GetSlot(card),&drive);
	
	if (res==ERROR_SUCCESS)
	{
		XUnmountMU(GetPort(card), GetSlot(card));
	}

	return drive;
}

//---------------------------------------------------------------------------

bool CXBoxMemoryCard::CardBeenUnpluggedSinceLastTimeIAsked(int card)
{
	char drive = 'U';

	DWORD res;
	
	if (card == -1 || card == TOTAL_XBOX_CARDS - 1)
	{
		// it's the hard drive.
		res = ERROR_SUCCESS;
	}
	else
	{
		res = XMountMU(GetPort(card),GetSlot(card),&drive);
	}
	
	if (res==ERROR_SUCCESS)
	{
		// Get the directory of the save.
		char root_path[4] = "x:\\";
		char path[MAX_PATH];
		root_path[0] = drive;
		DWORD creation_success = XCreateSaveGame(root_path, FRONTEND.mFEPSaveGame.mSaveGameName, OPEN_EXISTING, 0, path, MAX_PATH);
		if (creation_success == ERROR_SUCCESS)
		{
			// try to find the file too.
			strcat(path, SAVE_FILE_NAME);

			HANDLE dat = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			if (dat != INVALID_HANDLE_VALUE)
			{
				FILETIME t;

				BOOL worked = GetFileTime(dat, &t, NULL, NULL);

				CloseHandle(dat);

				if (worked)
				{
					// We've found the time!
					if (t.dwHighDateTime == last_creation_time.dwHighDateTime && t.dwLowDateTime == last_creation_time.dwLowDateTime)
					{
						// Wow, phew, finally something has happened.
						// And the games match.
						if (card != -1 && card != TOTAL_XBOX_CARDS - 1)
							XUnmountMU(GetPort(card),GetSlot(card));

						return false;
					}
					else
					{
						// save it for next time.
						last_creation_time = t;
					}
				}
			}
		}

		if (card == -1 || card == TOTAL_XBOX_CARDS - 1)
		{
			// it's the hard drive.
		}
		else
		{
			XUnmountMU(GetPort(card),GetSlot(card));
		}
	}
	else
	{
		// oh dear, couldn't mount this. Story of my life. But anyway, in this context this means that it must
		// be a change of MU - the old one must have mounted because we saved to it.
		return true;
	}
	
	return true;
}

//---------------------------------------------------------------------------

void CXBoxMemoryCard::SerialiseForReboot(FILE *file, bool saving)
{
	if (saving)
	{
		fwrite(&last_creation_time, sizeof(last_creation_time), 1, file);
	}
	else
	{
		fread (&last_creation_time, sizeof(last_creation_time), 1, file);
	}
}

#endif