// Music code

#ifndef MUSIC_H
#define MUSIC_H

#include "console.h"

enum EMusicPlayType
{
	MPT_SINGLE,
	MPT_LINEAR,
	MPT_RANDOM,
	MPT_SELECTION,
};

class CSong
{
public:
	CSong()
	{
		mIndex=-1;
		mNext=NULL;
	}

	char					mFilename[MAX_PATH];
	CSong					*mNext;
	int						mIndex;
};

class CMusicMenu : public CConsoleMenu
{
	virtual void	GetName(char *name)				{ strcpy(name,"Music"); };
	virtual int		GetNumEntries();
	virtual bool	GetShowSubmenus()				{ return(true); };
	virtual void	GetEntry(int num,char *text);
	virtual void	OnClick(int num);
};

enum	EMusicSelection
{
	MUS_FRONTEND,
	MUS_CREDITS,
	MUS_TUTORIAL,
	MUS_STEALTH,
	MUS_INGAME
};

class CMusic
{
public:
							CMusic()
							{
								mInitialised=FALSE;
							}
	void					Initialise();
	void					Shutdown();
	void					Play(char *filename);
	void					Stop();
	void					AddDirectoryToPlaylist(char *dir);
	void					AddToPlayList(char *name);
	void					WipePlayList();
	void					PlayFromList(CSong *s,BOOL fade=TRUE);
	void					PlaySelection(EMusicSelection sel,BOOL fade=FALSE);
	void					UpdateStatus();	
	void					SetVolume(float vol);
	int						GetTrackForFrontEnd(); // returns which track number the front end will use.
	int						GetTrackForCredits(); // and same for credits.
	float					GetVolume() { return ((float)mSetVolume)/127.0f; };
	int						GetCurrentVolume() { return mVolume; };
	CSong					*GetRandomSong();
	CSong					*GetSong(int which);
	
	EMusicPlayType			mPlayType;	
	bool					mPlaying;
	CSong					*mFirstSong;
	CSong					*mCurrentSong;	

	void					DeviceWarnOfStop() {} // this is the game telling you that soon it will want to stop

protected:
	CMusicMenu				mMenu;
	int						mTargetVolume;
	int						mSetVolume;
	
	CSong					*mQueuedSong;
	
	void					AddDirectoryExtsToPlaylist(char *dir,char *ext);
	void					FadeVolumes();
	
	// Platform-specific interface
	
	virtual void			DeviceInitialise()=0;
	virtual void			DeviceShutdown()=0;
	virtual void			DevicePlay(char *filename)=0;
	virtual void			DeviceStop()=0;
	virtual	BOOL			DeviceGetTrackFinished()=0;
	virtual void			DeviceSetVolume(int vol)=0;
	virtual void			DeviceAddDirectoryExtsToPlaylist(char *dir,char *ext)=0;
	virtual void			DeviceChangeTrack(char *filename); // like Stop() Play() but hopefully cleverer.

	// this one's non-pure because it's OK to have nothing in it.
	virtual void			DeviceUpdateStatus() {}
protected:
	int						mVolume;
	bool					mInitialised;
	EMusicSelection			mSelection;
};

#if	TARGET == PC

extern class CPCMusic	MUSIC;
#include	"PCMusic.h"

#elif TARGET == PS2

extern class CPS2Music	MUSIC;
#include	"PS2Music.h"

#elif TARGET == XBOX

extern class CXBOXMusic MUSIC;
#include    "XBOXMusic.h"

#endif

#endif