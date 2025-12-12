#include	"Common.h"

#if TARGET == PC

#include	"Game.h"
#include	"PCSoundManager.h"
#include	"Vfw.h"
#include	"DX.h"
#include	"Profile.h"
#include	"text.h"

CPCSoundManager	SOUND;

//---------------------------------------------------------------------------

#define	SM_IN_STEREO			TRUE

//---------------------------------------------------------------------------

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }


//---------------------------------------------------------------------------
CPCSample::~CPCSample()
{
	SOUND.KillAllInstancesOfSample(this);
	SAFE_DELETE(mWavData);
	SAFE_RELEASE(mDSBuffer);
}

//---------------------------------------------------------------------------
// Initialisation code
//---------------------------------------------------------------------------

bool CPCSoundManager::DeviceInit()
{
	mWaveSoundRead=new (MEMTYPE_SOUND) CWaveSoundRead();

	mPBuffer = NULL;
	
	HWND handleToWindow=LT.GetHWnd();

	mDS = NULL;
	
	for (int i = 0;i < MAX_SOUND_BUFFERS; i++)
	{
		mDSBuffer[i] = NULL;
	}
	
    // Create IDirectSound using the primary sound device
    if( FAILED(DirectSoundCreate8(NULL, &mDS, NULL ) ) ) 
	{        
		TRACE("Could not create DSound device!\n");
		return FALSE;
	}

    // Set co-operative level
    if( FAILED(mDS->SetCooperativeLevel( handleToWindow, DSSCL_PRIORITY ) ) ) 
	{        
		return FALSE;
	}
    
    // Create the primary buffer

    DSBUFFERDESC        dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);

	// SRG ### trying removing ctrl_3d ;
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;
       
    if (FAILED(mDS->CreateSoundBuffer(&dsbd, &mPBuffer, NULL)))
	{		
		return FALSE;
	}

    // Set primary buffer format to 44kHz and 16-bit output.

    WAVEFORMATEX wfx;
    ZeroMemory(&wfx,sizeof(WAVEFORMATEX)); 
    wfx.wFormatTag=WAVE_FORMAT_PCM; 
    if(SM_IN_STEREO)
	{
		wfx.nChannels       = 2;
	}
	else
	{
		wfx.nChannels       = 1;
	}
    wfx.nSamplesPerSec  = 44100; //22050; 
    wfx.wBitsPerSample  = 16; 
    wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (FAILED(mPBuffer->SetFormat(&wfx)))
		return FALSE;

	// Get our listener pointer

	if (FAILED(mPBuffer->QueryInterface(IID_IDirectSound3DListener,(void **) &mListener)))
	{
		ASSERT(0);
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
// This is called when the system shuts down
//---------------------------------------------------------------------------

void CPCSoundManager::DeviceShutdown()
{
	SAFE_RELEASE(mPBuffer);
	
	for(int i = 0; i < MAX_SOUND_BUFFERS; i++)
	{
		SAFE_RELEASE(mDS3DBuffer[i]);
		SAFE_RELEASE(mDSBuffer[i]);
	}
	
	SAFE_RELEASE(mDS);	

	SAFE_DELETE(mWaveSoundRead);
}

//---------------------------------------------------------------------------
// This function should shut down any playing sounds
//---------------------------------------------------------------------------

void CPCSoundManager::DeviceReset()
{
	for(int i=0;i<MAX_SOUND_BUFFERS;i++)
	{
		if (mDSBuffer[i])
		{
			mDSBuffer[i]->Stop();
		}
	}
}

//---------------------------------------------------------------------------
// Loads a new sample - this function should only setup platform-specific
// data, not any of the members of the platform-independant CSample
//---------------------------------------------------------------------------

CSample	*CPCSoundManager::LoadNewSample(char *fname,BOOL music)
{
	if (mInitialised== FALSE) return NULL;

	char filename[256];

	// What if this sample was actually meant to be language dependent?
	// We'll only handle the case where it starts "sounds\"
	extern bool language_dependent;
	if (language_dependent && strnicmp(fname, "sounds\\", 7) == 0)
	{
		// cool!
		sprintf(filename,"data\\sounds\\%s\\%s.wav", TEXT_DB.GetLanguageLoadedName(), fname + 7);
	}
	else
	{
		// simple turn the sample into a filename
		sprintf(filename,"data\\%s.wav",fname);
	}

	CPCSample *s=NULL;

	if (!(mWaveSoundRead->Open(filename)))
	{
		mWaveSoundRead->Reset();

		s = new( MEMTYPE_SOUND_SAMPLE ) CPCSample;

		s->mWavSize = mWaveSoundRead->m_ckIn.cksize;
		s->mWavData = new( MEMTYPE_SOUND_SAMPLE ) BYTE [s->mWavSize];
		
		UINT	foo;
		if(FAILED(mWaveSoundRead->Read(s->mWavSize, s->mWavData, &foo))) 
		{
			delete [] s->mWavData;
			s->mWavData = NULL;
			s->mWavSize = 0;
		}

		mWaveSoundRead->Close();

		// Create a buffer for this sound

		void	*pbData  = NULL;
		DWORD	dwLength=s->mWavSize;
		PCMWAVEFORMAT pcmwf; 
		DSBUFFERDESC dsbdesc; 
		
		memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT)); 
		pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM; 
		pcmwf.wf.nChannels = 1;
		pcmwf.wf.nSamplesPerSec = 44100; 
		pcmwf.wf.nBlockAlign = 2; 
		pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign; 
		pcmwf.wBitsPerSample = 16; 
		
		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
		dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
		dsbdesc.dwFlags = DSBCAPS_LOCDEFER | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY; // | DSBCAPS_CTRL3D;
		dsbdesc.guid3DAlgorithm = GUID_NULL;// DS3DALG_NO_VIRTUALIZATION; //DS3DALG_HRTF_LIGHT; //DS3DALG_HRTF_FULL;
		dsbdesc.dwBufferBytes = s->mWavSize; 
		dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf; 
		
		mDS->CreateSoundBuffer(&dsbdesc, &s->mDSBuffer, NULL);
		
		s->mDSBuffer->Lock(0, 0, &pbData, &dwLength, NULL, NULL, DSBLOCK_ENTIREBUFFER);
		memcpy(pbData, s->mWavData, s->mWavSize);
		s->mDSBuffer->Unlock( (pbData), (dwLength), NULL,	0 );
	}

	return(s);
}

//---------------------------------------------------------------------------
// Play a sound
//---------------------------------------------------------------------------

void CPCSoundManager::PlaySound(CSoundEvent *event)
{
	PROFILE_FN(PlaySound);

	SINT 	vol = DSBVOLUME_MAX;//Fade(GetVolumeForThing(event->mOwner.ToRead()), event);

	CPCSample *s	= (CPCSample *) event->mSample; 
	int		channel = event->mChannel;

	int		fromsample = ((int) ((event->mStartPoint+event->mTime) * 44100.0f)) * 2;
	int		tosample;
	if (event->mEndPoint!=-1.0f)
		tosample = ((int) (event->mEndPoint * 44100.0f)) * 2;
	else
		tosample = s->mWavSize;

	if (fromsample>=tosample) // Nothing to do!
		return;
	
	if ((s->mWavData != NULL) && (s->mWavSize))
	{
		// Duplicate the buffer into this sound channel

		mDS->DuplicateSoundBuffer(s->mDSBuffer,&mDSBuffer[channel]);

		// Get the DS3D interface for this channel
		
		mDSBuffer[channel]->QueryInterface(IID_IDirectSound3DBuffer,(void**) &mDS3DBuffer[channel]);

		mDSBuffer[channel]->SetVolume(vol);

		//Now just play the buffer, it gets passed into the the primary buffer and mixed atuomatically.
			
		DWORD flags=0;

		if (event->mLooping)
			flags |= DSBPLAY_LOOPING;

		UpdateSoundPosition(event, FALSE);

		UpdateSound(event, TRUE);

		mDSBuffer[channel]->SetCurrentPosition(fromsample);
		mDSBuffer[channel]->Play(0,0,flags);
		event->mPlaying=true;	
	}
}


//---------------------------------------------------------------------------
// UnPause
//---------------------------------------------------------------------------
void CPCSoundManager::UnPauseSound(CSoundEvent *event)
{
	int		channel = event->mChannel;
	if	(mDSBuffer[channel])
	{
		DWORD flags=0;

		if (event->mLooping)
			flags |= DSBPLAY_LOOPING;
		mDSBuffer[channel]->Play(0,0,flags);
	}
}


//---------------------------------------------------------------------------
// Pause
//---------------------------------------------------------------------------
void CPCSoundManager::PauseSound(CSoundEvent *event)
{
	int		channel = event->mChannel;
	if	(mDSBuffer[channel])
	{
		mDSBuffer[event->mChannel]->Stop();
	}
}


//---------------------------------------------------------------------------
// Stop a sound
//---------------------------------------------------------------------------

void CPCSoundManager::StopSound(CSoundEvent *event,BOOL blockuntilstopped)
{
	if (mDSBuffer[event->mChannel])
	{
		mDSBuffer[event->mChannel]->Stop();
		SAFE_RELEASE(mDSBuffer[event->mChannel]);
	}
}

//---------------------------------------------------------------------------
// Update global data (called at start of update process)
//---------------------------------------------------------------------------

void CPCSoundManager::UpdateGlobals()
{
/*	if (mInitialised== FALSE) return ;
	DS3DLISTENER l;
	l.dwSize=sizeof(l);

	FVector front,top,velocity;

	front=mCameraOrientation * FVector(0,-1,0);
	top=mCameraOrientation * FVector(0,0,1);

	velocity=mCameraPosition-mOldCameraPosition;
	
//	mListener->GetAllParameters(&l);

	l.vPosition.x=mCameraPosition.X/10.0f;
	l.vPosition.y=mCameraPosition.Y/10.0f;
	l.vPosition.z=mCameraPosition.Z/10.0f;
	l.vOrientFront.x=front.X;
	l.vOrientFront.y=front.Y;
	l.vOrientFront.z=front.Z;
	l.vOrientTop.x=top.X;
	l.vOrientTop.y=top.Y;
	l.vOrientTop.z=top.Z;
	l.vVelocity.x=(velocity.X*20.0f)/10.0f;
	l.vVelocity.y=(velocity.Y*20.0f)/10.0f;
	l.vVelocity.z=(velocity.Z*20.0f)/10.0f;
	l.flRolloffFactor=1.0f;
	l.flDistanceFactor=1.0f;
	l.flDopplerFactor=1.0f;
	
	if (mListener) mListener->SetAllParameters(&l,DS3D_DEFERRED);*/
}

//---------------------------------------------------------------------------
// Update a sound's information
//---------------------------------------------------------------------------

void CPCSoundManager::UpdateSound(CSoundEvent *event, BOOL first_time)
{
	if (mDSBuffer[event->mChannel])
	{
/*		DS3DBUFFER p;

		p.dwSize=sizeof(p);

//		mDS3DBuffer[event->mChannel]->GetAllParameters(&p);
		
		p.vPosition.x=event->mPos.X/10.0f;
		p.vPosition.y=event->mPos.Y/10.0f;
		p.vPosition.z=event->mPos.Z/10.0f;
		
		p.vVelocity.x=(event->mVelocity.X*20.0f)/10.0f;
		p.vVelocity.y=(event->mVelocity.Y*20.0f)/10.0f;
		p.vVelocity.z=(event->mVelocity.Z*20.0f)/10.0f;

		p.flMaxDistance=10.0f;
		p.flMinDistance=0.1f;
		p.dwInsideConeAngle=360;
		p.dwOutsideConeAngle=360;
		p.dwMode=DS3DMODE_NORMAL;
		p.lConeOutsideVolume=DSBVOLUME_MAX;
		p.vConeOrientation.x=0;
		p.vConeOrientation.y=0;
		p.vConeOrientation.z=1;
		
		if (!first_time)
		{
			mDS3DBuffer[event->mChannel]->SetAllParameters(&p,DS3D_DEFERRED);
		}
		else
			mDS3DBuffer[event->mChannel]->SetAllParameters(&p,DS3D_IMMEDIATE);	*/	
		mDSBuffer[event->mChannel]->SetPan(event->mPan);
	
		// ## SRG  clamp to 1.0 to stop stalls  (why won't it work??)
		if (event->mPitchMultiplier > 1.0f) event->mPitchMultiplier= 1.0f ;

		mDSBuffer[event->mChannel]->SetFrequency((UINT) (event->mPitchMultiplier*44000));
			
		// Ensure we actually fall off to silence

		int vol=event->mCurrentAttenuatedVolume;
		
		if (vol<-4000)
		{
			vol=vol+((vol+4000)*2);
		}

		mDSBuffer[event->mChannel]->SetVolume(vol);

		// Stop the event if the sample has finished

		DWORD status;

		mDSBuffer[event->mChannel]->GetStatus(&status);

		if (!first_time)
		{
			if (!(status & (DSBSTATUS_LOOPING || DSBSTATUS_PLAYING )))
			{
				StopSoundEvent(event);
			} 
		}
	}
}

//---------------------------------------------------------------------------
// Called after all the UpdateSound() calls have been made for a frame
//---------------------------------------------------------------------------

void CPCSoundManager::UpdatesDone()
{
	if (mInitialised== FALSE) return ;
	mListener->CommitDeferredSettings();
}

//---------------------------------------------------------------------------
// Get the sample data at a given time (in 16 bit signed format)
//---------------------------------------------------------------------------

int CPCSoundManager::GetSampleDataAtTime(CSample *sample,float time)
{
	int	offset=((int) (time * 44100.0f));

	short *sampledata=(short *) ((CPCSample *) sample)->mWavData;

	return(sampledata[offset]);
}

//---------------------------------------------------------------------------
// Get the sample data at a given sample (in 16 bit signed format)
//---------------------------------------------------------------------------

int CPCSoundManager::GetSampleDataAtSample(CSample *sample,int time)
{
	short *sampledata=(short *) ((CPCSample *) sample)->mWavData;
	
	return(sampledata[time]);
}

//---------------------------------------------------------------------------
// Returns the length of the sample in seconds
//---------------------------------------------------------------------------

float CPCSoundManager::GetSampleLength(CSample *sample)
{
	return(((float) ((CPCSample *) sample)->mWavSize)/(44100.0f*2.0f));
}

//---------------------------------------------------------------------------
// Returns the number of samples in the sample
//---------------------------------------------------------------------------

int	CPCSoundManager::GetSampleLengthInSamples(CSample *sample)
{	
	return(((CPCSample *) sample)->mWavSize/2);
}

//---------------------------------------------------------------------------
// Returns the number of a free channel, or -1 if all are occupied
//---------------------------------------------------------------------------

int CPCSoundManager::FindFreeChannel()
{
	for (int i=0;i<MAX_SOUND_BUFFERS;i++)
	{
		CSoundEvent *se=SOUND.GetFirstSoundEvent();
		
		bool ok=true;
		
		while (se)
		{
			if (se->mChannel==i)
				ok=false;
			se=se->mNextEvent;
		}		

		if (ok & (!mDSBuffer[i]))
			return(i);
	}
	
	return(-1);
}

#endif