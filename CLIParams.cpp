#include	"Common.h"

#include	"Platform.h"
//#include	"VertexShader.h"
#include	"CLIParams.h"
#include	"console.h"
#include	"text.h"

#include	<stdio.h>
#include	<string.h>

CCLIParams CLIPARAMS;


#define MAX_PARMS 30
#define PARM_LENGTH 256

CCLIParams::CCLIParams()
{	
#ifdef DEV_VERSION
	mModelViewer=FALSE;
	mCutsceneEditor=FALSE;
#endif
	
	mMusic=TRUE;
	mBuildResources = FALSE;
	mBuildPCResources=FALSE;
	mBuildPS2Resources=FALSE;
	mBuildXBOXResources=FALSE;
	mDeveloperMode=FALSE;
	mNoStaticShadows=FALSE;
	mLargeRAM=FALSE;
	mBuildModelInfo=FALSE;
	mNoBaseResources=FALSE;
	mBuildGoodies=FALSE;
	mShowDebugTrace=FALSE;
	
	mGeforce3=FALSE;
	mForcedCard=FALSE;
	mVShaders=FALSE;
	mForcedShaders=FALSE;
	mDevKit=FALSE;
	mHiDetailMode=FALSE;
	mDecimateMeshes=FALSE;
	mSound=TRUE;
	mEmulateDVD=FALSE;

	mTextureRAMLimit = 0x7fffffff; // default to huge limit, but not huge enough to confuse signed arithmetic
	
	mForceWindowed=FALSE;
	mResBuilderMode=FALSE;
	mStressTest=0;
	
	mNoMeshPartReduction=FALSE;

	mArtistTest = FALSE;
	mLevelNo = -1;
	mRecordDemo=false;
	mPlaybackDemo=false;
	mConfigurationNo=0;
	mPal=TRUE;
	mGoStraightToDeviceSelectScreen = -1;
	mNiceCompression = true;
	mLanguage=LANG_ENGLISH;
	mNoCodeOffCD=FALSE;
	mKillHUD = FALSE;

#if TARGET==XBOX
	mClearUtility=false;
	mCyclesBeforeReboot	= 1;

	// default to standard data location.
	strcpy(mBasePath, "d:");
#endif

	mAttractMode=FALSE;
	mInactiveTimeout=-1;
	mGameplayTimeout=-1;

	// Default to using a pure device
	mPureDevice=TRUE;
	mSkipFMV = FALSE;
}

void	CCLIParams::GetParams(char *text)
{
	if (text[0] == 0)
		return;
	
	// Be very careful not to use any game functions here, as
	// nothing is initialised at this point (not even the memory
	// manager)

	char	parms[MAX_PARMS][PARM_LENGTH];

	SINT	num_parms = 0;
	SINT	x = 0;
	parms[num_parms][x] = 0;

	// break into parms	
	while ((parms[num_parms][x] = *text) != NULL)
	{
		if (*text == ' ')
		{
			if (x != 0) // skip multiple whitespace
			{
				if (num_parms < MAX_PARMS)
				{
					parms[num_parms][x] = 0;
					num_parms ++;
					x = 0;
				}
				else 
					break;
			}
		}
		else
		{
			if (x < PARM_LENGTH)
				x ++;
			else
			{
				if (num_parms < MAX_PARMS)
				{
					parms[num_parms][x] = 0;
					num_parms ++;
					x = 0;
				}
				else 
					break;
			}
		}
		text ++;
	}
	num_parms ++;
	
	// scan parms
	SINT current_parm = 0;

	while (current_parm < num_parms)
	{
		if (!(stricmp(parms[current_parm], "-artists")))
			mArtistTest = TRUE;
		
		if (!(stricmp(parms[current_parm], "-nostaticshadows")))
			mNoStaticShadows = TRUE;

		if (!(stricmp(parms[current_parm], "-hidetail")))
			mHiDetailMode = TRUE;		

		if (!(stricmp(parms[current_parm], "-decimatemeshes")))			
			mDecimateMeshes=TRUE;
		
		if (!(stricmp(parms[current_parm], "-nomeshpartreduction")))			
			mNoMeshPartReduction=TRUE;

		if (!(stricmp(parms[current_parm], "-textureramlimit")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mTextureRAMLimit);
		}

		if (!(stricmp(parms[current_parm], "-forcewindowed")))
			mForceWindowed=TRUE;

		if (!(stricmp(parms[current_parm], "-emulatedvd")))
			mEmulateDVD=TRUE;

		if (!(stricmp(parms[current_parm], "-showdebugtrace")))
			mShowDebugTrace=TRUE;

		if (!(stricmp(parms[current_parm], "-buildgoodies")))
			mBuildGoodies = true;

		if (!(stricmp(parms[current_parm], "-resbuildermode")))
			mResBuilderMode=TRUE;
		
		if (!(stricmp(parms[current_parm], "-nocodeoffcd")))
			mNoCodeOffCD=TRUE;

		if (!(stricmp(parms[current_parm], "-forcewindowed")))
			mForceWindowed=TRUE;

		if (!(stricmp(parms[current_parm], "-resbuildermode")))
			mResBuilderMode=TRUE;

		if (!(stricmp(parms[current_parm], "-geforce2")))
		{
			mGeforce3=FALSE;
			mForcedCard=TRUE;
		}

		if (!(stricmp(parms[current_parm], "-geforce3")))
		{
			mGeforce3=TRUE;
			mForcedCard=TRUE;
		}

		if (!(stricmp(parms[current_parm], "-vshaders")))
		{
			mVShaders=TRUE;
			mForcedShaders=TRUE;
		}
		
		if (!(stricmp(parms[current_parm], "-novshaders")))
		{
			mVShaders=FALSE;
			mForcedShaders=FALSE;
		}
		
		if (!(stricmp(parms[current_parm], "-nomusic")))
			mMusic=FALSE;

		if (!(stricmp(parms[current_parm], "-nosound")))
			mSound=FALSE;
		
		if (!(stricmp(parms[current_parm], "-pure")))
			mPureDevice=TRUE;

		if (!(stricmp(parms[current_parm], "-impure")))
			mPureDevice=FALSE;

		if (!(stricmp(parms[current_parm], "-devkit")))
			mDevKit=TRUE;

		if (!(stricmp(parms[current_parm], "-quickcompression")))
			mNiceCompression=false;

		if (!(stricmp(parms[current_parm], "-largeram")))
			mLargeRAM=TRUE;

		if (!(stricmp(parms[current_parm], "-pal")))
			mPal=TRUE;

		if (!(stricmp(parms[current_parm], "-ntsc")))
			mPal=FALSE;

		if (!(stricmp(parms[current_parm], "-buildresources")))
		{
			TRACE("Building resource files for...\n");
			current_parm++;
			while ((current_parm<num_parms) && (parms[current_parm][0]!='-'))
			{
				if (!(stricmp(parms[current_parm], "PC")))
				{
					TRACE("PC\n");
					mBuildPCResources=TRUE;
				}
				if (!(stricmp(parms[current_parm], "PS2")))
				{
					TRACE("PS2\n");
					mBuildPS2Resources=TRUE;
				}
				if (!(stricmp(parms[current_parm], "XBOX")))
				{
					TRACE("XBOX\n");
					mBuildXBOXResources=TRUE;
				}
				current_parm++;
			}
			current_parm--;
			mBuildResources=TRUE;
			PLATFORM.SetMemorySize(256*1024*1024); // 256Mb heap for building resources by default
		}
		
		if (!(stricmp(parms[current_parm], "-nobaseresources")))
			mNoBaseResources=TRUE;

		if (!(stricmp(parms[current_parm], "-devmode")))
			mDeveloperMode=TRUE;

		if (!(stricmp(parms[current_parm], "-skipfmv")))
			mSkipFMV=TRUE;

		if (!(stricmp(parms[current_parm], "-attractmode")))
			mAttractMode=TRUE;

		if (!(stricmp(parms[current_parm], "-traceconsole")))
			CONSOLE.SetTrace(TRUE);
		
#ifdef DEV_VERSION
		if (!(stricmp(parms[current_parm], "-modelviewer")))
		{
			mModelViewer = TRUE;
			PLATFORM.SetMemorySize(64*1024*1024); // 64Mb heap for modelviewer by default
		}

		if (!(stricmp(parms[current_parm], "-buildmodelinfo")))
		{
			mBuildModelInfo = TRUE;
		}
		
		if (!(stricmp(parms[current_parm], "-cutsceneeditor")))
			mCutsceneEditor = TRUE;		

		if (!(stricmp(parms[current_parm], "-killhud")))
			mKillHUD = TRUE;		
#endif

		if (!(stricmp(parms[current_parm], "-level")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mLevelNo);
		}

#if TARGET == XBOX
		if (!(stricmp(parms[current_parm], "-clearutility")))
		{
			mClearUtility = true;
		}

		if (!(stricmp(parms[current_parm], "-reboot")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mCyclesBeforeReboot);
		}
#endif
		
		if (!(stricmp(parms[current_parm], "-stresstest")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mStressTest);
		}
		
		if (!(stricmp(parms[current_parm], "-norumble")))
		{
			TRACE("Rumble pack support DISABLED\n");
			PLATFORM.SetRumbleEnabled( FALSE );
		}		

		if (!(stricmp(parms[current_parm], "-record")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%s", mDemoFilename);
			mRecordDemo=true;		
		}

		if (!(stricmp(parms[current_parm], "-play")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%s", mDemoFilename);
			mPlaybackDemo=true;
		}

		if (!(stricmp(parms[current_parm], "-configuration")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mConfigurationNo);
		}

		if (!(stricmp(parms[current_parm], "-mem")))
		{
			current_parm ++;
			int temp;
			sscanf(parms[current_parm], "%d", &temp);

			if (temp>0)
			{
				PLATFORM.SetMemorySize(temp*1024*1024);
			}
		}

/*		else if (!(stricmp(parms[current_parm], "-controller")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mController);
		}
		else if (!(stricmp(parms[current_parm], "-joymode")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mJoyMode);
		}
		else if (!(stricmp(parms[current_parm], "-character")))
		{
			current_parm ++;
			sscanf(parms[current_parm], "%d", &mStartCharacter);
		}*/

		// else if ...

		current_parm++;
	};

#ifdef DEV_VERSION
	SASSERT(!(mModelViewer && mCutsceneEditor),"ROTFL! Modelviewer *and* cutscene editor? At the same time...?");
#endif
}


void	CCLIParams::GetParams(int num_parms, char**parms)
{
	// This is *really*, *really* stupid, but the only easy way to do it :-(

	char cli[4096];

	strcpy(cli,"");

	for (int i=0;i<num_parms;i++)
	{
		strcat(cli," ");
		strcat(cli,parms[i]);
	}

	GetParams(cli);
}
