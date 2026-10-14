#ifndef MESH_H
#define MESH_H

#include "Animation.h"
#include "MeshPart.h"
#include "MemBuffer.h"

#ifndef SIMPLE_MESHES
#include "ResourceAccumulator.h"
#include "Texture.h"
#endif

#ifndef SIMPLE_MESHES
#include	"MeshVB.h"
#else
#define BONE_SLOTS 3		//aargh hack.
#endif

class CMotionController;

class	CAnimMode
{
public:
	char			mModeName[16];
	EAnimMode		mMode;
	int				mStart;
	int				mLast;
	int				mFrameScaler; // last-start for normal anims, last-start+1 for loops
	float			mRate;
};

class	CEmitter
{
public:
	FVector			mPos;
	FMatrix			mOrientation;
	CMESHPART		*mMeshPart;
	SINT			mTrackingVertex;
	BOOL			mSubpartEmitter; // Does this emitter have it's own subpart (2.04 format)?
	char			mName[256];
	int				mNumber;
};

// Maximum number of chained (ie damaged/etc) mesh parts
#define MAX_CHAIN_ENTRIES 20

// Emitter effect flags

#define EFFECT_FLAG_USE_ORIENTATION 1
#define EFFECT_FLAG_SET_ORIENTATION 2

/**** In Meshpart.h!!
#if TARGET == PC
#define CMESH		CPCMesh
#elif TARGET == PS2
#define CMESH		CPS2Mesh
#endif
*/

#define MESH_NAME_LEN 300

class CMesh
{
public:	
	friend class		CMeshConstructor;

	CMesh();

	CMeshTexture		*mTextures;
	int					mNumTextures;

	static	BOOL		InitAll();
	static	void		ReleaseAll();
	static	void		LoadLevelResources( SINT level );
	static	void		FreeLevelResources();
	static	void		ShutdownAll();

	static	BOOL		InitDefaultTexture(); // JCL - had to make this separate for ParticleEd

	BOOL				InitFromFile(const char *name, SINT num_skins);
	BOOL				InitFromMem(CMEMBUFFER &mem, SINT num_skins);

	void				AddToList();
	void				RemoveFromList();

	void				CheckForAnimations(char* animdesc, CAnimMode* amode);
	void				BuildAllPolyBuckets();
	char*				GetName();

	int					GetNumEmitters();
	void				GetEmitterNameByID(int n,char *name, int *num);
	CMESHPART			*GetEmitterSubpartByID(int n);
	CMESHPART			*GetMovieCameraSubpart(int frame);
	EAnimMode			GetAnimModeByName(char *name);
	CAnimMode			*GetAnimModePtr(EAnimMode mode);
	CMESHPART			*GetEmitterSubpart(char *name,int num);
	int					GetEmitterIDByName(char *name, int num);
	CMESHPART*			GetMeshSubpart(char *name);

	static	CMESH		*GetMesh(const char *name, SINT num_skins = 1);
	float				GetFrameIncrement(EAnimMode a, SINT* realindex);

	float				Radius() {return mRadius;}
	float				CueFactor() {return mCueFactor;}

	void				ReduceFrames();
	void				ReducePolys();
	void				OptimiseTextures();
	void				OptimiseParts();

	SINT				GetNumParts() { return mNumParts; }
	CMESHPART			*GetPart( SINT aPartNo ) { return mParts[ aPartNo ]; }

	class CMESH			*mNextInChain;

	SINT				mNumSequences;
	CCutsceneSequence	*mSequences;

	SINT				mNumAnimModes;
	CAnimMode*			mAnimModes;

	SINT				mNumEmitters;
	CEmitter			*mEmitters;

	char				mName[MESH_NAME_LEN];
	CBoundingBox		*mBoundingBox;

	int					mResourceID;

#ifdef RESBUILDER
	void				AccumulateResources(CResourceAccumulator * accumulator,DWORD flags=0);
#endif
	void				AddRef() { mRefCount++; }
	void				Release() { mRefCount--; ASSERT(mRefCount>=0); };

	void				ForceDelete() { delete this; };

	int					GetRefCount() { return(mRefCount); };
	
#ifndef SIMPLE_MESHES
	void				Serialize(class CChunker *c,CResourceAccumulator *ra,DWORD flags);
	static CMESH		*Deserialize(class CChunkReader *c);
#endif

	static CMESH		*GetMeshByResourceID(int resid);

	FVector				TransformLocalToWorld(FVector pos, IRenderableThing *t, CMESHPART *mp,BOOL include_offset = TRUE);
	FVector				TransformWorldToLocal(FVector pos, IRenderableThing *t, CMESHPART *mp,BOOL include_offset = TRUE);
#ifdef LINK_EDITOR
#ifdef RESBUILDER
	
	void				AccumulateResources(CTextureAccumulator &ta, CMeshAccumulator &ma);
#endif
#endif	
	void				BuildEmitterCache();

	FVector				GetCentre() {return ZERO_FVECTOR;};
	
	// JCL - write out mesh if there's no animation etc...
	BOOL				SimpleWrite(char *fname);

	FVector				GetRandomVertexPos(CMeshPart*& return_part);

protected:
						~CMesh();
	
	void				CalculateBases();

	static	CMESH		*mFirstMesh;
	CMESH				*mNextMesh;
	
	SINT				mNumParts;
	CMESHPART			**mParts;

	float				mRadius;
	float				mCueFactor;

	static	CMeshTexture	*mDefaultTexture;

	SINT				mLastFrameCollisionGenerated;
	static BOOL			mInLevel;

	UINT				mRefCount;
};

#endif