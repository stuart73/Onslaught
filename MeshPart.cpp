// MeshPart.cpp: implementation of the CMeshPart class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
#include "stdafx.h"
#include "meshpart.h"

#include "MeshPart.h"
#include "PolyBucket.h"
#include "Profile.h"

#ifndef SIMPLE_MESHES
#include "texture.h"
#include "MotionController.h"
#include "RenderThing.h"
#include "game.h"
#include "debuglog.h"
#include "cliparams.h"
#endif

#include <stdio.h>
#include <string.h>
#include "chunker.h"

//#if TARGET == PC
#ifdef _DIRECTX
#ifndef SIMPLE_MESHES
#include "VBufTexture.h"
#endif
#endif

#include <stdlib.h>


ULONG	CMeshPart::mLastCachedWorldTransCount;
ULONG	CMeshPart::mLastCachedModelTransCount;
SINT	CMeshPart::mLastCachedWorldVF;
SINT	CMeshPart::mLastCachedModelVF;
FMatrix	CMeshPart::mLastCachedWorldOri;
FMatrix	CMeshPart::mLastCachedModelOri;
FVector	CMeshPart::mLastCachedWorldPos;
FVector	CMeshPart::mLastCachedModelPos;
IRenderableThing* CMeshPart::mCachedRenderThing=NULL;
CMeshPart *CMeshPart::mCachedMeshPart=NULL;

//******************************************************************************************
// Mesh texture constructor & destructor
//******************************************************************************************

CMeshTexture::CMeshTexture()
{
	mFrames=0;
}

CMeshTexture::~CMeshTexture()
{
	if (mFrames!=0)
	{
		// It's actually only the first of the mTexture*'s that is allocated.
		SAFE_DELETE_ARRAY(mTextureBlend);

		// but we still NULL all the others.
		mTextureU=NULL;
		mTextureV=NULL;
		mTextureUScale=NULL;
		mTextureVScale=NULL;
		mFrames=0;
	}

#ifndef SIMPLE_MESHES
	SAFE_RELEASE(mTexture);
	SAFE_RELEASE(mVBuffer);	
#endif
}

//******************************************************************************************
// Change the number of frames in a mesh texture
//******************************************************************************************

void CMeshTexture::SetNumFrames(int f)
{
	if (mFrames!=0)
	{
		// It's actually only the first of the mTexture*'s that is allocated.
		SAFE_DELETE_ARRAY(mTextureBlend);

		// but we still NULL all the others.
		mTextureU=NULL;
		mTextureV=NULL;
		mTextureUScale=NULL;
		mTextureVScale=NULL;
	}
	mFrames=f;
	
	if (f==0)
	{
		mTextureBlend=NULL;
		mTextureU=NULL;
		mTextureV=NULL;
		mTextureUScale=NULL;
		mTextureVScale=NULL;
	}
	else
	{
		// See how we allocate one.
		mTextureBlend=new( MEMTYPE_MESHTEXTURE ) float[mFrames * 5];

		// And derive the others.
		mTextureU     =mTextureBlend + mFrames * 1;
		mTextureV     =mTextureBlend + mFrames * 2;
		mTextureUScale=mTextureBlend + mFrames * 3;
		mTextureVScale=mTextureBlend + mFrames * 4;
	}
}

//******************************************************************************************
// Clear a mesh texture to default values
//******************************************************************************************

void CMeshTexture::ClearToDefaults()
{
	mTexture=NULL;
	SetNumFrames(1);
	mTextureU[0]=0.0f;
	mTextureV[0]=0.0f;
	mTextureUScale[0]=1.0f;
	mTextureVScale[0]=1.0f;
	mTextureBlend[0]=1.0f;
	mMaxBlend=1.0f;
	mVBuffer=NULL;
}

//******************************************************************************************
// Setup the vertex buffer
//******************************************************************************************

void CMeshTexture::SetupVBuffer()
{
#ifndef SIMPLE_MESHES
//#if TARGET == PC
#ifdef _DIRECTX
	mVBuffer=CVBufTexture::GetTextureByTex(mTexture);
	ASSERT(mVBuffer);
	mVBuffer->SetupV(FVF_GTEX_FLAGS,D3DUSAGE_WRITEONLY,sizeof(CFVF_GTEX),D3DPT_TRIANGLELIST,D3DPOOL_MANAGED);
	mVBuffer->SetupI(D3DFMT_INDEX16,D3DUSAGE_WRITEONLY,2,D3DPOOL_MANAGED);
#endif
#endif
}

#ifdef RESBUILDER
void CMeshTexture::AccumulateResources( CResourceAccumulator * accumulator, DWORD flags )
{
#ifndef SIMPLE_MESHES
	if( mTexture )
	{
		if ((flags & RES_INHIBITPAGING)!=0)
			accumulator->AddTexture(mTexture,flags);
		else
			accumulator->AddTexture(mTexture,flags | RES_ALLOWPAGING);
	}
#endif
}
#endif
//******************************************************************************************
//** Poly Bucket Stuff
BOOL	CMeshPart::GetFirstPoly(const FVector &pos, float radius, CCollisionTriangle* out)
{
	if (!mPolyBucket)
	{
		LOG.AddMessage("FATAL ERROR: Something required poly bucket on a mesh, which it did not have");
		return FALSE ;
	}

	const CPolyBucketTriangle* tri = mPolyBucket->GetFirstCollision(pos, radius) ;
	if (tri == NULL) return FALSE ;

	out->mP1 = mPolyBucket->GetPoint(tri->mP1) ;
	out->mP2 = mPolyBucket->GetPoint(tri->mP2) ;
	out->mP3 = mPolyBucket->GetPoint(tri->mP3) ;

	return TRUE ;
}

//******************************************************************************************
BOOL	CMeshPart::GetNextPoly(CCollisionTriangle* out)
{
	if (mPolyBucket)
	{
		const CPolyBucketTriangle* tri =mPolyBucket->GetNextCollision() ;
		if (tri == NULL) return FALSE ;

		out->mP1 = mPolyBucket->GetPoint(tri->mP1) ;
		out->mP2 = mPolyBucket->GetPoint(tri->mP2) ;
		out->mP3 = mPolyBucket->GetPoint(tri->mP3) ;

		return TRUE ;
	}

	return FALSE;
}


//******************************************************************************************
//** Poly Bucket Stuff
BOOL	CMeshPart::GetFirstPolyLine(const FVector &start_pos, const FVector &end_pos, CCollisionTriangle* out)
{
	if (!mPolyBucket)
	{
		LOG.AddMessage("FATAL ERROR: Something required poly bucket on a mesh, which it did not have");
		return FALSE ;
	}

	const CPolyBucketTriangle* tri = mPolyBucket->GetFirstCollisionLine(start_pos, end_pos) ;
	if (tri == NULL) return FALSE ;

	out->mP1 = mPolyBucket->GetPoint(tri->mP1) ;
	out->mP2 = mPolyBucket->GetPoint(tri->mP2) ;
	out->mP3 = mPolyBucket->GetPoint(tri->mP3) ;
	return TRUE;
}


//******************************************************************************************
void	CMeshPart::BuildPolyBucket()
{
	if (mPolyBucket!=NULL) return ;

	if ((mPartType==MESHPART_TYPE_MESH) || (mPartType==MESHPART_TYPE_SKELETAL))
	{
		mPolyBucket = new( MEMTYPE_POLYBUCKET ) CPolyBucket;
		if (!mPolyBucket)
				return ; 

		// Create a copy of this and perform poly-reduction on it!

		CMeshPart *temp=Clone();

#ifndef SIMPLE_MESHES
		if ((stricmp(mBaseMesh->GetName(), "boss-fenrir.msh")!=0)  && 
			(stricmp(mBaseMesh->GetName(), "tempbuilding3.msh")!=0) )
		{
			if (CLIPARAMS.mBuildResources)
				temp->ReducePolys();
		}
		else
		{
			LOG.AddMessage("Ignoring mesh '%s' for poly bucket reduction", mBaseMesh->GetName() );
		}
#endif

		mPolyBucket->Build(*temp);

		for (int j=0;j<temp->mNumChildren;j++)
			temp->mChildren[j]=NULL;
		SAFE_DELETE(temp);
	}
}


//******************************************************************************************
// pass TRUE to found_hit input if you have already hit a poly and are only intrested in
// finding the first poly the line hits (i.e. an optimisation which stops the poly bucket
// advancing sectors) (note: it will still continue to return tris in the same bucket sector)

BOOL	CMeshPart::GetNextPolyLine(CCollisionTriangle* out, BOOL found_hit)
{
	if (mPolyBucket)
	{
		const CPolyBucketTriangle* tri = mPolyBucket->GetNextCollisionLine(found_hit) ;
		if (tri == NULL) return FALSE ;
		out->mP1 = mPolyBucket->GetPoint(tri->mP1) ;
		out->mP2 = mPolyBucket->GetPoint(tri->mP2) ;
		out->mP3 = mPolyBucket->GetPoint(tri->mP3) ;

		return TRUE ;
	}
	
	return FALSE;
}

//******************************************************************************************
CMeshPart::CMeshPart()
{
	mDVertices  = NULL;
	mPVertices  = NULL;
	mTriangles  = NULL;
//	mEBR		= NULL;
	mBones		= NULL;
	mBoneSlots  = NULL;
	mBoneWeights= NULL;
	mVHFrameMap = NULL;
	mHFramePositions = NULL;

	mCachedOrientation=NULL;
	mHFrameOrientations = NULL;

#ifdef USECOMPRESSEDMATRICES
	mCachedOrientationSmaller=NULL;
	mHFrameOrientationsSmaller=NULL;
#endif

	mHFrameFOVs	= NULL;
	
	mNextMeshPartInChain = NULL;
	mNumChainedParts = 0;
	mIsChained=false;

	mNumTriangles = 0;
	mNumDVertices = 0;
	mNumPVertices = 0;
	mNumVFrames	  = 0;
	mNumAFrames	  = 0;

	mOffsetPos = ZERO_FVECTOR;
	mOrientation = ID_FMATRIX;

//	mCollisionVolume = NULL;
	mRadius	= 0.5f;

	mNumChildren = 0;
	mParent = NULL;

	mNumBones=0;
	
	mPartNumber = 0;

	mPolyBucket = NULL;
	
	mReferenceTo = NULL;
	
	mCachedPosition=NULL;
	mLastCachedWorldTransCount = 99999;
	mLastCachedModelTransCount = 99999;
	mCachedRenderThing = NULL ;
	mNumCachedFrames=0;
	mCachedPosition=NULL;

	mPositionStatic=FALSE;
	mOrientationStatic=FALSE;

	mBoundingBox=new (MEMTYPE_MESH) CBoundingBox;

#ifndef SIMPLE_MESHES
	mMeshVB=new (MEMTYPE_MESH) CMESHVB;
	mMeshVB->SetMeshPart((CMESHPART*)(this));
#endif
}

//******************************************************************************************
CMeshPart::~CMeshPart()
{
//	delete	mEBR;
	
	SAFE_DELETE_ARRAY(mCachedPosition);
	SAFE_DELETE_ARRAY(mCachedOrientation);
	
	//!JCL - remove this later when changes have propagated through the resource files.
	if (mNumChildren > 0)
		SAFE_DELETE_ARRAY(mChildren);

	if (mDVertices)
		delete [] mDVertices;
	
#ifdef USECOMPRESSEDMATRICES
	// They're all actually allocated in the block above, so forget it.

	// but we've got other things to worry about
	if (mHFrameOrientationsSmaller)
		delete[] mHFrameOrientationsSmaller;

	if (mCachedOrientationSmaller)
	{
		SAFE_DELETE_ARRAY(mCachedOrientationSmaller);
	}
#endif

#if TARGET!=XBOX
/*	if (mVFrames)
		delete [] mVFrames;*/
	
	if (mPVertices)
	{
		SINT	c0;
		for (c0 = 0; c0 < mNumAFrames; c0 ++)
		{
			if (mPVertices[c0])
				delete [] mPVertices[c0];
		}
		
		delete [] mPVertices;
	}
	
	if (mTriangles)
		delete [] mTriangles;

#endif

	if (mVHFrameMap)
		delete [] mVHFrameMap;
	if (mHFrameOrientations)
		delete [] mHFrameOrientations;

	if (mHFramePositions)
		delete [] mHFramePositions;
	if (mHFrameFOVs)
		delete [] mHFrameFOVs;
	
	if (mBones)
		delete [] mBones;
	
	if (mBoneWeights)
	{
		for (int i=0;i<mNumPVertices;i++)
			delete [] mBoneWeights[i];
		delete [] mBoneWeights;
	}
	if (mBoneSlots)
	{
		for (int i=0;i<mNumPVertices;i++)
			delete [] mBoneSlots[i];
		delete [] mBoneSlots;
	}
	
	if (mPolyBucket)
		delete mPolyBucket;

	SAFE_DELETE(mBoundingBox);

#ifndef SIMPLE_MESHES
	SAFE_DELETE(mMeshVB);
#endif
}

//******************************************************************************************
#if TARGET == XBOX

BOOL	CMeshPart::InitMemory(SINT ndv, SINT npv, SINT nt, SINT nvf, SINT naf)
{
	mNumDVertices = ndv;
	mNumPVertices = npv;
	mNumTriangles = nt;
	mNumVFrames = nvf;
	mNumAFrames = naf;
	
	char *everything = new (MEMTYPE_MESH) char[sizeof(CMeshDVertex) * mNumDVertices +
								               sizeof(float) * mNumVFrames +
								               sizeof(FVector*) * mNumAFrames +
								               sizeof(FVector) * mNumPVertices * mNumAFrames +
								               sizeof(CMeshTriangle) * mNumTriangles];

	mDVertices = (CMeshDVertex *)everything;
	everything += sizeof(CMeshDVertex) * mNumDVertices;

	if (!mDVertices)
	{
		ASSERT(0);
		return FALSE;
	}
	
	mVFrames = (float *)everything;
	everything += sizeof(float) * mNumVFrames;

	if (!mVFrames)
	{
		ASSERT(0);
		return FALSE;
	}
	
	mPVertices = (FVector **)everything;
	everything += sizeof(FVector*) * mNumAFrames;

	if (!mPVertices)
	{
		ASSERT(0);
		return FALSE;
	}
	
	SINT	c0;
	for (c0 = 0; c0 < mNumAFrames; c0 ++)
	{
		mPVertices[c0] = (FVector *)everything;
		everything += sizeof(FVector) * mNumPVertices;

		if (!mPVertices[c0])
		{
			ASSERT(0);
			return FALSE;
		}
	}
	
	mTriangles = (CMeshTriangle *)everything;
	everything += sizeof(CMeshTriangle) * mNumTriangles;

	if (!mTriangles)
	{
		ASSERT(0);
		return FALSE;
	}

	return TRUE;
}

#else

BOOL	CMeshPart::InitMemory(SINT ndv, SINT npv, SINT nt, SINT nvf, SINT naf)
{
	mNumDVertices = ndv;
	mNumPVertices = npv;
	mNumTriangles = nt;
	mNumVFrames = nvf;
	mNumAFrames = naf;
	
	mDVertices = new( MEMTYPE_MESH ) CMeshDVertex[mNumDVertices];
	if (!mDVertices)
	{
		ASSERT(0);
		return FALSE;
	}
	
/*	mVFrames = new( MEMTYPE_MESH ) float[mNumVFrames];
	if (!mVFrames)
	{
		ASSERT(0);
		return FALSE;
	}*/
	
	mPVertices = new( MEMTYPE_MESH ) FVector*[mNumAFrames];
	if (!mPVertices)
	{
		ASSERT(0);
		return FALSE;
	}
	
	SINT	c0;
	for (c0 = 0; c0 < mNumAFrames; c0 ++)
	{
		mPVertices[c0] = new( MEMTYPE_MESH ) FVector[mNumPVertices];
		if (!mPVertices[c0])
		{
			ASSERT(0);
			return FALSE;
		}
	}
	
	mTriangles = new( MEMTYPE_MESH ) CMeshTriangle[mNumTriangles];
	if (!mTriangles)
	{
		ASSERT(0);
		return FALSE;
	}

#ifndef SIMPLE_MESHES
	MEM_MANAGER.TagMem(mDVertices,sizeof(CMeshDVertex)*mNumDVertices,"Meshes/%s/Part %d/DVertices",mBaseMesh->mName,mPartNumber);
//	MEM_MANAGER.TagMem(mVFrames,sizeof(float)*mNumVFrames,"Meshes/%s/Part %d/VFrames",mBaseMesh->mName,mPartNumber);
	MEM_MANAGER.TagMem(mPVertices,sizeof(FVector)*mNumPVertices*mNumAFrames,"Meshes/%s/Part %d/PVertices",mBaseMesh->mName,mPartNumber);
	MEM_MANAGER.TagMem(mTriangles,sizeof(CMeshTriangle)*mNumTriangles,"Meshes/%s/Part %d/Triangles",mBaseMesh->mName,mPartNumber);	
#endif

	return TRUE;
}

#endif

//******************************************************************************************
void	CMeshPart::GenerateSimpleBoundingBox()
{
	if ((!mPVertices) || (mPVertices==0))
		return;

	// get extremities	
	float	minx =  999999.f;
	float	miny =  999999.f;
	float	minz =  999999.f;
	float	maxx = -999999.f;
	float	maxy = -999999.f;
	float	maxz = -999999.f;

	float	maxdist = 0;

	mBoundingBox->mValid=false;

	SINT		c0;
	FVector *pv = mPVertices[0];
	for (c0 = 0; c0< mNumPVertices; c0++)
	{
		FVector v = (*pv);
		if (v.X < minx)	minx = v.X;
		if (v.Y < miny)	miny = v.Y;
		if (v.Z < minz)	minz = v.Z;
		if (v.X > maxx)	maxx = v.X;
		if (v.Y > maxy)	maxy = v.Y;
		if (v.Z > maxz)	maxz = v.Z;

		float	d = (v + mOffsetPos).MagnitudeSq();
		if (d > maxdist)
			maxdist = d;

		mBoundingBox->mValid=true;

		pv ++;
	}

	mBoundingBox->mOrigin = (FVector(minx, miny, minz) + FVector(maxx, maxy, maxz)) / 2.f;
	mBoundingBox->mAxes   = (FVector(minx, miny, minz) - FVector(maxx, maxy, maxz)) / 2.f;
	mBoundingBox->mAxes.X = FABS(mBoundingBox->mAxes.X);
	mBoundingBox->mAxes.Y = FABS(mBoundingBox->mAxes.Y);
	mBoundingBox->mAxes.Z = FABS(mBoundingBox->mAxes.Z);

	// Pick the "middle" axis as the cue factor

	mCueFactor = mBoundingBox->mAxes.Z;
	if (((mBoundingBox->mAxes.X<=mBoundingBox->mAxes.Y) && (mBoundingBox->mAxes.Y<=mBoundingBox->mAxes.Z)) || 
		((mBoundingBox->mAxes.Z<=mBoundingBox->mAxes.Y) && (mBoundingBox->mAxes.Y<=mBoundingBox->mAxes.X)))
		mCueFactor = mBoundingBox->mAxes.Y;
	if (((mBoundingBox->mAxes.Y<=mBoundingBox->mAxes.X) && (mBoundingBox->mAxes.X<=mBoundingBox->mAxes.Z)) || 
		((mBoundingBox->mAxes.Z<=mBoundingBox->mAxes.X) && (mBoundingBox->mAxes.X<=mBoundingBox->mAxes.Y)))
		mCueFactor = mBoundingBox->mAxes.X;

	mRadius = sqrtf(maxdist);

	if (!mBoundingBox->mValid)
	{
		mBoundingBox->mOrigin.Set(0,0,0);
		mBoundingBox->mAxes.Set(0,0,0);
		mRadius=0.0f;
	}

	mBoundingBox->CalculateRadius();

/*	char text[10000];
	sprintf(text,"Bounding Box Calc for %s: Origin: %f %f %f, Axes: %f %f %f\n", mBaseMesh->mName,
		    mBoundingBox.mOrigin.X, mBoundingBox.mOrigin.Y, mBoundingBox.mOrigin.Z,
			mBoundingBox.mAxes.X, mBoundingBox.mAxes.Y, mBoundingBox.mAxes.Z);
	OutputDebugString(text);*/
}

//******************************************************************************************
void	CMeshPart::Read201(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins)
{
	// load vertices
	SINT	c0;

	for (c0 = 0; c0 < mNumDVertices; c0 ++)
	{
		mem.Read(&mDVertices[c0].mPVert, 4);
		mem.Read(&mDVertices[c0].mU, 4);
		mem.Read(&mDVertices[c0].mV, 4);

//### VERY CHEAP HACK FOR FLIPPING TEXTURES BY -V

		mDVertices[c0].mV = -mDVertices[c0].mV;

//### ENDHACK

		mem.Read(&mDVertices[c0].mDiffuse, 4);

		SINT tex;
		mem.Read(&tex, 4);
		
		if ((tex < -2) || (tex >= num_textures))
			tex = -1; // could be 0 I suppose

		for (int i=0;i<TEXTURE_TYPES;i++)
			mDVertices[c0].mTexture[i]=-1;

		if (tex == -1)
		{ 
			// Use first texture in this case
			mDVertices[c0].mTexture[0] = 0;
			ASSERT(parent->mTextures>0); // No textures on mesh! Help!
		}
		else
		{
			mDVertices[c0].mTexture[0]=tex;
		}		

		mDVertices[c0].mNormal = FVector(1.0f, 0, 0);
	}

	// set 
/*	for (c0 = 0; c0 < mNumVFrames; c0 ++)
		mVFrames[c0] = float(c0);*/

	// load PVerts
	for (c0 = 0; c0 < mNumAFrames; c0 ++)
	{
		SINT	c1;
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1].Load(mem) ;
		}

		// transform points into local space
	
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1] -= mOffsetPos;
			mPVertices[c0][c1] = mOrientation.Transpose() * mPVertices[c0][c1]; 
		}
	}

	// load Triangles
	for (c0 = 0; c0 < mNumTriangles; c0++)
	{
		SINT a, b, c;

		mem.Read(&a, 4);
		mem.Read(&b, 4);
		mem.Read(&c, 4);

		ASSERT(a < mNumDVertices);
		ASSERT(b < mNumDVertices);
		ASSERT(c < mNumDVertices);
		ASSERT(a >= 0);
		ASSERT(b >= 0);
		ASSERT(c >= 0);

		mTriangles[c0].mA = &mDVertices[a];
		mTriangles[c0].mB = &mDVertices[c];
		mTriangles[c0].mC = &mDVertices[b]; // inside out
	}

//	MakeEBR();
	EvaluateNormals(true);
}
//******************************************************************************************
void	CMeshPart::Read202(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins)
{
	// load vertices
	SINT	c0;

	for (c0 = 0; c0 < mNumDVertices; c0 ++)
	{
		mem.Read(&mDVertices[c0].mPVert, 4);
		mem.Read(&mDVertices[c0].mU, 4);
		mem.Read(&mDVertices[c0].mV, 4);
//### VERY CHEAP HACK FOR FLIPPING TEXTURES BY -V

		mDVertices[c0].mV = -mDVertices[c0].mV;

//### ENDHACK
		mem.Read(&mDVertices[c0].mDiffuse, 4);

		SINT tex;
		mem.Read(&tex, 4);

		if ((tex < -2) || (tex >= num_textures))
			tex = -1; // could be 0 I suppose

		for (int i=0;i<TEXTURE_TYPES;i++)
			mDVertices[c0].mTexture[i]=-1;

		if (tex == -1)
		{
			// Use first texture in this case
			mDVertices[c0].mTexture[0] = 0;
			ASSERT(parent->mTextures>0); // No textures on mesh! Help!
		}
		else
		{
			mDVertices[c0].mTexture[0] = tex;
		}

		mDVertices[c0].mNormal = FVector(1.0f, 0, 0);
	}

	// load vframes
	float lTemp;
	for (c0 = 0; c0 < mNumVFrames; c0 ++)
		mem.Read(&lTemp, 4);

	// load PVerts
	for (c0 = 0; c0 < mNumAFrames; c0 ++)
	{
		SINT	c1;
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1].Load(mem) ;

			//mem.Read(mPVertices[c0], 12 * mNumPVertices);
		}

		// transform points into local space
	
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1] -= mOffsetPos;
			mPVertices[c0][c1] = mOrientation.Transpose() * mPVertices[c0][c1]; 
		}
	}

//	char	foo[100];
//	sprintf(foo, "Mesh %s: AFrames: %d\n", this->mBaseMesh->mName, mNumAFrames);
//	OutputDebugString(foo);

	// load Triangles
	for (c0 = 0; c0 < mNumTriangles; c0++)
	{
		SINT a, b, c;

		mem.Read(&a, 4);
		mem.Read(&b, 4);
		mem.Read(&c, 4);

		ASSERT(a < mNumDVertices);
		ASSERT(b < mNumDVertices);
		ASSERT(c < mNumDVertices);
		ASSERT(a >= 0);
		ASSERT(b >= 0);
		ASSERT(c >= 0);

		mTriangles[c0].mA = &mDVertices[a];
		mTriangles[c0].mB = &mDVertices[c];
		mTriangles[c0].mC = &mDVertices[b]; // inside out
	}

//	MakeEBR();
	EvaluateNormals(true);
}
//******************************************************************************************
void	CMeshPart::Read203(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins)
{
	// load vertices
	SINT	c0;

	for (c0 = 0; c0 < mNumDVertices; c0 ++)
	{
		mem.Read(&mDVertices[c0].mPVert, 4);
		mem.Read(&mDVertices[c0].mU, 4);
		mem.Read(&mDVertices[c0].mV, 4);
//### VERY CHEAP HACK FOR FLIPPING TEXTURES BY -V

		mDVertices[c0].mV = -mDVertices[c0].mV;

//### ENDHACK
		mem.Read(&mDVertices[c0].mDiffuse, 4);

		SINT tex;
		mem.Read(&tex, 4);

		if ((tex < -2) || (tex >= num_textures))
			tex = -1; // could be 0 I suppose

		for (int i=0;i<TEXTURE_TYPES;i++)
			mDVertices[c0].mTexture[i]=-1;

		if (tex == -1)
		{
			// Use first texture in this case
			mDVertices[c0].mTexture[0] = 0;
			ASSERT(parent->mTextures>0); // No textures on mesh! Help!
		}
		else
		{
			mDVertices[c0].mTexture[0] = tex;
		}

		mDVertices[c0].mNormal = FVector(-10.0f, 0, 0);  // dummy value
	}

	// load vframes
	float lTemp;
	for (c0 = 0; c0 < mNumVFrames; c0 ++)
		mem.Read(&lTemp, 4);

	// load PVerts
	for (c0 = 0; c0 < mNumAFrames; c0 ++)
	{
		SINT	c1;
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1].Load(mem) ;
		//	mem.Read(mPVertices[c0], 12 * mNumPVertices);
		}

		// transform points into local space
	
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1] -= mOffsetPos;
			mPVertices[c0][c1] = mOrientation.Transpose() * mPVertices[c0][c1]; 
		}
	}

//	char	foo[100];
//	sprintf(foo, "Mesh %s: AFrames: %d\n", this->mBaseMesh->mName, mNumAFrames);
//	OutputDebugString(foo);

	// JCL - now with normals!

	CMeshDVertex	*dverts = new( MEMTYPE_MESH ) CMeshDVertex[mNumDVertices * 5];  // lots of temp verts
	ASSERT(dverts!=NULL);
	SINT			num_dverts = 0;

	// load Triangles
	for (c0 = 0; c0 < mNumTriangles; c0++)
	{
		SINT a, b, c;

		mem.Read(&a, 4);
		mem.Read(&b, 4);
		mem.Read(&c, 4);

		ASSERT(a < mNumDVertices);
		ASSERT(b < mNumDVertices);
		ASSERT(c < mNumDVertices);
		ASSERT(a >= 0);
		ASSERT(b >= 0);
		ASSERT(c >= 0);

		mTriangles[c0].mA = &mDVertices[a];
		mTriangles[c0].mB = &mDVertices[c];
		mTriangles[c0].mC = &mDVertices[b]; // inside out

		// OK - read in normal, and create more vertices if necessary....
		SINT	c1, c2;

		for (c1 = 0; c1 < 3; c1 ++)
		{
			CMeshDVertex	**dp;

			switch (c1)
			{
			case 0:	dp = &mTriangles[c0].mA; break;	//!! sorry - order swapped as above
			case 1:	dp = &mTriangles[c0].mC; break;
			case 2:	dp = &mTriangles[c0].mB; break;   
			}

			FVector normal;
			mem.Read(&normal, 12);

			// negate the normal
			normal.Z = -normal.Z;

			// does this vertex have a normal?
			if ((*dp)->mNormal.X == -10.f)
			{
				// nope - make it so!
				(*dp)->mNormal = normal;
			}
			else if ((*dp)->mNormal == normal)
			{
				// same - so nothing to do
			}
			else
			{
				// ok - need to create a new vertex?
				// first - have we created this already?
				BOOL	done = FALSE;
				for (c2 = 0; c2 < num_dverts; c2 ++)
				{
					// is this the same vert?
					if ((dverts[c2].mPVert == (*dp)->mPVert) &&
						(dverts[c2].mTexture[0] == (*dp)->mTexture[0]) &&
					    (dverts[c2].mNormal == normal))
					{
						// cool - use this
						*dp = &mDVertices[mNumDVertices + c2]; // note - invalid.  must relocate later
						done = TRUE;
						break;
					}
				}
				if (!done)
				{
					// oh dear - must create new vertex...
					dverts[num_dverts] = **dp;
					dverts[num_dverts].mNormal		= normal;
					*dp = &mDVertices[mNumDVertices + num_dverts]; // note - invalid.  must relocate later

					num_dverts ++;
				}
			}
		}
	}

	// Ok - now we need to build a new DVerts array
	if (num_dverts > 0)
	{
		CMeshDVertex	*old_dverts = mDVertices;

		mDVertices = new( MEMTYPE_MESH ) CMeshDVertex[mNumDVertices + num_dverts];

		// copy data
		memcpy(mDVertices, old_dverts, sizeof(CMeshDVertex) * mNumDVertices);
		memcpy(mDVertices + mNumDVertices, dverts, sizeof(CMeshDVertex) * num_dverts);

		// and relocate
		for (c0 = 0; c0 < mNumTriangles; c0++)
		{
			mTriangles[c0].mA = (mTriangles[c0].mA - old_dverts) + mDVertices;
			mTriangles[c0].mB = (mTriangles[c0].mB - old_dverts) + mDVertices;
			mTriangles[c0].mC = (mTriangles[c0].mC - old_dverts) + mDVertices;
		}

		// free old data
		delete [] old_dverts;		

		mNumDVertices += num_dverts;
	}

	delete [] dverts;

//	MakeEBR();
	EvaluateNormals(false);
}

//******************************************************************************************

void	CMeshPart::Read204(CMEMBUFFER &mem, CMESH *parent, CMeshTexture *textures, SINT num_textures, SINT num_skins, int numtexturelayers,UINT version)
{
	// load vertices
	SINT	c0;

	for (c0 = 0; c0 < mNumDVertices; c0 ++)
	{
		mem.Read(&mDVertices[c0].mPVert, 4);
		mem.Read(&mDVertices[c0].mU, 4);
		mem.Read(&mDVertices[c0].mV, 4);
//### VERY CHEAP HACK FOR FLIPPING TEXTURES BY -V

		mDVertices[c0].mV = -mDVertices[c0].mV;

//### ENDHACK
		DWORD diffuse;
		mem.Read(&diffuse, 4);

		// Convert to RGBA

		int r,g,b,a;

		a=(diffuse>>24) & 0xFF;
		b=(diffuse>>16) & 0xFF;
		g=(diffuse>>8) & 0xFF;
		r=(diffuse & 0xFF);

		mDVertices[c0].mDiffuse=b+					// B
								(g<<8)+				// G
								(r<<16)+			// R
								(a<<24);			// A

		// Clear all possible types
		int i;
		for (i=0;i<TEXTURE_TYPES;i++)
			mDVertices[c0].mTexture[i]=-1;

		// Load those supported by this file

		for (i=0;i<numtexturelayers;i++)
		{
			SINT tex;
			mem.Read(&tex, 4);
			float amt;
			float uoffset,voffset,uscale,vscale;

			if (((version==MKID("2.05")) || (version==MKID("2.06"))))
			{
				mem.Read(&amt,sizeof(amt));
				uoffset=0.0f;
				voffset=0.0f;
				uscale=1.0f;
				vscale=1.0f;
			}
			else
				amt=1.0f;

			if (version==MKID("2.06"))
			{
				// Load UV offset/scale
				mem.Read(&uoffset,sizeof(uoffset));
				mem.Read(&voffset,sizeof(voffset));
				mem.Read(&uscale,sizeof(uscale));
				mem.Read(&vscale,sizeof(vscale));
			}

			if ((tex < -2) || (tex >= num_textures))
				tex = -1; // could be 0 I suppose

			if (tex == -1)
			{
				if (i==0)
				{
					// No base texture - use first one

					mDVertices[c0].mTexture[i]=0;

					ASSERT(parent->mTextures>0); // Help! No textures on mesh!
				}
				else
					mDVertices[c0].mTexture[i] = -1;
			}
			else
			{

				if (version==MKID("2.07"))
				{
					amt=parent->mTextures[tex].mMaxBlend; // So that we know if to assign this or not
				}
				
				// Hack to ensure textures with a duplicate diffuse2 channel don't get loaded as such

				if (i==TEXTURE_TYPE_DIFFUSE2)
				{
					// Kill Diffuse2 textures if they are the same as the basic diffuse
					if (parent->mTextures[tex].mTexture==parent->mTextures[mDVertices[c0].mTexture[TEXTURE_TYPE_DIFFUSE]].mTexture) 
						amt=0.0f;
				}

				if (amt>0.0f)
				{
					mDVertices[c0].mTexture[i] = tex;

					if (((version==MKID("2.05")) || (version==MKID("2.06"))))
					{
						parent->mTextures[tex].mTextureBlend[0]=amt;
						parent->mTextures[tex].mTextureU[0]=uoffset;
						parent->mTextures[tex].mTextureV[0]=voffset;
						parent->mTextures[tex].mTextureUScale[0]=uscale;
						parent->mTextures[tex].mTextureVScale[0]=vscale;
						parent->mTextures[tex].mMaxBlend=amt;
					}
				}
				else
					mDVertices[c0].mTexture[i] = -1;
			}
		}

		mDVertices[c0].mNormal = FVector(-10.0f, 0, 0);  // dummy value
	}

	// load vframes
	float lTemp;
	for (c0 = 0; c0 < mNumVFrames; c0 ++)
		mem.Read(&lTemp, 4);

	// load PVerts
	for (c0 = 0; c0 < mNumAFrames; c0 ++)
	{
		SINT	c1;
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1].Load(mem) ;
		}

		// transform points into local space
/*		SINT	c1;
		for (c1 = 0; c1 < mNumPVertices; c1 ++)
		{
			mPVertices[c0][c1].Z=-mPVertices[c0][c1].Z;
		//	mPVertices[c0][c1] -= mOffsetPos;
	//		mPVertices[c0][c1] = mOrientation.Transpose() * mPVertices[c0][c1]; 
		}*/
	}

	if (mPartType==MESHPART_TYPE_SKELETAL)
	{
//		OutputDebugString("Loading skeletal mesh\n");
		// Load bone information (for skeletal meshes)

		mem.Read(&mNumBones,sizeof(mNumBones));	// Number of bones

		char buffer[256];
		sprintf(buffer,"Got %d bones\n",mNumBones);
		TRACE(buffer);

		// Allocate some memory for this stuff

		mBones = new( MEMTYPE_MESH ) CMESHPART*[mNumBones];
		ASSERT(mBones);
		mBoneWeights = new( MEMTYPE_MESH ) float*[mNumPVertices];
		ASSERT(mBoneWeights);
		mBoneSlots = new( MEMTYPE_MESH ) int*[mNumPVertices];
		ASSERT(mBoneSlots);

		// Bones
		int i;
		for (i=0;i<mNumBones;i++)
		{
			SINT boneindex;
			mem.Read(&boneindex,sizeof(boneindex));

			ASSERT((boneindex>=0) && (boneindex<parent->GetNumParts()));
			mBones[i]=parent->GetPart(boneindex);
		}

		// Bone weights

		for (i=0;i<mNumPVertices;i++)
		{
			mBoneWeights[i]=new( MEMTYPE_MESH ) float[mNumBones];
			ASSERT(mBoneWeights[i]);
			float totalweight=0.0f;
			int j;
			for (j=0;j<mNumBones;j++)
			{
				mem.Read(&mBoneWeights[i][j],sizeof(mBoneWeights[i][j]));
				totalweight+=mBoneWeights[i][j];
			}

			// Fixup weights if they don't add up

			if ((totalweight!=1.0f) && (totalweight!=0.0f))
			{
				float newtotalweight=0.0f;
				for (int j=0;j<mNumBones;j++)
				{
					mBoneWeights[i][j]*=(1/totalweight);
					newtotalweight+=mBoneWeights[i][j];
				}
				ASSERT(fabsf(newtotalweight-1.0f)<0.01);
			}

			// Convert the bone weights into a list of bone "slots" for the
			// optimised renderer
			mBoneSlots[i]=new( MEMTYPE_MESH ) int[BONE_SLOTS];

			float *tempweights=new( MEMTYPE_MESH ) float[mNumBones];

			for (j=0;j<mNumBones;j++)
				tempweights[j]=mBoneWeights[i][j];
			
			for (j=0;j<BONE_SLOTS;j++)
			{
				float best=0;
				int bestn=0;
				for (int k=0;k<mNumBones;k++)
				{
					if (tempweights[k]>best)
					{
						best=tempweights[k];
						bestn=k;
					}
				}
				if (best==0.0f)
				{
					if (j==0)
						mBoneSlots[i][j]=-1;
					else
						mBoneSlots[i][j]=mBoneSlots[i][j-1];
				}
				else
					mBoneSlots[i][j]=bestn;
				tempweights[bestn]-=(1/(float) BONE_SLOTS);
			}

			SAFE_DELETE(tempweights);
		}
	}

//	char	foo[100];
//	sprintf(foo, "Mesh %s: AFrames: %d\n", this->mBaseMesh->mName, mNumAFrames);
//	OutputDebugString(foo);

	// JCL - now with normals!

	CMeshDVertex	*dverts = new( MEMTYPE_MESH ) CMeshDVertex[mNumDVertices * 5];  // lots of temp verts
	SINT			num_dverts = 0;

	// load Triangles
	for (c0 = 0; c0 < mNumTriangles; c0++)
	{
		SINT a, b, c;

		mem.Read(&a, 4);
		mem.Read(&b, 4);
		mem.Read(&c, 4);

		ASSERT(a < mNumDVertices);
		ASSERT(b < mNumDVertices);
		ASSERT(c < mNumDVertices);
		ASSERT(a >= 0);
		ASSERT(b >= 0);
		ASSERT(c >= 0);

		mTriangles[c0].mA = &mDVertices[a];
		mTriangles[c0].mB = &mDVertices[c];
		mTriangles[c0].mC = &mDVertices[b]; // inside out

		// OK - read in normal, and create more vertices if necessary....
		SINT	c1, c2;

		for (c1 = 0; c1 < 3; c1 ++)
		{
			CMeshDVertex	**dp;

			switch (c1)
			{
			case 0:	dp = &mTriangles[c0].mA; break;	//!! sorry - order swapped as above
			case 1:	dp = &mTriangles[c0].mC; break;
			case 2:	dp = &mTriangles[c0].mB; break;   
			}

			FVector normal;
			normal.Load(mem) ;

		//	mem.Read(&normal, 12);

			// negate the normal
			//normal.Z = -normal.Z;

			// does this vertex have a normal?
			if ((*dp)->mNormal.X == -10.f)
			{
				// nope - make it so!
				(*dp)->mNormal = normal;
			}
			else if ((*dp)->mNormal == normal)
			{
				// same - so nothing to do
			}
			else
			{
				// ok - need to create a new vertex?
				// first - have we created this already?
				BOOL	done = FALSE;
				for (c2 = 0; c2 < num_dverts; c2 ++)
				{
					// is this the same vert?
					if ((dverts[c2].mPVert == (*dp)->mPVert) &&
						(dverts[c2].mTexture[0] == (*dp)->mTexture[0]) &&
					    (dverts[c2].mNormal == normal))
					{
						// cool - use this
						*dp = &mDVertices[mNumDVertices + c2]; // note - invalid.  must relocate later
						done = TRUE;
						break;
					}
				}
				if (!done)
				{
					// oh dear - must create new vertex...
					dverts[num_dverts] = **dp;
					dverts[num_dverts].mNormal		= normal;
					*dp = &mDVertices[mNumDVertices + num_dverts]; // note - invalid.  must relocate later

					num_dverts ++;
				}
			}
		}
	}

	// Ok - now we need to build a new DVerts array
	if (num_dverts > 0)
	{
		CMeshDVertex	*old_dverts = mDVertices;

		mDVertices = new( MEMTYPE_MESH ) CMeshDVertex[mNumDVertices + num_dverts];

		// copy data
		memcpy(mDVertices, old_dverts, sizeof(CMeshDVertex) * mNumDVertices);
		memcpy(mDVertices + mNumDVertices, dverts, sizeof(CMeshDVertex) * num_dverts);

		// free old data
		delete [] old_dverts;

		// and relocate
		for (c0 = 0; c0 < mNumTriangles; c0++)
		{
			mTriangles[c0].mA = (mTriangles[c0].mA - old_dverts) + mDVertices;
			mTriangles[c0].mB = (mTriangles[c0].mB - old_dverts) + mDVertices;
			mTriangles[c0].mC = (mTriangles[c0].mC - old_dverts) + mDVertices;
		}

		mNumDVertices += num_dverts;
	}

	delete [] dverts;

//	MakeEBR();
	EvaluateNormals(false);
}

/*
//******************************************************************************************
void	CMeshPart::AddEdge(UWORD va, UWORD vb, UWORD faceindex)
{
	EBREdge*	edge = mEBR;
	for(int i=0; i<mNEBREdges; edge++, i++)
	{
		if((va==edge->VertB)&&(vb==edge->VertA))
		{
			if(edge->FaceB!=EBR_NOTUSED) return;
			edge->FaceB=faceindex;
			return;
		}
		if((va==edge->VertA)&&(vb==edge->VertB)) return;
	}
	edge->VertA=va;
	edge->VertB=vb;
	edge->FaceA=faceindex;
	edge->FaceB=EBR_NOTUSED;
	mNEBREdges++;
	ASSERT(mNEBREdges<65536);
}
//******************************************************************************************
void	CMeshPart::MakeEBR()
{
	mEBR = new EBREdge[65536];
	mNEBREdges = 0;

	UWORD*	vi = new UWORD[65536];

	for(UWORD c=0; c<mNumDVertices; c++)
	{
		for(UWORD ic=0; ic<c; ic++)
		{
			if(mDVertices[c].mPVert == mDVertices[ic].mPVert)	break;
		}
		vi[c]=ic;
	}
	
	CMeshTriangle* tri = mTriangles;
	for(int i = 0; i<mNumTriangles; tri++, i++)
	{
		AddEdge(vi[tri->mA-mDVertices], vi[tri->mB-mDVertices], i); 
		AddEdge(vi[tri->mB-mDVertices], vi[tri->mC-mDVertices], i); 
		AddEdge(vi[tri->mC-mDVertices], vi[tri->mA-mDVertices], i); 
	}
	delete vi;
	EBREdge* t = new EBREdge[mNEBREdges];
	memcpy(t, mEBR, mNEBREdges*sizeof(EBREdge));
	delete mEBR;
	mEBR = t;
}
*/
//******************************************************************************************
void	CMeshPart::HierarchyAdjust(FVector pos, FMatrix ori, CMESHPART *parent)
{
#if TARGET == XBOX
	SASSERT(0, "Code removed on XBOX");
#else
	// Process children *before* we put this node into localspace!

	SINT	c0;

	for (c0 = 0; c0 < mNumChildren; c0 ++)
	{
		if (mChildren[c0])
			mChildren[c0]->HierarchyAdjust(mOffsetPos, mOrientation, (CMESHPART*)this);
	}
	
	// sort out the hierarchy so that all transformations are relative to the parent node

	mOffsetPos -= pos;
	mOffsetPos = ori.Transpose() * mOffsetPos;
	mOrientation = mOrientation * ori.Transpose();

	// Note : This needs to match V->H frame mapping by time!

	for (int i=0;i<mNumHFrames;i++)
	{
		if (parent)
		{
			if (parent->mNumHFrames==i) 
			{
				ori=parent->mHFrameOrientations[i];
				pos=parent->mHFramePositions[i];
			}
		}
		mHFramePositions[i] -= pos;
		mHFramePositions[i] = ori.Transpose() * mHFramePositions[i];
		mHFrameOrientations[i]=ori.Transpose() * mHFrameOrientations[i];
	}
#endif
}

//******************************************************************************************
SINT	CMeshPart::FindVertex(FVector pos)
{
	if ((!mPVertices) || (!mPVertices[0]))
		return 0;

	// Look for nearby vertex
	float	min = 99999.f;
	SINT	minn = 0;

	SINT	c0;

	for (c0 = 0; c0 < mNumPVertices; c0 ++)
	{
		float d = (mPVertices[0][c0] - pos).Magnitude();

		if (d < min)
		{
			min = d;
			minn = c0;
		}
	}

	return minn;
}

//******************************************************************************************
// This returns the part that actually contains mesh data for this part, or NULL if there
// is no mesh data
//******************************************************************************************

CMESHPART *CMeshPart::GetMeshDataPart()
{
	if ((mPartType==MESHPART_TYPE_MESH) || (mPartType==MESHPART_TYPE_SKELETAL))
		return((CMESHPART*)(this));
	if (mPartType==MESHPART_TYPE_REFERENCE)
		return(mReferenceTo);

	return(NULL);
}


//******************************************************************************************
// Returns the position & orientation for this part *ONLY* at the (v)frame given
//******************************************************************************************

void CMeshPart::GetTransforms(SINT vf1,SINT vf2,float vffrac, FMatrix *ori,FVector *pos)
{
/*	int hframe = (int) mVHFrameMap[frame];
	*pos=mHFramePositions[hframe];
	*ori=mHFrameOrientations[hframe];*/

	UBYTE hframe1,hframe2;

	if (vf1>=mNumVFrames)
		vf1=mNumVFrames-1;
	if (vf2>=mNumVFrames)
		vf2=mNumVFrames-1;
	
	hframe1 = mVHFrameMap[vf1];
	hframe2 = mVHFrameMap[vf2];
	
	*pos=mHFramePositions[hframe1]+((mHFramePositions[hframe2]-mHFramePositions[hframe1])*vffrac);
	*ori=GetHFrameOrientation(hframe1)+((GetHFrameOrientation(hframe2)-GetHFrameOrientation(hframe1))*vffrac);

	// Technically we should normalise this, but that screws up because some of the 
	// matrices are flipped. In fact the normalisation doesn't really matter much, as
	// over the steps involved the error should be negligable.
	
	// ori->Normalise();
}

#ifndef SIMPLE_MESHES
#include "debuglog.h"
#endif

//******************************************************************************************
// Returns the world transformations (ie the composite of the transforms for this
// part and all its parents) at the frame given
//******************************************************************************************

void CMeshPart::GetWorldTransforms(SINT vf1,SINT vf2,float vffrac,IRenderableThing *t,FMatrix *ori,FVector *pos,bool inmodelspace,bool usecontrollers,bool forcenocache)
{
	PROFILE_FN(GetWorldTransforms);

#ifndef SIMPLE_MESHES
	ULONG frame_count  = GAME.GetRenderFrameNumber();
#else
	ULONG frame_count = 1;
#endif

	CMotionController *c;
#ifndef SIMPLE_MESHES
	if ((t) && (usecontrollers))
		c=t->GetMotionController();
	else
#endif
		c=NULL;
	
	if ((t != NULL && t ==  mCachedRenderThing) && (mCachedMeshPart==this) && (!c))
	{
	
		if (inmodelspace == false &&
			frame_count == mLastCachedWorldTransCount &&
			vf1 == mLastCachedWorldVF)
		{
			*ori = mLastCachedWorldOri;
			*pos = mLastCachedWorldPos ;
			return ;
		}
		else if (inmodelspace == true &&
			     frame_count == mLastCachedModelTransCount &&
				 vf1 == mLastCachedModelVF)
		{
			*ori = mLastCachedModelOri;
			*pos = mLastCachedModelPos ;

			return ;
		}
	}

	FMatrix parentori;
	FVector parentpos;
	
#ifndef SIMPLE_MESHES
	if ( ((!c) && (!forcenocache) && (mNumCachedFrames>0)) )// && (!LT.xKeyOn('A')) )
	{
		PROFILE_COUNT(GWTCacheHits);

		CMeshPart *pospart=this;
		CMeshPart *oripart=this;

		while (pospart->mPositionStatic)
		{
			pospart=pospart->mParent;
		}

		while (oripart->mOrientationStatic)
		{
			oripart=oripart->mParent;
		}

		ASSERT(pospart);
		ASSERT(oripart);

		int vf1pos=vf1;
		int vf2pos=vf2;
		int vf1ori=vf1;
		int vf2ori=vf2;
		
		if (vf1pos>=pospart->mNumCachedFrames)
			vf1pos=pospart->mNumCachedFrames-1;
		if (vf2pos>=pospart->mNumCachedFrames)
			vf2pos=pospart->mNumCachedFrames-1;

		if (vf1ori>=oripart->mNumCachedFrames)
			vf1ori=oripart->mNumCachedFrames-1;
		if (vf2ori>=oripart->mNumCachedFrames)
			vf2ori=oripart->mNumCachedFrames-1;

		*pos=pospart->mCachedPosition[vf1pos]+((pospart->mCachedPosition[vf2pos]-pospart->mCachedPosition[vf1pos])*vffrac);
		*ori=oripart->GetCachedOrientation(vf1ori)+((oripart->GetCachedOrientation(vf2ori)-oripart->GetCachedOrientation(vf1ori))*vffrac);

		// Technically we should normalise this, but that screws up because some of the 
		// matrices are flipped. In fact the normalisation doesn't really matter much, as
		// over the steps involved the error should be negligable.		

		// ori->Normalise();

		if ((!inmodelspace) && (t))
		{
			// Add world position

			FMatrix tori=t->GetRenderOrientation();

			(*ori) = tori * (*ori);
			(*pos) = t->GetRenderPos() + (tori * (*pos));
		}

		if (inmodelspace==false)
		{
			mLastCachedWorldOri = *ori;
			mLastCachedWorldPos = *pos;
			mLastCachedWorldVF = vf1 ;
			mLastCachedWorldTransCount = frame_count;
		}
		else
		{
			mLastCachedModelOri = *ori;
			mLastCachedModelPos = *pos;
			mLastCachedModelVF = vf1 ;
			mLastCachedModelTransCount = frame_count;
		}
		mCachedMeshPart=this;
		mCachedRenderThing = t ;

		return;
	}
#endif

	PROFILE_COUNT(GWTCacheMisses);

	if (!mParent)
	{
		// Root node
		parentori.MakeID();
		parentpos.Set(0,0,0);
	}
	else
	{
		mParent->GetWorldTransforms(vf1,vf2,vffrac,t,&parentori,&parentpos,inmodelspace,usecontrollers,forcenocache);
	}

	GetTransforms(vf1,vf2,vffrac,ori,pos);

#ifndef SIMPLE_MESHES
	if (t)
	{
		CMotionController *c=t->GetMotionController();

		if ((c) && (usecontrollers))
			c->UpdateRenderPositionPre((CMESHPART*)this,&parentpos,&parentori,inmodelspace);

		if ((!inmodelspace) && (mPartNumber==0))
		{
			// Add world position
			parentpos += t->GetRenderPos();
			parentori = parentori * t->GetRenderOrientation();
		}
	}
#endif

	parentpos += parentori * (*pos);
	parentori = parentori * (*ori);

#ifndef SIMPLE_MESHES
	if (t)
	{
		CMotionController *c=t->GetMotionController();
		if ((c) && (usecontrollers))
			c->UpdateRenderPositionPost((CMESHPART*)this,&parentpos,&parentori,inmodelspace);
	}
#endif

	(*pos)=parentpos;
	(*ori)=parentori;

	if (inmodelspace==false)
	{
		mLastCachedWorldOri = parentori;
		mLastCachedWorldPos = parentpos;
		mLastCachedWorldVF	= vf1 ;
		mLastCachedWorldTransCount = frame_count;
	}
	else
	{
		mLastCachedModelOri = parentori;
		mLastCachedModelPos = parentpos;
		mLastCachedModelVF = vf1 ;
		mLastCachedModelTransCount = frame_count;
	}
	mCachedRenderThing = t ;
}

//******************************************************************************************
// Builds a position cache for the specified mesh part
//******************************************************************************************

/*int cachesaved=0;
int cachesize=0;*/

void CMeshPart::BuildCache()
{
#if TARGET == XBOX
	SASSERT(0, "Code removed");
#else
	mNumCachedFrames=1;

	if (mNumHFrames>1)
		mNumCachedFrames=mNumVFrames;

	CMeshPart *c=mParent;

	while (c)
	{
		if (c->mNumHFrames>1)
			mNumCachedFrames=mNumVFrames;
		c=c->mParent;
	}

	if ((mNumHFrames==1) && (mParent))
	{
		if ((GetHFrameOrientation(0).Row[0].X==1.0f) &&
			(GetHFrameOrientation(0).Row[0].Y==0.0f) &&
			(GetHFrameOrientation(0).Row[0].Z==0.0f) &&
			
			(GetHFrameOrientation(0).Row[1].X==0.0f) &&
			(GetHFrameOrientation(0).Row[1].Y==1.0f) &&
			(GetHFrameOrientation(0).Row[1].Z==0.0f) &&
			
			(GetHFrameOrientation(0).Row[2].X==0.0f) &&
			(GetHFrameOrientation(0).Row[2].Y==0.0f) &&
			(GetHFrameOrientation(0).Row[2].Z==1.0f))
		{
			// Static orientation
			mOrientationStatic=TRUE;
		}
		
		if ((mHFramePositions[0].X==0.0f) &&
			(mHFramePositions[0].Y==0.0f) &&
			(mHFramePositions[0].Z==0.0f))
		{
			// Static position
			mPositionStatic=TRUE;
		}
	}
/*
	if ((mPositionStatic) && (mOrientationStatic))
	{
		TRACE("Entirely static node!\n");
//		cachesaved+=(sizeof(FVector)+sizeof(FMatrix))*mNumCachedFrames;
		mNumCachedFrames=0;
		return;
	}

	if (mNumHFrames==1)
	{
		// Node is not static, but there's little or no point caching data for it
		mNumCachedFrames=0;
		return;
	}
*/
	char buf[256];
	sprintf(buf,"Mesh %s part %s cached %d frames\n",mBaseMesh->mName,mName,mNumCachedFrames);
	TRACE(buf);

	if (!mPositionStatic)
	{
		mCachedPosition=new (MEMTYPE_MESHCACHE) FVector[mNumCachedFrames];
	}
	if (!mOrientationStatic)
	{
		mCachedOrientation=new (MEMTYPE_MESHCACHE) FMatrix[mNumCachedFrames];
	}

	for (int i=0;i<mNumCachedFrames;i++)
	{
		FMatrix ori;
		FVector pos;
		GetWorldTransforms(i,i,0.0f,NULL,&ori,&pos,true,false,true);
		if (!mOrientationStatic)
			mCachedOrientation[i]=ori;
		if (!mPositionStatic)
			mCachedPosition[i]=pos;
	}

	int size=0;
	if (!mPositionStatic)
		size+=sizeof(FVector);
	if (!mOrientationStatic)
		size+=sizeof(FMatrix);

/*	cachesize+=size*mNumCachedFrames;
	cachesaved+=((sizeof(FVector)+sizeof(FMatrix))*mNumCachedFrames)-(size*mNumCachedFrames);

	char buf[256];
	sprintf(buf,"Cache size=%dK\nSaved=%dK\n",cachesize/1024,cachesaved/1024);
	TRACE(buf);*/
	
	PROFILE_ALLOCATE(MeshPartCacheMemory,size*mNumCachedFrames);
#endif
}

//******************************************************************************************
// Builds the meshpart chain for a given meshpart
//******************************************************************************************

void CMeshPart::BuildChain()
{
	CMESHPART *chainentry[MAX_CHAIN_ENTRIES];

	int i;
	for (i=0;i<MAX_CHAIN_ENTRIES;i++)
		chainentry[i]=NULL;

	int numchained=0;

	for (i=0;i<mBaseMesh->GetNumParts();i++)
	{
		if (strnicmp(mName,mBaseMesh->GetPart(i)->mName,strlen(mName))==0)
		{
			// Names begin the same
			// Extract tag
			char *tag=&mBaseMesh->GetPart(i)->mName[strlen(mName)];

			if (strlen(tag)>=8)
			{
				if (strnicmp(tag,"_damaged",8)==0) // Add extra tags here as necessary
				{
					int entryno=1;
					if (strlen(tag)>8)
						entryno=atoi(&tag[8]);
					ASSERT(entryno>=0);
					ASSERT(entryno<MAX_CHAIN_ENTRIES);
					if (chainentry[entryno]!=NULL)
					{
						char mbuf[256];
						sprintf(mbuf,"Duplicate damage number for part %s!",mBaseMesh->GetPart(i)->mName);
						SASSERT(0,mbuf);
					}
					chainentry[entryno]=mBaseMesh->GetPart(i);
					numchained++;
				}
			}
		}
	}

	CMESHPART  *last= (CMESHPART *)this;

	for (i=0;i<MAX_CHAIN_ENTRIES;i++)
	{
		if (chainentry[i])
		{
			last->mNextMeshPartInChain=chainentry[i];
			last->mNumChainedParts=numchained;
			chainentry[i]->mIsChained=true;
//			char buffer[256];
//			sprintf(buffer,"Chained %s to %s",chainentry[i]->mName,last->mName);
//			OutputDebugString(buffer);
			last=chainentry[i];
			numchained--;
		}
	}
}


//******************************************************************************************
void	CMeshPart::EvaluateNormals(bool replaceexisting)
{
	SINT	c0;

	//! JCL.....
	if (mNumDVertices > 10000)
		return;

	// evaluate normals. (potentially slow...)
	for (c0 = 0; c0 < mNumDVertices; c0 ++)
	{
		FVector n = ZERO_FVECTOR;
		FVector sn = ZERO_FVECTOR;
		SINT	nn = 0;
		SINT	snn = 0;

		SINT c1;

		for (c1 = 0; c1 < mNumTriangles; c1++)
		{
			if ((mTriangles[c1].mA->mPVert == mDVertices[c0].mPVert) ||
				(mTriangles[c1].mB->mPVert == mDVertices[c0].mPVert) ||
				(mTriangles[c1].mC->mPVert == mDVertices[c0].mPVert))
			{
				// Calculate "smoothed" normal (ie all normals have C1 across whole(?) model...

				FVector normal = (mPVertices[0][mTriangles[c1].mA->mPVert] - mPVertices[0][mTriangles[c1].mB->mPVert]) ^
								 (mPVertices[0][mTriangles[c1].mA->mPVert] - mPVertices[0][mTriangles[c1].mC->mPVert]);
				
				normal.Normalise();
				
				sn = sn + normal;
				snn ++;
			}

			if ((mTriangles[c1].mA == &mDVertices[c0]) ||
				(mTriangles[c1].mB == &mDVertices[c0]) ||
				(mTriangles[c1].mC == &mDVertices[c0]))
			{
				// Calculate "real" normal
				//! use frame 0...

				FVector normal = (mPVertices[0][mTriangles[c1].mA->mPVert] - mPVertices[0][mTriangles[c1].mB->mPVert]) ^
								 (mPVertices[0][mTriangles[c1].mA->mPVert] - mPVertices[0][mTriangles[c1].mC->mPVert]);

				normal.Normalise();

				n = n + normal;
				nn ++;
			}
		}

		if (replaceexisting)
		{
			if (nn == 0)
			{
				// eh?  vertex without face...
				mDVertices[c0].mNormal = FVector(1.f, 0, 0);
			}
			else
			{
				if (n.Magnitude() < 0.001f)
					mDVertices[c0].mNormal = FVector(1.f, 0, 0);  // very odd case...
				else
				{
					n.Normalise();
					mDVertices[c0].mNormal = n;
				}
			}
			
			// Flip Z
			
			//mDVertices[c0].mNormal.Z=-mDVertices[c0].mNormal.Z;
		}

		if (snn == 0)
		{
			// No faces
			mDVertices[c0].mSmoothNormal = FVector(1.0f,0.0f,0.0f);
		}
		else
		{
			sn.Normalise();
			mDVertices[c0].mSmoothNormal = sn;
		}
	}
}

//******************************************************************************************

SINT CMeshPart::ResolveFrameNumber(float frame,int animind,float &offset,CMotionController *mc)
{
	SINT vframe = 0;
	float vframeoffset = 0;

#ifndef SIMPLE_MESHES
	
	if 	((mBaseMesh->mNumAnimModes) &&
		 (mNumVFrames>1) &&
		 (animind>=0))
	{
		CAnimMode*	am = &mBaseMesh->mAnimModes[animind];
		
		float fvframe=frame;
		fvframe*=am->mFrameScaler; 
		fvframe+=am->mStart;
		if (mc)
			mc->UpdateRenderFrame((CMESHPART *)this,&fvframe);
		vframe = (SINT(floorf(fvframe))) % mNumVFrames;
		vframeoffset=(float) (fvframe-floorf(fvframe));
	}
	else
	{
		float fvframe = 0.0f;
		// No animation
		if (mc)
			mc->UpdateRenderFrame((CMESHPART *)this,&fvframe);
		vframe = (SINT(floorf(fvframe)));
		vframeoffset=(float) (fvframe-floorf(fvframe));
	}
#endif
	
	offset=vframeoffset;
	
	return(vframe);
}

//******************************************************************************************
#ifndef SIMPLE_MESHES
#ifdef RESBUILDER

// a backdoor for the few meshes that look obviously shit with 8-byte matrixes.
bool allowed_to_compress = true;

void CMeshPart::Serialize(class CChunker *c,class CResourceAccumulator *ra)
{
	int i;

	// All polybuckets we need should be built by now!

	// Delete vertex/poly data as it's not needed any more

	SASSERT((void *) &mOrientation==(void *) this,"It looks like you've added a vtable to CMeshPart! This needs special code in Serialize()/Deserialize()!");		

	BYTE *temp=new (MEMTYPE_SCRATCHPAD) BYTE[sizeof(CMESHPART)];
	CMESHPART *t=(CMESHPART *) temp;
	memcpy(t,this,sizeof(CMESHPART));
	
	// Clear all pointers
	
	SERIALIZE_CLEAR(t->mBaseMesh);
	SERIALIZE_CLEAR(t->mBones);
	SERIALIZE_CLEAR(t->mCachedOrientation);
	SERIALIZE_CLEAR(t->mCachedPosition);
	SERIALIZE_CLEAR(t->mCachedRenderThing);
	SERIALIZE_CLEAR(t->mChildren);
	SERIALIZE_CLEAR(t->mDVertices);
	SERIALIZE_CLEAR(t->mHFrameFOVs);
	SERIALIZE_CLEAR(t->mHFrameOrientations);
	SERIALIZE_CLEAR(t->mHFramePositions);
	SERIALIZE_CLEAR(t->mNextMeshPartInChain);
	SERIALIZE_CLEAR(t->mParent);
	SERIALIZE_CLEAR(t->mPolyBucket);
	SERIALIZE_CLEAR(t->mPVertices);
	SERIALIZE_CLEAR(t->mReferenceTo);
	SERIALIZE_CLEAR(t->mTriangles);
//	SERIALIZE_CLEAR(t->mVFrames);
	SERIALIZE_CLEAR(t->mVHFrameMap);
	SERIALIZE_CLEAR(t->mBoundingBox);

	// only the XBOX likes compressed matrixes.
	if (ra->GetTargetPlatform() != XBOX)
		allowed_to_compress = FALSE;

	// Clear vertex data as it's not needed, not ever.
	if (1)// (ra->GetTargetPlatform()!=XBOX) || (mNumBones==0))
	{
		t->mNumDVertices=0;
		t->mNumPVertices=0;
		t->mNumTriangles=0;
		t->mBoneSlots=NULL;
		t->mBoneWeights=NULL;
	}
		
	c->Start(MKID("MESP"));
	
	c->Start(MKID("CMSP"));	
	c->Write(t,sizeof(CMESHPART),1);
	c->End();

	if (t->mNumDVertices>0)
	{
		c->Start(MKID("DVRT"));

		int dvsize=sizeof(CMeshDVertex);

		c->Write(&dvsize,sizeof(dvsize),1);

		for (i=0;i<mNumDVertices;i++)
			c->Write(&mDVertices[i],dvsize,1);

		c->End();
	}

	if (t->mNumTriangles>0)
	{
		c->Start(MKID("TRIS"));
		for (i=0;i<mNumTriangles;i++)
		{
			UINT tri[3];
			tri[0]=mTriangles[i].mA-mDVertices;
			tri[1]=mTriangles[i].mB-mDVertices;
			tri[2]=mTriangles[i].mC-mDVertices;

			c->Write(&tri,sizeof(UINT),3);
		}
		c->End();
	}

	if (t->mNumPVertices>0)
	{
		c->Start(MKID("PVRT"));
		for (i=0;i<mNumAFrames;i++)
			c->Write(mPVertices[i],sizeof(FVector),mNumPVertices);
		c->End();
	}

	if (mNumChildren>0)
	{
		c->Start(MKID("CHLD"));
		for (i=0;i<mNumChildren;i++)
		{
			int pnum=mChildren[i]->mPartNumber;
			c->Write(&pnum,sizeof(pnum),1);
		}
		c->End();
	}	

	if (mParent)
	{
		c->Start(MKID("PRNT"));
		int pnum=mParent->mPartNumber;
		c->Write(&pnum,sizeof(pnum),1);
		c->End();
	}
	
	if (mNextMeshPartInChain)
	{
		c->Start(MKID("NMIC"));
		int pnum=mNextMeshPartInChain->mPartNumber;
		c->Write(&pnum,sizeof(pnum),1);
		c->End();
	}	

	c->Start(MKID("BBOX"));
	mBoundingBox->Serialize(c,ra);
	c->End();

	if (mNumVFrames>0)
	{
		if (mVHFrameMap)
		{
			c->Start(MKID("VHFM"));
			c->Write(mVHFrameMap,sizeof(UBYTE),mNumVFrames);
			c->End();
		}
	}
	
	if (mNumHFrames>0)
	{
		if (mHFrameOrientations)
		{
			// Now are we allowed to compress?
			if (allowed_to_compress)
			{
				c->Start(MKID("HORC"));
			
				// We have to compress the matrices.
				FSmallerMatrix *small_mat = new (MEMTYPE_SCRATCHPAD) FSmallerMatrix[mNumHFrames];

				for (int i = 0; i < mNumHFrames; i++)
				{
					small_mat[i] = mHFrameOrientations[i];
				}

				c->Write(small_mat, sizeof(FSmallerMatrix), mNumHFrames);

				delete[] small_mat;
			}
			else
			{
				c->Start(MKID("HORI"));
			
				// no compression
				c->Write(mHFrameOrientations,sizeof(FMatrix),mNumHFrames);
			}

			c->End();
		}
		
		if (mHFramePositions)
		{
			c->Start(MKID("HPOS"));
			c->Write(mHFramePositions,sizeof(FVector),mNumHFrames);
			c->End();
		}
		
		if (mHFrameFOVs)
		{
			c->Start(MKID("HFOV"));
			c->Write(mHFrameFOVs,sizeof(float),mNumHFrames);
			c->End();		
		}
	}

	if (mNumBones>0)
	{
		c->Start(MKID("BONE"));
		for (i=0;i<mNumBones;i++)
		{
			int pnum=mBones[i]->mPartNumber;
			c->Write(&pnum,sizeof(pnum),1);
		}
		c->End();

		if (t->mBoneWeights)
		{
			c->Start(MKID("BONW"));
			for (int i=0;i<mNumPVertices;i++)
			{
				c->Write(mBoneWeights[i],sizeof(float),mNumBones);
			}
			c->End();	
		}

		if (t->mBoneSlots)
		{
			c->Start(MKID("BONS"));
			for (int i=0;i<mNumPVertices;i++)
			{
				c->Write(mBoneSlots[i],sizeof(int),BONE_SLOTS);
			}
			c->End();
		}
	}

	if (mPolyBucket)
		mPolyBucket->Serialize(c,this);

	if (mCachedPosition)
	{
		c->Start(MKID("CPOS"));
		c->Write(mCachedPosition,sizeof(FVector),mNumCachedFrames);
		c->End();
	}

	if (mCachedOrientation)
	{
		if (allowed_to_compress)
		{
			c->Start(MKID("CORC"));

			// We have to compress the matrices.
			FSmallerMatrix *small_mat = new (MEMTYPE_SCRATCHPAD) FSmallerMatrix[mNumCachedFrames];

			for (int i = 0; i < mNumCachedFrames; i++)
			{
				small_mat[i] = mCachedOrientation[i];
			}

			c->Write(small_mat, sizeof(FSmallerMatrix), mNumCachedFrames);

			delete[] small_mat;
		}
		else
		{
			c->Start(MKID("CORI"));

			// no compression.
			c->Write(mCachedOrientation,sizeof(FMatrix),mNumCachedFrames);
		}

		c->End();	
	}

	if (mReferenceTo)
	{
		c->Start(MKID("REFR"));
		int pnum=mReferenceTo->mPartNumber;
		c->Write(&pnum,sizeof(pnum),1);
		c->End();
	}

	delete temp;

	// MeshVB

#if TARGET==PC
	mMeshVB->Serialize(c,ra);
#endif

	c->End();
}
#endif
//******************************************************************************************


void CMeshPart::WipeVertexData()
{
	SAFE_DELETE_ARRAY(mDVertices);

	if (mPVertices)
	{
		SINT	c0;
		for (c0 = 0; c0 < mNumAFrames; c0 ++)
		{
			if (mPVertices[c0])
				SAFE_DELETE_ARRAY(mPVertices[c0]);
		}
		
		SAFE_DELETE_ARRAY(mPVertices);
	}
	
	SAFE_DELETE_ARRAY(mTriangles);

	mNumDVertices=0;
	mNumPVertices=0;
	mNumTriangles=0;
}

//******************************************************************************************
FVector	CMeshPart::GetRandomVertexPos()
{
	FVector return_vec = FVector(0.0f,0.0f,0.0f);

	if (mPolyBucket == NULL)
	{
		LOG.AddMessage("FATAL ERROR: can't get ranom vertex (no poly bucket defined)");
		return return_vec ;
	}

	CCollisionTriangle tri ;

	if (mPolyBucket->GetRandomTriangle(&tri) == TRUE)
	{
		int s1 = GAME.Random() % 3;
		switch (s1)
		{
			case 0: tri.mP1->GetPoint(&return_vec,mPolyBucket->GetCompressionData());break ;
			case 1: tri.mP2->GetPoint(&return_vec,mPolyBucket->GetCompressionData());break ;
			case 2: tri.mP3->GetPoint(&return_vec,mPolyBucket->GetCompressionData());break ;
		
		}
		return return_vec ;
	}

	return return_vec ;
}


//******************************************************************************************

int ocachesize=0;

CMESHPART *CMeshPart::Deserialize(class CChunkReader *c,CMESHPART *n,CMesh *mesh)
{
	int i;

	// Make sure the bounding box has been deleted
	// Actually, don't, why bother? We'll only allocate it later. So let's keep it.
	// But in the spirit of paranoia we'll set it to NULL just in case.
	CBoundingBox *oldbbox = n->mBoundingBox;
	n->mBoundingBox = NULL;

	// Save pointers

	CMESHVB *meshvb=n->mMeshVB;

	// Load
	
	if (c->GetNext()!=MKID("MESP"))
	{
		SASSERT(0,"CMeshPart deserialize format failure!");
	}
	
	if (c->GetNext()!=MKID("CMSP"))
	{
		SASSERT(0,"CMeshPart deserialize format failure!");	
	}

	// Deal with the PS2 padding the size to align stuff

	int correctedsize=c->GetSize();
#if TARGET==PS2
	while ((correctedsize & 0xF)!=0)
		correctedsize++;
#endif
	
	if (correctedsize!=sizeof(CMESHPART))
	{
		printf("Size=%d, sizeof=%d\n",c->GetSize(),sizeof(CMESHPART));
		SASSERT(0,"CMeshPart deserialize format failure!");
	}
	
	c->Read(n,c->GetSize(),1);

	// Set base mesh
	
	n->mBaseMesh=(CMESH *) mesh;

	// Allocate memory

	n->InitMemory(n->mNumDVertices,n->mNumPVertices,n->mNumTriangles,n->mNumVFrames,n->mNumAFrames);

	if (n->mNumVFrames>0)
		n->mVHFrameMap = new( MEMTYPE_MESH ) UBYTE[n->mNumVFrames];

	if (n->mNumHFrames>0)
	{
		// We don't do the hframeorientations here because we don't know if they'll be compressed or not.
		if (n->mHFramePositions)
			n->mHFramePositions = new( MEMTYPE_MESH ) FVector[n->mNumHFrames];
		if (n->mHFrameFOVs)
			n->mHFrameFOVs = new( MEMTYPE_MESH ) float[n->mNumHFrames];

		MEM_MANAGER.TagMem(n->mHFramePositions,sizeof(FVector)*n->mNumHFrames,"Meshes/%s/Part %d/Position data",n->mBaseMesh->mName,n->mPartNumber);
		MEM_MANAGER.TagMem(n->mHFrameFOVs,sizeof(float)*n->mNumHFrames,"Meshes/%s/Part %d/FOV data",n->mBaseMesh->mName,n->mPartNumber);		
	}

	if (n->mNumBones>0)
	{
		if (n->mBones)
			n->mBones = new( MEMTYPE_MESH ) CMESHPART*[n->mNumBones];
		if (n->mNumPVertices>0)
		{
			if (n->mBoneWeights)
				n->mBoneWeights = new( MEMTYPE_MESH ) float*[n->mNumPVertices];
			if (n->mBoneSlots)
				n->mBoneSlots = new( MEMTYPE_MESH ) int*[n->mNumPVertices];
			for (i=0;i<n->mNumPVertices;i++)
			{
				n->mBoneWeights[i]=new( MEMTYPE_MESH ) float[n->mNumBones];
				n->mBoneSlots[i]=new( MEMTYPE_MESH ) int[BONE_SLOTS];
			}

			MEM_MANAGER.TagMem(n->mBoneWeights,sizeof(float)*n->mNumPVertices*n->mNumBones,"Meshes/%s/Part %d/Bone weights",n->mBaseMesh->mName,n->mPartNumber);
			MEM_MANAGER.TagMem(n->mBoneSlots,sizeof(int)*n->mNumPVertices*BONE_SLOTS,"Meshes/%s/Part %d/Bone slots",n->mBaseMesh->mName,n->mPartNumber);			
		}
	}
	
	
	if (n->mNumDVertices>0)
	{
		if (c->GetNext()!=MKID("DVRT"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");	
		}	

		int dvsize;
		c->Read(&dvsize,sizeof(dvsize),1);

		int correctedsize=dvsize;
#if TARGET==PS2
		while ((correctedsize & 0xF)!=0)
			correctedsize++;
#endif
		
		if (correctedsize!=sizeof(CMeshDVertex))
		{
			printf("Size=%d, sizeof=%d\n",correctedsize,sizeof(CMeshDVertex));
			SASSERT(0,"CMeshDVertex deserialize format failure!");
		}
		
		for (i=0;i<n->mNumDVertices;i++)
		{
			c->Read(&n->mDVertices[i],dvsize,1);
		}

	}

	if (n->mNumTriangles>0)
	{
		if (c->GetNext()!=MKID("TRIS"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");	
		}

		for (i=0;i<n->mNumTriangles;i++)
		{
			UINT tri[3];
			c->Read(&tri,sizeof(UINT),3);

			n->mTriangles[i].mA=tri[0]+n->mDVertices;
			n->mTriangles[i].mB=tri[1]+n->mDVertices;
			n->mTriangles[i].mC=tri[2]+n->mDVertices;
		}
	}

	if (n->mNumPVertices>0)
	{
		if (c->GetNext()!=MKID("PVRT"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");	
		}

		for (i=0;i<n->mNumAFrames;i++)
			c->Read(n->mPVertices[i],sizeof(FVector),n->mNumPVertices);
	}

	if (n->mNumChildren>0)
	{
		if (c->GetNext()!=MKID("CHLD"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}

		n->mChildren = new (MEMTYPE_MESH) CMESHPART*[n->mNumChildren];

		for (i=0;i<n->mNumChildren;i++)
		{
			int pnum;
			c->Read(&pnum,sizeof(pnum),1);
			n->mChildren[i]=mesh->GetPart(pnum);
			if (!n->mChildren[i])
			{
				SASSERT(0,"CMeshPart attempted to deserialize a reference to an unavailable child node!");
			}
		}
	}	

	if (n->mParent)
	{
		if (c->GetNext()!=MKID("PRNT"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}
		int pnum;
		c->Read(&pnum,sizeof(pnum),1);
		n->mParent=mesh->GetPart(pnum);
		if (!n->mParent)
		{
			SASSERT(0,"CMeshPart attempted to deserialize a reference to an unavailable parent node!");		
		}
	}
	
	if (n->mNextMeshPartInChain)
	{
		if (c->GetNext()!=MKID("NMIC"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}
		int pnum;
		c->Read(&pnum,sizeof(pnum),1);
		n->mNextMeshPartInChain=mesh->GetPart(pnum);
		if (!n->mNextMeshPartInChain)
		{
			SASSERT(0,"CMeshPart attempted to deserialize a reference to an unavailable next chain node!");	
		}
	}	

	if (c->GetNext()!=MKID("BBOX"))
	{
		SASSERT(0,"CMeshPart deserialize format failure!");
	}	
	n->mBoundingBox=CBoundingBox::Deserialize(c, oldbbox);

	if (n->mNumVFrames>0)
	{
/*		if (c->GetNext()!=MKID("VFRM"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}
			
		Read(n->mVFrames,sizeof(float),n->mNumVFrames);*/

		if (c->GetNext()!=MKID("VHFM"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}
			
		c->Read(n->mVHFrameMap,sizeof(UBYTE),n->mNumVFrames);
	}

	if (n->mNumHFrames>0)
	{
		if (n->mHFrameOrientations)
		{
			ULONG nextid = c->GetNext();

			if (nextid==MKID("HORI"))
			{
				// uncompressed orientations.
				n->mHFrameOrientations = new( MEMTYPE_MESH ) FMatrix[n->mNumHFrames];
				c->Read(n->mHFrameOrientations,sizeof(FMatrix),n->mNumHFrames);

#ifdef USECOMPRESSEDMATRICES
				n->mHFrameOrientationsSmaller = NULL;
#endif
			}
			else if (nextid == MKID("HORC"))
			{
				// compressed orientations.
#ifdef USECOMPRESSEDMATRICES
				n->mHFrameOrientationsSmaller = new( MEMTYPE_MESH ) FSmallerMatrix[n->mNumHFrames];
				c->Read(n->mHFrameOrientationsSmaller,sizeof(FSmallerMatrix),n->mNumHFrames);
				n->mHFrameOrientations = NULL;
#else
				SASSERT(0, "Target does not support compressed matrices!");
#endif
			}
			else
			{
				SASSERT(0,"CMeshPart deserialize format failure!");
			}
		}

		if (n->mHFramePositions)
		{
			if (c->GetNext()!=MKID("HPOS"))
			{
				SASSERT(0,"CMeshPart deserialize format failure!");
			}
			c->Read(n->mHFramePositions,sizeof(FVector),n->mNumHFrames);
		}

		if (n->mHFrameFOVs)
		{
			if (c->GetNext()!=MKID("HFOV"))
			{
				SASSERT(0,"CMeshPart deserialize format failure!");
			}
			c->Read(n->mHFrameFOVs,sizeof(float),n->mNumHFrames);
		}
	}

	if (n->mNumBones>0)
	{
		if (c->GetNext()!=MKID("BONE"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}

		for (i=0;i<n->mNumBones;i++)
		{
			int pnum;
			c->Read(&pnum,sizeof(pnum),1);
			n->mBones[i]=mesh->GetPart(pnum);
		}

		// Disabled because we never serialize these

		if (n->mBoneWeights)
		{
			if (c->GetNext()!=MKID("BONW"))
			{
				SASSERT(0,"CMeshPart deserialize format failure!");
			}

			for (i=0;i<n->mNumPVertices;i++)
			{
				c->Read(n->mBoneWeights[i],sizeof(float),n->mNumBones);
			}
		}

		if (n->mBoneSlots)
		{
			if (c->GetNext()!=MKID("BONS"))
			{
				SASSERT(0,"CMeshPart deserialize format failure!");
			}
			for (i=0;i<n->mNumPVertices;i++)
			{
				c->Read(n->mBoneSlots[i],sizeof(int),BONE_SLOTS);
			}
		}
	}

	if (n->mPolyBucket)
	{
		n->mPolyBucket=CPolyBucket::Deserialize(c);	
	}

/*	
	delete n->mPolyBucket;

	n->mPolyBucket = new( MEMTYPE_MESH ) CPolyBucket;
	if (!n->mPolyBucket)
		return FALSE;  

	n->mPolyBucket->Build(*n);
*/
	if (n->mCachedPosition)
	{
#if TARGET!=PS2
		n->mCachedPosition=new (MEMTYPE_MESHCACHE) FVector[n->mNumCachedFrames];
#else
		if (false) //((n->mNumCachedFrames<0) && (n->mNumCachedFrames>0))
			n->mCachedPosition=new (MEMTYPE_MESHCACHE) FVector[n->mNumCachedFrames];
#endif
		
		if (c->GetNext()!=MKID("CPOS"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}
#if TARGET!=PS2
		c->Read(n->mCachedPosition,sizeof(FVector),n->mNumCachedFrames);
#else
		if (false) //((n->mNumCachedFrames<0) && (n->mNumCachedFrames>0))
			c->Read(n->mCachedPosition,sizeof(FVector),n->mNumCachedFrames);
		else
		{
			n->mCachedPosition=NULL;
			n->mNumCachedFrames=0;
			c->Skip();
		}
#endif

		MEM_MANAGER.TagMem(n->mCachedPosition,sizeof(FVector)*n->mNumCachedFrames,"Meshes/%s/Part %d/Position cache",n->mBaseMesh->mName,n->mPartNumber);
	}

	if (n->mCachedOrientation)
	{
		// These things may or may not be compressed.
		ULONG nextid = c->GetNext();

		if (nextid==MKID("CORI"))
		{
			// uncompressed.
			ocachesize+=sizeof(FMatrix)*n->mNumCachedFrames;
			printf("Mesh %s part %s has %d cached orientation frames - ocache size now %dK\n",n->mBaseMesh->mName,n->mName,n->mNumCachedFrames,ocachesize/1024);

#if TARGET!=PS2
			n->mCachedOrientation= new (MEMTYPE_MESHCACHE) FMatrix[n->mNumCachedFrames];
#else
			if (false)
				n->mCachedOrientation= new (MEMTYPE_MESHCACHE) FMatrix[n->mNumCachedFrames];
#endif 

#if TARGET!=PS2
			c->Read(n->mCachedOrientation,sizeof(FMatrix),n->mNumCachedFrames);
#else
			if (false)
				c->Read(n->mCachedOrientation,sizeof(FMatrix),n->mNumCachedFrames);
			else
			{
				n->mCachedOrientation=NULL;
				n->mNumCachedFrames=0;
				c->Skip();
			}
#endif

			MEM_MANAGER.TagMem(n->mCachedOrientation,sizeof(FMatrix)*n->mNumCachedFrames,"Meshes/%s/Part %d/Orientation cache",n->mBaseMesh->mName,n->mPartNumber);

#ifdef USECOMPRESSEDMATRICES
			n->mCachedOrientationSmaller = NULL;
#endif
		}
		else if (nextid == MKID("CORC"))
		{
#ifdef USECOMPRESSEDMATRICES
			// compressed.
			ocachesize+=sizeof(FSmallerMatrix)*n->mNumCachedFrames;
			printf("Mesh %s part %s has %d cached orientation frames - ocache size now %dK\n",n->mBaseMesh->mName,n->mName,n->mNumCachedFrames,ocachesize/1024);

			n->mCachedOrientationSmaller= new (MEMTYPE_MESHCACHE) FSmallerMatrix[n->mNumCachedFrames];

			c->Read(n->mCachedOrientationSmaller,sizeof(FSmallerMatrix),n->mNumCachedFrames);

			MEM_MANAGER.TagMem(n->mCachedOrientationSmaller,sizeof(FSmallerMatrix)*n->mNumCachedFrames,"Meshes/%s/Part %d/Orientation cache",n->mBaseMesh->mName,n->mPartNumber);

			n->mCachedOrientation = NULL;
#else
			SASSERT(0, "Trying to load compressed data onto wrong platform");
#endif
		}
		else
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}
	}
	else
	{
		// just in case...
		n->mCachedOrientationSmaller = NULL;
	}

	if (n->mReferenceTo)
	{
		if (c->GetNext()!=MKID("REFR"))
		{
			SASSERT(0,"CMeshPart deserialize format failure!");
		}
		int pnum;
		c->Read(&pnum,sizeof(pnum),1);
		n->mReferenceTo=mesh->GetPart(pnum);
		if (!n->mReferenceTo)
		{
			SASSERT(0,"CMeshPart attempted to deserialize a reference to an unavailable reference node!");		
		}
	}

	// Cache information
	
	n->mLastCachedWorldTransCount=99999;
	n->mLastCachedModelTransCount=99999;
	n->mCachedRenderThing=NULL;

	// Restore pointers

	n->mMeshVB=meshvb;

	// MeshVB

	n->mMeshVB->Deserialize(c);
		
	return(n);	
}


//******************************************************************************************
#ifdef RESBUILDER

void CBoundingBox::Serialize(class CChunker *c,class CResourceAccumulator *ra)
{
	c->Start(MKID("BBOX"));
	c->Write(&mOrigin,sizeof(mOrigin),1);
	c->Write(&mAxes,sizeof(mAxes),1);
	c->Write(&mValid,sizeof(mValid),1);
	c->Write(&mRadius,sizeof(mRadius),1);
	c->End();
}
#endif
//******************************************************************************************

CBoundingBox *CBoundingBox::Deserialize(class CChunkReader *c, CBoundingBox *already_allocated_memory)
{
	if (c->GetNext()!=MKID("BBOX"))
	{
		SASSERT(0,"CBoundingBox deserialize format failure!");
	}

	CBoundingBox *n;
	if (already_allocated_memory) n = already_allocated_memory;
	else						  n = new (MEMTYPE_MESH) CBoundingBox;

	c->Read(&n->mOrigin,sizeof(n->mOrigin),1);
	c->Read(&n->mAxes,sizeof(n->mAxes),1);
	c->Read(&n->mValid,sizeof(n->mValid),1);
	c->Read(&n->mRadius,sizeof(n->mRadius),1);
	
	return(n);
}

#endif

//******************************************************************************************

int savedpolys=0;
int totalpolys=0;

void CMeshPart::ReducePolys()
{
	if (mNumPVertices<32)
		return;

	int numremoved=0;
	int killedpolys=0;

	int i,j,k,l;

	float threshold=0.2f;

	if (mNumPVertices>300)
		threshold=0.3f;

	int originalverts=mNumPVertices;

	// Perform reduction
		
	int *edgelist=new(MEMTYPE_MESH) int[mNumPVertices];
	int edgelistsize=0;
	UINT *trilist=new(MEMTYPE_MESH) UINT[mNumTriangles];
	int trilistsize=0;
	
	for (i=0;i<mNumPVertices;i++)
	{
		// Find all the edges we could collapse along

		edgelistsize=0;
		trilistsize=0;

		for (j=0;j<mNumTriangles;j++)
		{
			if ((mTriangles[j].mA->mPVert==i) ||
				(mTriangles[j].mB->mPVert==i) ||
				(mTriangles[j].mC->mPVert==i))
			{
				trilist[trilistsize++]=j; // Add triangle to trilist

				BOOL ain=(mTriangles[j].mA->mPVert==i);
				BOOL bin=(mTriangles[j].mB->mPVert==i);
				BOOL cin=(mTriangles[j].mC->mPVert==i);
				for (k=0;k<edgelistsize;k++)
				{
					if (edgelist[k]==mTriangles[j].mA->mPVert)
						ain=TRUE;
					if (edgelist[k]==mTriangles[j].mB->mPVert)
						bin=TRUE;
					if (edgelist[k]==mTriangles[j].mC->mPVert)
						cin=TRUE;
				}
				if (!ain)
					edgelist[edgelistsize++]=mTriangles[j].mA->mPVert;
				if (!bin)
					edgelist[edgelistsize++]=mTriangles[j].mB->mPVert;
				if (!cin)
					edgelist[edgelistsize++]=mTriangles[j].mC->mPVert;
			}
		}

		BOOL oktocollapse=TRUE;

		// Check if u is on the edge of the model

		for (j=0;j<edgelistsize;j++)
		{
			int numtrisonedge=0;
			for (k=0;k<trilistsize;k++)
			{
				if ((mTriangles[trilist[k]].mA->mPVert==edgelist[j]) ||
					(mTriangles[trilist[k]].mB->mPVert==edgelist[j]) ||
					(mTriangles[trilist[k]].mC->mPVert==edgelist[j]))
				{
					numtrisonedge++;
				}
			}
			if (numtrisonedge<2)
				oktocollapse=FALSE;
		}

		if (mNumPVertices<(originalverts/4))	// Never collapse below a certain point
			oktocollapse=FALSE;

		if (oktocollapse)
		{
			int bestedge=-1;
			float bestcurvature=1.0f;

			for (j=0;j<edgelistsize;j++)
			{
				// Collapsing vert[i] (u) to vert[edgelist[j]] (v)

				int u=i;
				int v=edgelist[j];

				// Calculate curvature

				// Trilist contains all tris adjoining u

				float maxcurvature=0.0f;

				for (k=0;k<trilistsize;k++)
				{
					FVector normala=(mPVertices[0][mTriangles[trilist[k]].mC->mPVert]-mPVertices[0][mTriangles[trilist[k]].mA->mPVert]) ^ (mPVertices[0][mTriangles[trilist[k]].mB->mPVert]-mPVertices[0][mTriangles[trilist[k]].mA->mPVert]);
					normala.Normalise();
					float mincurvature=1.0f;
					int edgefaces=0;
					for (l=0;l<trilistsize;l++)
					{					
						if ((mTriangles[trilist[l]].mA->mPVert==v) ||
							(mTriangles[trilist[l]].mB->mPVert==v) ||
							(mTriangles[trilist[l]].mC->mPVert==v))
						{
							FVector normalb=(mPVertices[0][mTriangles[trilist[l]].mC->mPVert]-mPVertices[0][mTriangles[trilist[l]].mA->mPVert]) ^ (mPVertices[0][mTriangles[trilist[l]].mB->mPVert]-mPVertices[0][mTriangles[trilist[l]].mA->mPVert]);
							normalb.Normalise();
							float curvature=(1-(normala * normalb))/2.0f;
							if (curvature<mincurvature)
								mincurvature=curvature;
							edgefaces++;
						}
					}

					if (edgefaces==1)
						mincurvature=1.0f; // Never collapse edges that are genuinely "edges"

					if (mincurvature>maxcurvature)
						maxcurvature=mincurvature;
				}

				float edgelength=(mPVertices[0][u]-mPVertices[0][v]).Magnitude();

				if (edgelength<1.0f)
					edgelength=1.0f;

				maxcurvature=maxcurvature*edgelength;

				if (maxcurvature<bestcurvature)
				{
					bestedge=j;
					bestcurvature=maxcurvature;
				}
			}

			// Collapse along best edge found

			if ((bestcurvature<threshold) && (bestedge!=-1))
			{
				numremoved++;

				// Remove vertex
					
				for (k=0;k<mNumDVertices;k++)
				{
					if (mDVertices[k].mPVert==i)
					{
						mDVertices[k].mPVert=edgelist[bestedge];
					}
					if (mDVertices[k].mPVert>i)
						mDVertices[k].mPVert--;
				}

				for (k=i+1;k<mNumPVertices;k++)
					mPVertices[0][k-1]=mPVertices[0][k];

				mNumPVertices--;

				// Remove any collapsed polys
	/*
				for (k=0;k<trilistsize;k++)
				{
					if ((mTriangles[trilist[k]].mA->mPVert==mTriangles[trilist[k]].mB->mPVert) ||
						(mTriangles[trilist[k]].mA->mPVert==mTriangles[trilist[k]].mC->mPVert) ||
						(mTriangles[trilist[k]].mB->mPVert==mTriangles[trilist[k]].mC->mPVert))
					{
						killedpolys++;
						for (l=trilist[k]+1;l<mNumTriangles;l++)
						{
							mTriangles[l-1].mA=mTriangles[l].mA;
							mTriangles[l-1].mB=mTriangles[l].mB;
							mTriangles[l-1].mC=mTriangles[l].mC;
						}
						for (l=k+1;l<trilistsize;l++)
						{
							if (trilist[l]>trilist[k])
								trilist[l]--;
						}
						mNumTriangles--;
					}
				}*/

				i--;
			}
		}
	}

	SAFE_DELETE(edgelist);
	SAFE_DELETE(trilist);

	for (k=0;k<mNumTriangles;k++)
	{
		if ((mTriangles[k].mA->mPVert==mTriangles[k].mB->mPVert) ||
			(mTriangles[k].mA->mPVert==mTriangles[k].mC->mPVert) ||
			(mTriangles[k].mB->mPVert==mTriangles[k].mC->mPVert))
		{
			killedpolys++;
			for (l=k+1;l<mNumTriangles;l++)
			{
				mTriangles[l-1].mA=mTriangles[l].mA;
				mTriangles[l-1].mB=mTriangles[l].mB;
				mTriangles[l-1].mC=mTriangles[l].mC;
			}
			mNumTriangles--;
			k--;
		}
	}

	char buf[256];
	sprintf(buf,"Part %s removed %d of %d verts\n",mName,numremoved,mNumDVertices);
	TRACE(buf);

	sprintf(buf,"Removed %d of %d polys\n",killedpolys,mNumTriangles);
	TRACE(buf);

	savedpolys+=killedpolys;
	totalpolys+=mNumTriangles;

	sprintf(buf,"Total polys saved : %d of %d\n",savedpolys,totalpolys);
	TRACE(buf);
}

//******************************************************************************************

CMeshPart *CMeshPart::Clone()
{
#if TARGET == XBOX
	SASSERT(0, "code removed!");
	return NULL;
#else
	CMeshPart *t=new (MEMTYPE_MESH) CMeshPart();

	int i;

	t->mPartNumber=mPartNumber;
	t->mPartType=mPartType;
	t->mRadius=mRadius;
	t->mCueFactor=mCueFactor;

	t->mBaseMesh=mBaseMesh;
	t->mBaseOrientation=mBaseOrientation;
	t->mBasePosition=mBasePosition;
	t->mReferenceTo=mReferenceTo;
	
	t->mBoundingBox->mAxes=mBoundingBox->mAxes;
	t->mBoundingBox->mOrigin=mBoundingBox->mOrigin;
	t->mBoundingBox->mRadius=mBoundingBox->mRadius;
	t->mBoundingBox->mValid=mBoundingBox->mValid;
	t->mCueFactor=mCueFactor;
	t->mIsChained=mIsChained;
	t->mNextMeshPartInChain=mNextMeshPartInChain;
	strcpy(t->mName,mName);
	t->mNumChildren=mNumChildren;
	t->mChildren=new (MEMTYPE_MESH) CMESHPART*[mNumChildren];
	for (i=0;i<mNumChildren;i++)
		t->mChildren[i]=mChildren[i];
	
	t->InitMemory(mNumDVertices,mNumPVertices,mNumTriangles,mNumVFrames,mNumAFrames);

	memcpy(t->mDVertices,mDVertices,sizeof(CMeshDVertex)*mNumDVertices);
	/*memcpy(t->mVFrames,mVFrames,sizeof(float)*mNumVFrames);*/

	for (i=0;i<mNumAFrames;i++)
		memcpy(t->mPVertices[i],mPVertices[i],sizeof(FVector)*mNumPVertices);

	for (i=0;i<mNumTriangles;i++)
	{
		t->mTriangles[i].mA=&t->mDVertices[mTriangles[i].mA-mDVertices];
		t->mTriangles[i].mB=&t->mDVertices[mTriangles[i].mB-mDVertices];
		t->mTriangles[i].mC=&t->mDVertices[mTriangles[i].mC-mDVertices];
	}

	t->mNumHFrames=mNumHFrames;

	if (mNumHFrames>0)
	{
		if (mHFrameOrientations)
		{
			t->mHFrameOrientations = new( MEMTYPE_MESH ) FMatrix[t->mNumHFrames];

			memcpy(t->mHFrameOrientations,mHFrameOrientations,sizeof(FMatrix)*mNumHFrames);
		}

		if (mHFramePositions)
		{
			t->mHFramePositions = new( MEMTYPE_MESH ) FVector[t->mNumHFrames];
			memcpy(t->mHFramePositions,mHFramePositions,sizeof(FVector)*mNumHFrames);
		}

		if (mHFrameFOVs)
		{
			t->mHFrameFOVs = new( MEMTYPE_MESH ) float[t->mNumHFrames];			
			memcpy(t->mHFrameFOVs,mHFrameFOVs,sizeof(float)*mNumHFrames);
		}
	}

	if (mNumVFrames>0)
	{
		t->mVHFrameMap = new( MEMTYPE_MESH ) UBYTE[mNumVFrames];
		memcpy(t->mVHFrameMap,mVHFrameMap,sizeof(UBYTE)*mNumVFrames);
	}

	t->mNumBones=mNumBones;

	if (mBones)
	{
		t->mBones = new( MEMTYPE_MESH ) CMESHPART*[t->mNumBones];

		for (i=0;i<mNumBones;i++)
		{
			t->mBones[i]=mBones[i];
		}

		if (mNumPVertices>0)
		{
			if (mBoneWeights)
			{
				t->mBoneWeights = new( MEMTYPE_MESH ) float*[mNumPVertices];
				memcpy(t->mBoneWeights,mBoneWeights,sizeof(float*)*mNumPVertices);
			}

			if (mBoneSlots)
			{
				t->mBoneSlots = new( MEMTYPE_MESH ) int*[mNumPVertices];
				memcpy(t->mBoneSlots,mBoneSlots,sizeof(int*)*mNumPVertices);
			}

			for (i=0;i<mNumPVertices;i++)
			{
				t->mBoneWeights[i]=new( MEMTYPE_MESH ) float[t->mNumBones];
				t->mBoneSlots[i]=new( MEMTYPE_MESH ) int[BONE_SLOTS];
				memcpy(t->mBoneWeights[i],mBoneWeights[i],sizeof(float)*mNumBones);
				memcpy(t->mBoneSlots[i],mBoneSlots[i],sizeof(int)*BONE_SLOTS);
			}
		}
	}
	else
	{
		t->mBones=NULL;
		t->mBoneWeights=NULL;
		t->mBoneSlots=NULL;
	}

	return(t);
#endif
}

//******************************************************************************************

void CMeshPart::MergeVertices(CMeshPart *from)
{
	SASSERT(mNumAFrames==1,"Cannot merge vertex-animated meshes!");

	char buf[256];
	sprintf(buf,"Merging %s into %s\n",from->mName,mName);
	TRACE(buf);

	int pverts=mNumPVertices+from->mNumPVertices;
	int dverts=mNumDVertices+from->mNumDVertices;
	int tris=mNumTriangles+from->mNumTriangles;
	
	FVector *newpverts = new( MEMTYPE_MESH ) FVector[pverts];
	SASSERT(newpverts,"MergeVertices() - Unable to allocate PVerts");	

	CMeshDVertex *newdverts = new( MEMTYPE_MESH ) CMeshDVertex[dverts];
	SASSERT(newdverts,"MergeVertices() - Unable to allocate DVerts");
		
	CMeshTriangle *newtris = new( MEMTYPE_MESH ) CMeshTriangle[tris];
	SASSERT(newtris,"MergeVertices() - Unable to allocate Tris");

	// Copy original data

	int i;

	for (i=0;i<mNumPVertices;i++)
		newpverts[i]=mPVertices[0][i];	

	for (i=0;i<mNumDVertices;i++)
		newdverts[i]=mDVertices[i];

	for (i=0;i<mNumTriangles;i++)
	{
		newtris[i].mA=&newdverts[(mTriangles[i].mA-mDVertices)];
		newtris[i].mB=&newdverts[(mTriangles[i].mB-mDVertices)];
		newtris[i].mC=&newdverts[(mTriangles[i].mC-mDVertices)];
	}

	// Get transforms

	FMatrix fori,pori;
	FVector fpos,ppos;

	from->GetWorldTransforms(0,0,0.0f,NULL,&fori,&fpos,TRUE,FALSE,TRUE);
	GetWorldTransforms(0,0,0.0f,NULL,&pori,&ppos,TRUE,FALSE,TRUE);
	
	// Merge in extra data

	for (i=0;i<from->mNumPVertices;i++)
	{
		FVector pos=(fori * from->mPVertices[0][i])+fpos; // Worldspace
		pos=pori.Inverse() * (pos-ppos);
		newpverts[i+mNumPVertices]=pos;
	}

	for (i=0;i<from->mNumDVertices;i++)
	{
		newdverts[i+mNumDVertices]=from->mDVertices[i];
		newdverts[i+mNumDVertices].mNormal=pori.Inverse() * (fori * newdverts[i+mNumDVertices].mNormal);
		newdverts[i+mNumDVertices].mPVert+=mNumPVertices;
	}

	for (i=0;i<from->mNumTriangles;i++)
	{		
		newtris[i+mNumTriangles].mA=newdverts+mNumDVertices+(from->mTriangles[i].mA-from->mDVertices);
		newtris[i+mNumTriangles].mB=newdverts+mNumDVertices+(from->mTriangles[i].mB-from->mDVertices);
		newtris[i+mNumTriangles].mC=newdverts+mNumDVertices+(from->mTriangles[i].mC-from->mDVertices);
	}

	// Replace current data with merged data

	mNumPVertices=pverts;
	mNumDVertices=dverts;
	mNumTriangles=tris;

	mDVertices=newdverts;
	mPVertices[0]=newpverts;
	mTriangles=newtris;
}
