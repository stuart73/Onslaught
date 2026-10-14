// MeshPart.h: interface for the CMeshPart class.
//
//////////////////////////////////////////////////////////////////////

#ifndef MESHPART_H
#define MESHPART_H

#if TARGET==XBOX
#define USECOMPRESSEDMATRICES
#endif

#include "meshtypes.h"

//JCL - needs to be here - sorry!
#ifdef _DIRECTX
#define CMESH		CMesh
#elif TARGET == PS2
#define CMESH		CMesh
#endif

class CMESH;
class CResourceAccumulator;
class CMotionController;
class CPolyBucketTriangle;
class CCollisionTriangle;

#ifdef SIMPLE_MESHES

#include	"JCLTexture.h"
#define		MESH_TEXTURE			CJCLTexture
#define		MESH_TEXTURE_SURFACE	CJCLTexture
#define		CTEXTURE				CJCLTexture

#elif (TARGET == PC) || (TARGET == XBOX)

#include	"Texture.h"
#define		MESH_TEXTURE			CVBufTexture
#define		MESH_TEXTURE_SURFACE	CTEXTURE

#elif TARGET == PS2

#include	"Texture.h"
#define		MESH_TEXTURE			CTEXTURE
#define		MESH_TEXTURE_SURFACE	CTEXTURE

#endif

//--------------------------------------------------------------------------------
// A tiny 8-byte matrix.
#define SMATHS_33_ANDER 127
#define SMATHS_33_MULTIPLIER (2.016f)

class FSmallerMatrix
{
private:
	ULONG A, B;

	static float __forceinline ToFloat(ULONG amt)
	{
		ASSERT(amt <= SMATHS_33_ANDER);

		return float(SLONG(amt) - (SMATHS_33_ANDER >> 1)) * (SMATHS_33_MULTIPLIER / SMATHS_33_ANDER);
	}

	// faster more common case. Well actually there's no point optimising (I think)
	static float __forceinline ToFloat(ULONG amt, ULONG rshift)
	{
		return ToFloat((amt >> rshift) & SMATHS_33_ANDER);
	}

	static ULONG FromFloat(float amt)
	{
		float r1 = amt * (SMATHS_33_ANDER / SMATHS_33_MULTIPLIER) + float(SMATHS_33_ANDER >> 1);

		if (fabs(amt) < 500.f)
		{
			ASSERT(r1 >= 0.f);
		}
		else
		{
			return SMATHS_33_ANDER >> 1;
		}

		ULONG r2 = ULONG(r1);

		ASSERT(r2 <= SMATHS_33_ANDER);

		return r2;
	}

public:
	const FSmallerMatrix &operator =(const FSmallerMatrix &rhs)
	{
		A = rhs.A;
		B = rhs.B;

		return *this;
	}

	FSmallerMatrix() {}

	const FSmallerMatrix &operator =(const FMatrix &rhs)
	{
		A = (FromFloat(rhs.Row[0].X) << 25) +
			(FromFloat(rhs.Row[0].Y) << 18) +
			(FromFloat(rhs.Row[0].Z) << 11) +
			(FromFloat(rhs.Row[1].X) <<  4);

		B = (FromFloat(rhs.Row[1].Z) << 22) +
			(FromFloat(rhs.Row[2].X) << 15) +
			(FromFloat(rhs.Row[2].Y) <<  8) +
			(FromFloat(rhs.Row[2].Z)      );

		// overlap
		ULONG y = FromFloat(rhs.Row[1].Y);
		A += y >>  3;
		B += y << 29;

		return *this;
	}

	FSmallerMatrix(const class FMatrix &rhs) {*this = rhs;}

	operator FMatrix() const
	{
		FMatrix retval;

		retval.Row[0].X = ToFloat(A, 25);
		retval.Row[0].Y = ToFloat(A, 18);
		retval.Row[0].Z = ToFloat(A, 11);
		retval.Row[1].X = ToFloat(A,  4);

		// overlap
		retval.Row[1].Y = ToFloat(((A << 3) | (B >> 29)) & SMATHS_33_ANDER);
		
		retval.Row[1].Z = ToFloat(B, 22);
		retval.Row[2].X = ToFloat(B, 15);
		retval.Row[2].Y = ToFloat(B,  8);
		retval.Row[2].Z = ToFloat(B,  0);

		// No point for W
		retval.Row[0].W = 0.f;
		retval.Row[1].W = 0.f;
		retval.Row[2].W = 0.f;
		retval.Row[3].W = 0.f;

		return retval;
	}

	FMatrix operator -(const FSmallerMatrix &rhs) const
	{
		FMatrix ml = *this;
		FMatrix mr(rhs );

		return ml - mr;
	}
	FMatrix operator +(const FSmallerMatrix &rhs) const
	{
		FMatrix ml = *this;
		FMatrix mr(rhs );

		return ml + mr;
	}
};
// a smaller matrix
//--------------------------------------------------------------------------------

class	CMeshTexture
{
public:
					CMeshTexture();
					~CMeshTexture();
	class MESH_TEXTURE_SURFACE	*mTexture;
	class MESH_TEXTURE *mVBuffer;
	int				mFrames;
	float			*mTextureBlend;
	float			*mTextureU;
	float			*mTextureV;
	float			*mTextureUScale;
	float			*mTextureVScale;
	float			mMaxBlend;

	void			ClearToDefaults();
	void			SetupVBuffer();
	void			SetNumFrames(int f);

	void			AccumulateResources( CResourceAccumulator * accumulator,DWORD flags=0 );
};

// (JCL) - simplified replacement for CCollisionVolume
class	CBoundingBox	// axis aligned
{
public:
	FVector mOrigin;
	FVector	mAxes;
	BOOL	mValid;
	float	mRadius;
	void	CalculateRadius()
	{
		mRadius=((mAxes.X*mAxes.X)+(mAxes.Y*mAxes.Y)+(mAxes.Z*mAxes.Z));
		if (mRadius>0)
			mRadius=sqrtf(mRadius);
	}
	void				Serialize(class CChunker *c,class CResourceAccumulator *ra);
	static CBoundingBox	*Deserialize(class CChunkReader *c, CBoundingBox *already_allocated_memory = NULL);
};

class IRenderableThing;

//===========================================================================
class	CMeshDVertex
{
public:
	FVector			mNormal;
	FVector			mSmoothNormal;
	SINT			mPVert;
	float			mU, mV;
	DWORD			mDiffuse;
	SINT			mTexture[TEXTURE_TYPES];
	SINT			mIndex[TEXTURE_TYPES]; // temp for CTEXTURE::AddVertex;
};

class	CMeshTriangle
{
public:
	CMeshDVertex	*mA, *mB, *mC;
};

#ifdef _DIRECTX
#define CMESHPART	CDXMeshPart
#elif TARGET == PS2
#define CMESHPART	CPS2MeshPart
#endif

class	CCutsceneSequence
{
public:
	class CMESHPART		*mCamera;
	int					mStart;
	int					mEnd;
};

#include "MemBuffer.h"

#ifndef SIMPLE_MESHES
#include "MeshVB.h"
#endif

class	CMESHPART;
class	CPolyBucket;

class CMeshPart  
{
public:
	friend class		CMeshConstructor;
	
	// It is important that all the FMatrix and FVectors are placed at the start of the class,
	// as otherwise they will cause alignment problems!

	FMatrix			mOrientation;	
	FMatrix			mBaseOrientation;

	FVector			mOffsetPos;
	FVector			mBasePosition;	

	CMeshPart();
	~CMeshPart();

	BOOL			InitMemory(SINT ndv, SINT npv, SINT nt, SINT nvf, SINT naf);
	void			Read201(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins);
	void			Read202(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins);
	void			Read203(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins);
	void			Read204(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins, int numtexturelayers,UINT version);
	void			GenerateSimpleBoundingBox();
	void			HierarchyAdjust(FVector pos, FMatrix ori, CMESHPART *parent);	

	void			EvaluateNormals(bool replaceexisting);
	SINT			FindVertex(FVector pos);
	void			BuildPolyBucket();

	void			GetTransforms(SINT vf1,SINT vf2,float vffrac,FMatrix *ori,FVector *pos);
	void			GetWorldTransforms(SINT vf1,SINT vf2,float vffrac,IRenderableThing *t,FMatrix *ori,FVector *pos,bool inmodelspace,bool usecontrollers,bool forcenocache=false);

	BOOL			GetFirstPoly(const FVector &pos, float radius, CCollisionTriangle* out);
	BOOL			GetNextPoly(CCollisionTriangle* out);

	BOOL			GetFirstPolyLine(const FVector &start_pos, const FVector &end_pos, CCollisionTriangle* out);
	BOOL			GetNextPolyLine(CCollisionTriangle* out, BOOL found_hit = FALSE);
	
	SINT			ResolveFrameNumber(float frame,int animind,float &offset,CMotionController *mc=NULL);	
	FVector			GetRandomVertexPos() ;
	
	void			WipeVertexData();
	
	void			MergeVertices(CMeshPart *from);
	
	CMeshPart		*Clone();
	
	void			ReducePolys();

	void			BuildCache();
	void			BuildChain();

	CMeshTriangle	*mTriangles;           // dead
	FVector			**mPVertices;          // dead

	SINT			mPartNumber;	// which part I am  //0
	SINT			mPartType;		// Part type    // 01

	SINT			mNumChildren;          // 0
	CMESHPART		**mChildren;           // 0
	CMESHPART		*mParent;             // 0
		
	CMESHPART		*GetMeshDataPart();
	
	CMESHPART		*mNextMeshPartInChain;  // 0

	int				mNumChainedParts;    // 0
	BOOL			mIsChained;         // 0

	SINT			mNumDVertices;      // 0
	SINT			mNumPVertices;      // 0
	SINT			mNumTriangles;     // 0
	SINT			mNumAFrames;       // 1
	SINT			mNumVFrames;       // 1
	SINT			mNumHFrames;      // 1
	SINT			mNumBones;        //00

	// float *mVFrames              // dead

	UBYTE			*mVHFrameMap;     // dead

	FVector			*mHFramePositions;  // dead
	float			*mHFrameFOVs;   // 0000
	CMESHPART		**mBones;       // 0000
	float			**mBoneWeights; // 0000
	int				**mBoneSlots;   // 00000

	char			mName[32];

	CBoundingBox	*mBoundingBox;
	CPolyBucket		*mPolyBucket;

	FVector			*mCachedPosition;

	FMatrix			*mCachedOrientation;
	FMatrix			*mHFrameOrientations;

	FSmallerMatrix	*mCachedOrientationSmaller;
	FSmallerMatrix	*mHFrameOrientationsSmaller;

#ifdef USECOMPRESSEDMATRICES
	const FMatrix GetHFrameOrientation(int which) const
	{
		// see the terror of the copy constructor. Either this will get optimised out, or it will happen so infrequently as not
		// to matter anyway.
		if (mHFrameOrientations) return mHFrameOrientations[which];
		
		ASSERT(mHFrameOrientationsSmaller);

		return mHFrameOrientationsSmaller[which];
	}

	const FMatrix GetCachedOrientation(int which) const
	{
		if (mCachedOrientation) return mCachedOrientation[which];

		ASSERT(mCachedOrientationSmaller);

		return mCachedOrientationSmaller[which];
	}
#else
	// no compression rubbish, just return the obvious.
	const FMatrix &GetHFrameOrientation(int which) const
	{
		return mHFrameOrientations[which];
	}
	const FMatrix &GetCachedOrientation(int which) const
	{
		return mCachedOrientation[which];
	}
#endif


	int				mNumCachedFrames;
	BOOL			mPositionStatic;
	BOOL			mOrientationStatic;

	CMESHPART		*mReferenceTo;

	CMESH			*mBaseMesh;

	float			mRadius;
	float			mCueFactor;

	CMeshDVertex	*mDVertices;

	static ULONG	mLastCachedWorldTransCount ;
	static ULONG	mLastCachedModelTransCount ;
	static SINT		mLastCachedWorldVF;
	static SINT		mLastCachedModelVF;
	static FMatrix	mLastCachedWorldOri;
	static FMatrix	mLastCachedModelOri;
	static FVector	mLastCachedWorldPos;
	static FVector	mLastCachedModelPos;
	static IRenderableThing* mCachedRenderThing ;
	static CMeshPart *mCachedMeshPart;

	void				Serialize(class CChunker *c,class CResourceAccumulator *ra);
	static CMESHPART	*Deserialize(class CChunkReader *c,CMESHPART *n,class CMesh *mesh);

#ifndef SIMPLE_MESHES

	CMESHVB			*mMeshVB;	// cached Vertex Buffer
#endif
};

#ifdef _DIRECTX
#include "DXMeshPart.h"
#elif TARGET == PS2
#include "PS2MeshPart.h"
#endif

#endif // MESHPART_H
