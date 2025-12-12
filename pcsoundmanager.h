#ifndef PCSOUNDMANAGER_H
#define PCSOUNDMANAGER_H

// Don't include this file directly - use "SoundManager.h"

#include <windows.h>
#include "Thing.h"
#include <commdlg.h>
#include "mmsystem.h"
#include "WavRead.h"
#include <Dsound.h>
#include "resource.h" 
#include "SampleList.h"
#include "math.h"
#include "Camera.h"
#include "soundmanager.h"

// The number of sound buffers we want available

#define MAX_SOUND_BUFFERS	32

//---------------------------------------------------------------------------

class	CPCSample : public CSample
{
public:
	CPCSample()
	{
		mWavData = NULL;
	}

	virtual ~CPCSample();

	BYTE	*mWavData;
	SINT	mWavSize;

	LPDIRECTSOUNDBUFFER		mDSBuffer;
};

//---------------------------------------------------------------------------

class CPCSoundManager : public CSoundManager
{
public:
					CPCSoundManager()
					{
						mWaveSoundRead=NULL;
					}

			bool	DeviceInit();
			void	DeviceShutdown();

			void	DeviceReset();
			
			void	UnloadSample(CSample *s) {};
	
			CSample *LoadNewSample(char *filename,BOOL music);	
			CSample *LoadSampleFromBuffer(CMEMBUFFER *mb,BOOL music) { SASSERT(0,"LoadSampleFromBuffer() is not implemented"); return NULL;};

			void	PlaySound(CSoundEvent *event);	
			void	StopSound(CSoundEvent *event,BOOL blockuntilstopped=FALSE);
			void	PauseSound(CSoundEvent *event);	
			void	UnPauseSound(CSoundEvent *event);	
			void	UpdateSound(CSoundEvent *event, BOOL first_update = FALSE);
			void	UpdateGlobals();
			void	UpdatesDone();
			void	DevicePauseAll() {};
			void	DeviceUnpauseAll() {};
	
			float	GetSampleLength(CSample *sample);
			int		GetSampleDataAtTime(CSample *samp,float time);
			int		GetSampleLengthInSamples(CSample *sample);
			int		GetSampleDataAtSample(CSample *samp,int time);

			int		GetAvailableChannels() { return MAX_SOUND_BUFFERS; };
			int		FindFreeChannel();

protected:
	LPDIRECTSOUND8			mDS;
	LPDIRECTSOUNDBUFFER		mDSBuffer[MAX_SOUND_BUFFERS];
	LPDIRECTSOUND3DBUFFER	mDS3DBuffer[MAX_SOUND_BUFFERS];
	LPDIRECTSOUNDBUFFER		mPBuffer;
	LPDIRECTSOUND3DLISTENER mListener;

	CWaveSoundRead			*mWaveSoundRead;
};

#endif

