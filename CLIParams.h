#ifndef CLIPARAMS_H
#define CLIPARAMS_H

class	CCLIParams
{
public:
	CCLIParams();
	~CCLIParams() {};

	void	GetParams(char *text);
	void	GetParams(int num_params, char**parms);

	BOOL	mArtistTest;
	BOOL	mNoStaticShadows;
	SINT	mConfigurationNo;
	SINT	mLevelNo;
	BOOL	mMusic;
	BOOL	mDeveloperMode;
	BOOL	mBuildResources;
	BOOL	mBuildPCResources;
	BOOL	mBuildPS2Resources;
	BOOL	mBuildXBOXResources;
	BOOL	mNoBaseResources;
	BOOL	mBuildModelInfo;
	BOOL	mGeforce3;
	BOOL	mVShaders;
	BOOL	mForcedCard;
	BOOL	mForceWindowed;
	BOOL	mForcedShaders;
	BOOL	mPureDevice;
	BOOL	mEmulateDVD;
	BOOL	mDevKit;
	BOOL	mHiDetailMode;
	BOOL	mDecimateMeshes;
	DWORD	mTextureRAMLimit;
	BOOL	mLargeRAM;
	BOOL	mPal;
	BOOL	mNoCodeOffCD;
	BOOL	mResBuilderMode;
	bool	mRecordDemo;
	bool	mPlaybackDemo;
	bool	mNiceCompression;
	bool	mBuildGoodies;
	BOOL	mSound;
	BOOL	mNoMeshPartReduction;
	char	mDemoFilename[256];
	int		mGoStraightToDeviceSelectScreen;
	BOOL	mSkipFMV;
	BOOL	mKillHUD;
	int		mStressTest;
	int		mLanguage;
	BOOL	mShowDebugTrace;

#if TARGET == XBOX
	char	mBasePath[64]; // usually "d:", doesn't need to be more than 64 because that's the size of the structure it's loaded from.
	bool	mClearUtility;
	int		mCyclesBeforeReboot; // at the n'th attract cycle we actually reboot the xbox.
#endif

#ifdef DEV_VERSION	
	bool	mModelViewer;
	bool	mCutsceneEditor;
#endif

	// These are for the playable demo only, but I've left them in for simplicity
	BOOL	mAttractMode;
	int		mInactiveTimeout; // in milliseconds
	int		mGameplayTimeout; // in milliseconds
};

extern	CCLIParams CLIPARAMS;


#endif