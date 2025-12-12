// XBox memory card access

#ifndef XBOXMEMORYCARD_H
#define XBOXMEMORYCARD_H

#include "MemoryCard.h"

#define XBOX_BLOCK_SIZE 16384
#define TOTAL_XBOX_CARDS 9

// not quite sure where to put this #define, so it's going here.
// It's the number that we pass to the xbox dashboard so that it passes it back to us when we go to dash to delete savegames.
#define XBOX_DASH_ID (0xB16B00B5)

class CXBoxMemoryCard : public CMemoryCard
{
public:

	void		InitialiseOnce();
	void		Initialise();
	void		Shutdown();
	
	BOOL		IsHDDAvailable();
	
	int			GetNumCards(int *num);
	int			GetCardInfo(int card,BOOL *present,BOOL *formatted,int *bytesfree,int *bytestotal);
	int			GetCardName(int card,WCHAR *buffer);
	int			GetCardName2(int card,WCHAR *buffer);
	int			GetCardOrPreviousCardName2(int card,WCHAR *buffer); // if the card is removed, the name will persist until next insertion.
	int			GetCardType(int card,int *cardtype);
	
	int			Format(int card);
	int			Unformat(int card);
	
	int			GetNumSaves(int card,int *num);
	int			GetSaveName(int card,int save,WCHAR *name);
	
	int			CreateSave(int card,WCHAR *name,int *save, BOOL allowed_overwrite);
	int			DeleteSave(int card,int save,WCHAR *name);
	
	int			WriteSave(int card,int save,WCHAR *name,void *data,int datasize);
	int			ReadSave(int card,int save,WCHAR *name,void *data,int datasize,int *bytesread);

	int			GetSaveSize(int datasize,int *savesize);
	int			MakeHumanReadableSize(int bytes,WCHAR *buffer);

	// this returns even non-save-game files, because I bet the testers will use the cert tools to create 4096.
	int			CountNumHardDriveFiles();

	// if in doubt, returns 'U'
	char		MountGetDriveLetterAndUnmount(int card);

	void		AccumulateResources(class CResourceAccumulator *ra, DWORD flags=0) {};

	bool		CardBeenUnpluggedSinceLastTimeIAsked(int card);

	// this will tell you if anything's been inserted but not affect anything else.
	BOOL		PeekAtInsertions();

	void		SerialiseForReboot(FILE *file, bool saving);

	bool		Update();
private:
	void		FormatInt( int value, char *number );
	DWORD		GetPort(int card);
	DWORD		GetSlot(int card);
	void		UpdateCardInfo(int card);
	void		UpdateCardSaveInfo(int card,char drive);
	int			WriteSaveImage(char *directory,class CDXTexture *image);
	void		GetMountedCardFreeAndTotalBytes(int card, char drive);


	BOOL		mCardPresent[TOTAL_XBOX_CARDS];
	BOOL		mCardFormatted[TOTAL_XBOX_CARDS];
	int			mCardBytesTotal[TOTAL_XBOX_CARDS];
	int			mCardBytesFree[TOTAL_XBOX_CARDS];
	WCHAR		mCardName[TOTAL_XBOX_CARDS][MAX_MUNAME]; // these names persist after removal - useful for "card removed" messages

	// There's different reasons why a card would need updating.
	enum UpdateReason
	{
		NO = 0, // doesn't need updating
		UNPLUGGED = 1, // it's been unplugged/plugged
		PLUGGED = 2,
		MODULE_INIT = 3 // the memorycard module has been initted
	};
	UpdateReason mCardNeedsUpdating[TOTAL_XBOX_CARDS];

	int			mCardSaves[TOTAL_XBOX_CARDS];
	WCHAR		*mCardSaveName[TOTAL_XBOX_CARDS][MAX_MEMORY_CARD_SAVES];
	DWORD		mLastAskedHash[TOTAL_XBOX_CARDS];

	// for the sake of some extra functionality we don't always use the insertions/removals bits right away,
	// we must remember them.
	DWORD		mRememberedInsertedCards, mRememberedRemovedCards;

	DWORD		mCurrentDeviceBitmap;
	class CDXTexture	*mSaveImage;
};

#endif