#include	"Common.h"

#ifdef _DIRECTX

#include	"DX.h"
#include	"vertexshader.h"
#include	"DXMeshVB.h"
#include	"Texture.h"
#include	"Mesh.h"
#include	"Profile.h"
#include	"Engine.h"
#include	"vbuftexture.h"
#include	"console.h"
#include	"meshrenderer.h"
#include	"renderablething.h"
#include	"state.h"
#include	"renderqueue.h"
#include	"renderinfo.h"
#include	"shaders\fragments\constants.h"
#include	"atmospherics.h"
#include	"chunker.h"

// Replace this with a link to the debug version for debugging
#pragma comment (lib,"NVTriStrip\\Release\\NVTriStrip.lib")

#include	"NVTriStrip\NVTriStrip.h"

int		CDXMeshVB::mTotalTriangles=0;
int		CDXMeshVB::mTotalStrips=0;
int		CDXMeshVB::mMaxStripLength=0;

//********************************************************************
CDXMeshVB::CDXMeshVB()
{
	mMeshPart = NULL;
	mNumTextures = 0;
	mCurrentShader = NULL;

	SINT	c0;
	for	(c0 = 0; c0 < MAX_MESH_MAT_PARTS; c0 ++)
	{
		mVBPart[c0]=NULL;
	}

	LT.AddDeviceObject(this);
}
//********************************************************************
CDXMeshVB::~CDXMeshVB()
{
	DeleteDeviceObjects();
	SINT	c0;
	for	(c0 = 0; c0 < MAX_MESH_MAT_PARTS; c0 ++)
	{
		SAFE_DELETE(mVBPart[0]);
	}
	LT.RemoveDeviceObject(this);
}

// --------------------------------------------------------------------------
// Build a normal (ie non-shader) vertex buffer
// --------------------------------------------------------------------------

HRESULT CDXMeshVB::BuildNormalVB()
{
	// OK - need to trawl through everything and split things up by material
	mNumTextures = 0; 

	if ((mMeshPart->mNumDVertices<1) || (mMeshPart->mNumTriangles<1))
		return(S_OK); // Obviously not worth doing anything with

//	CONSOLE.Status("Building normal VB"); 

	SINT			nverts		= mMeshPart->mNumDVertices;
	SINT			ntris		= mMeshPart->mNumTriangles; 
	CFVF_GTEX		*verts		= new( MEMTYPE_MESHVB ) CFVF_GTEX[nverts];
	UWORD			*indices	= new( MEMTYPE_MESHVB ) UWORD[nverts * MAX_MESH_MAT_PARTS * 6];

	SINT			c0, c1;  

/*	char buffer[256];
	if (strlen(mMeshPart->mBaseMesh->mName)<254)
	{
		sprintf(buffer,"Mesh %s - %d tris\n",mMeshPart->mBaseMesh->mName,ntris);
		OutputDebugString(buffer);
	}*/

//	CONSOLE.StatusProgress("Building normal VB","[Adding vertices]");

	// OK - add all the vertices first
	for (c0 = 0; c0 < nverts; c0 ++)
	{
		CMeshDVertex	*vert = &mMeshPart->mDVertices[c0];

		// Scan through for textures

		SINT	tex = 0;
		
		for (c1 = 0; c1 < mNumTextures; c1 ++)
		{
			BOOL found=TRUE;
			for (int j=0;j<TEXTURE_TYPES;j++)
			{
				if ((vert->mTexture[j]!=-1) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mMaxBlend>0.0f) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mTexture!=NULL))
				{
					// Vertex has some texture on this layer
					if (mVBPart[c1]->mTexture[j]!=&mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]])
						found=FALSE;
				}
				else
				{
					// Vertex has no texture on this layer
					if (mVBPart[c1]->mTexture[j])
						found=FALSE;
				}
			}

			if (found)
				break;
		}
		
		if (c1 == mNumTextures)
		{
			// need to create a new texture
			SASSERT(mNumTextures < MAX_MESH_MAT_PARTS,"Too many textures on one mesh part!");
			
			mVBPart[mNumTextures]=new (MEMTYPE_MESHVB) SDXMeshMatPart;
			
			mVBPart[mNumTextures]->mNumVertices = 0;
			mVBPart[mNumTextures]->mNumIndices = 0;
			mVBPart[mNumTextures]->mNumPrimitives = 0;
			for (int j=0;j<TEXTURE_TYPES;j++)
			{
				if ((vert->mTexture[j]!=-1) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mMaxBlend>0.0f) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mTexture!=NULL))
					mVBPart[mNumTextures]->mTexture[j]=&mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]];
				else
					mVBPart[mNumTextures]->mTexture[j]=NULL;
			}
			mNumTextures++;
		}

		// ...and add the vertex to our pseudo-vb

		CFVF_GTEX	*jclvert = &verts[c0];
		
		jclvert->mPos		= *(mMeshPart->mPVertices[0] + vert->mPVert);
		jclvert->mDiffuse	= vert->mDiffuse;

		jclvert->mU			= vert->mU;
		jclvert->mV			= vert->mV;
 		jclvert->mNormal	= vert->mNormal;
	}

	// Now stripify the model

//	CONSOLE.StatusProgress("Building normal VB","[Stripifying]");
	
/*	sprintf(buffer,"%d textures found\n",mNumTextures);
	OutputDebugString(buffer);	*/

	int i;

	// Setup the stripification library

	SetCacheSize(CACHESIZE_GEFORCE3);
	SetStitchStrips(true);
	SetMinStripSize(0);
	SetListsOnly(false);

	for (i=0;i<mNumTextures;i++)
	{
		mVBPart[i]->mNumIndices=0;		
		mVBPart[i]->mNumPrimitives=0;
	}

	UWORD	*stripindexlist[MAX_MESH_MAT_PARTS];
	int		stripindices[MAX_MESH_MAT_PARTS];

	for (i=0;i<mNumTextures;i++)
	{
		stripindexlist[i]=new( MEMTYPE_MESHVB ) UWORD[ntris * 3];
		stripindices[i]=0;
	}

	for (c0 = 0; c0 < ntris; c0 ++)
	{
		CMeshTriangle	*tri = &mMeshPart->mTriangles[c0];

		// Find a suitable texture

		for (c1 = 0; c1 < mNumTextures; c1 ++)
		{
			BOOL found=TRUE;
			for (int j=0;j<TEXTURE_TYPES;j++)
			{
				ASSERT(tri->mA->mTexture[j]==tri->mB->mTexture[j]);
				ASSERT(tri->mB->mTexture[j]==tri->mC->mTexture[j]);
				
				if ((tri->mA->mTexture[j]!=-1) && (mMeshPart->mBaseMesh->mTextures[tri->mA->mTexture[j]].mMaxBlend>0.0f) && (mMeshPart->mBaseMesh->mTextures[tri->mA->mTexture[j]].mTexture!=NULL))
				{
					// Vertex has some texture on this layer
					if (mVBPart[c1]->mTexture[j]!=&mMeshPart->mBaseMesh->mTextures[tri->mA->mTexture[j]])
						found=FALSE;
				}
				else
				{
					// Vertex has no texture on this layer
					if (mVBPart[c1]->mTexture[j])
						found=FALSE;
				}
			}
			
			if (found)
				break;
		}

		ASSERT(c1 != mNumTextures); // shouldn't happen

		stripindexlist[c1][stripindices[c1]]=tri->mA-mMeshPart->mDVertices;
		stripindices[c1]++;
		stripindexlist[c1][stripindices[c1]]=tri->mB-mMeshPart->mDVertices;
		stripindices[c1]++;
		stripindexlist[c1][stripindices[c1]]=tri->mC-mMeshPart->mDVertices;
		stripindices[c1]++;
		
/*		UWORD	*index = &indices[(c1 * nverts * 6) + mVBPart[c1]->mNumIndices];
		index[0]=tri->mA - mMeshPart->mDVertices;
		index[1]=tri->mB - mMeshPart->mDVertices;
		index[2]=tri->mC - mMeshPart->mDVertices;
		mVBPart[c1]->mNumIndices+=3;
		mVBPart[c1]->mNumPrimitives++;*/
		
		mVBPart[c1]->mNumIndices++; // Keep a triangle count for the moment
	}

/*
	for (i=0;i<mNumTextures;i++)
	{
		sprintf(buffer,"Layer %d - rendermode %d - texture %s\n",i,mVBPart[i]->mTextureRenderMode,mVBPart[i]->mTexture->mTexture->GetName());
		OutputDebugString(buffer);
		sprintf(buffer,"Got %d triangles\n",mVBPart[i]->mNumIndices);
		OutputDebugString(buffer);
	}
*/
	// Now perform the stripification

	int totalindices=0;	

	for (i=0;i<mNumTextures;i++)
	{
		// Generate strips

		UWORD numprims;
		PrimitiveGroup *prim;

		GenerateStrips(stripindexlist[i],stripindices[i],&prim,&numprims);

		ASSERT(numprims==1);
		ASSERT(prim->type==PT_STRIP);

		mVBPart[i]->mNumIndices=0;

		for (int j=0;j<((int) prim->numIndices);j++)
		{
			UWORD	*index = &indices[(i * nverts * 6) + mVBPart[i]->mNumIndices];
			
			index[0]=prim->indices[j];

/*			ASSERT(index[0]>=0);
			ASSERT(index[0]<nverts);*/
			
			mVBPart[i]->mNumIndices += 1;
		}

		mVBPart[i]->mNumPrimitives=mVBPart[i]->mNumIndices-2;

		delete [] prim;
	}
/*
	for (i=0;i<mNumTextures;i++)
	{
		sprintf(buffer,"Layer %d - rendermode %d - texture %s\n",i,mVBPart[i]->mTextureRenderMode,mVBPart[i]->mTexture->mTexture->GetName());
		OutputDebugString(buffer);
		sprintf(buffer,"Stripping got %d triangles\n",mVBPart[i]->mNumPrimitives);
		OutputDebugString(buffer);
	}
*/
	for (i=0;i<mNumTextures;i++)
	{
		delete [] stripindexlist[i];
	}
	
	// OK - now create the buffers

//	CONSOLE.StatusProgress("Building normal VB","[Building buffers]");

	BOOL	all_ok = TRUE;

	// Just create one VB for the whole mesh

	if (mNumTextures>0)
	{
		mVBPart[0]->mNumVertices=nverts;
		mVBPart[0]->mVBSize=mVBPart[0]->mNumVertices * sizeof(CFVF_GTEX);
		if(!(FAILED( LT.D3D_CreateVertexBuffer(mVBPart[0]->mVBSize,
			D3DUSAGE_WRITEONLY, FVF_GTEX_FLAGS,
			D3DPOOL_MANAGED, &mVBPart[0]->mVBuff, &mVBPart[0]->mpVBData))))
		{
			PROFILE_ALLOCATE(SVBMainMemory,mVBPart[0]->mVBSize);
			PROFILE_ALLOCATE(SVBVideoMemory,mVBPart[0]->mVBSize);

			MEM_MANAGER.TagMem(this,mVBPart[0]->mVBSize,"Meshes/%s/Part %d/Vertex buffer",mMeshPart->mBaseMesh->mName,mMeshPart->mPartNumber);
						
			BYTE	*data;
			
			if (!(FAILED(mVBPart[0]->mVBuff->Lock(0, 0, &data, 0))))
			{
				memcpy(data, &verts[0],  nverts * sizeof(CFVF_GTEX));
				
				mVBPart[0]->mVBuff->Unlock();
			}
			else
				all_ok = FALSE;
		}
		else
		{
			ASSERT(FALSE);
			all_ok = FALSE;
		}

		// "Copy" this VB into all the other VBParts

		for (c0=1;c0<mNumTextures;c0++)
		{
			mVBPart[c0]->mVBSize=mVBPart[0]->mVBSize;
			mVBPart[c0]->mVBuff=mVBPart[0]->mVBuff;
			mVBPart[c0]->mNumVertices=mVBPart[0]->mNumVertices;
		}
	}

	for	(c0 = 0; c0 < mNumTextures; c0 ++)
	{
		mVBPart[c0]->mActive=true; // VBParts are always active by default
		// Index buffer
		if (mVBPart[c0]->mActive)
		{
			mVBPart[c0]->mIBSize=mVBPart[c0]->mNumIndices * 2;
			if(!(FAILED( LT.D3D_CreateIndexBuffer( mVBPart[c0]->mIBSize,
											   D3DUSAGE_WRITEONLY, D3DFMT_INDEX16 ,
											   D3DPOOL_MANAGED, &mVBPart[c0]->mIBuff))))
			{
				PROFILE_ALLOCATE(SIBMainMemory,mVBPart[c0]->mIBSize);
				PROFILE_ALLOCATE(SIBVideoMemory,mVBPart[c0]->mIBSize);
								
				MEM_MANAGER.TagMem(this,mVBPart[c0]->mIBSize,"Meshes/%s/Part %d/VBPart %d/Index buffer",mMeshPart->mBaseMesh->mName,mMeshPart->mPartNumber,c0);

				BYTE	*data;

				if (!(FAILED(mVBPart[c0]->mIBuff->Lock(0, 0, &data, 0))))
				{
					memcpy(data, &indices[c0 * nverts * 6],  mVBPart[c0]->mNumIndices * 2);

					mVBPart[c0]->mIBuff->Unlock();
				}
				else
					all_ok = FALSE;
			}
			else
				all_ok = FALSE;
		}
	}

	mVertexBufferChunkSize=sizeof(CFVF_GTEX);
	mFVFFlags=FVF_GTEX_FLAGS;
	mPrimitiveType = FromD3DPrimType(D3DPT_TRIANGLESTRIP);
	
	delete [] indices;
	delete [] verts;

//	CONSOLE.StatusDone("Building normal VB",all_ok);
	
//	CONSOLE.Print("Indices before stripification %d, after %d\n",ntris*3,totalindices);

/*	if (strlen(mMeshPart->mBaseMesh->mName)<254)
	{
		sprintf(buffer,"Mesh %s done!\n",mMeshPart->mBaseMesh->mName);
		OutputDebugString(buffer);
	}
*/
	return	all_ok ? S_OK : E_FAIL;
}

// --------------------------------------------------------------------------
// Build a vertex buffer for skeletal models 
//
// These are basically the same as the normal vertex buffers, except they
// have added vertex/bone weight information
//
// The index buffer is exactly the same as normal
// --------------------------------------------------------------------------

HRESULT CDXMeshVB::BuildSkeletalVB()
{
	// OK - need to trawl through everything and split things up by material
	mNumTextures = 0;

	CONSOLE.Status("Building skeletal VB");
	
	SINT			nverts		= mMeshPart->mNumDVertices;
	SINT			ntris		= mMeshPart->mNumTriangles; 

	CSKELETAL_VB	*verts		= new( MEMTYPE_MESHVB ) CSKELETAL_VB[nverts * MAX_MESH_MAT_PARTS];
	UWORD			*indices	= new( MEMTYPE_MESHVB ) UWORD[nverts * MAX_MESH_MAT_PARTS * 3];
	UWORD			*vert_inds	= new( MEMTYPE_MESHVB ) UWORD[nverts];

	SINT			c0, c1;

	CVertexShader::UpdateEmulationState();

//	CONSOLE.StatusProgress("Building skeletal VB","[Adding vertices]");		

	// OK - add all the vertices first
	for (c0 = 0; c0 < nverts; c0 ++)
	{
		CMeshDVertex	*vert = &mMeshPart->mDVertices[c0];
		
		// Scan through for textures
		
		SINT	tex = 0;
		
		for (c1 = 0; c1 < mNumTextures; c1 ++)
		{
			BOOL found=TRUE;
			for (int j=0;j<TEXTURE_TYPES;j++)
			{
				if ((vert->mTexture[j]!=-1) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mMaxBlend>0.0f) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mTexture!=NULL))
				{
					// Vertex has some texture on this layer
					if (mVBPart[c1]->mTexture[j]!=&mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]])
						found=FALSE;
				}
				else
				{
					// Vertex has no texture on this layer
					if (mVBPart[c1]->mTexture[j])
						found=FALSE;
				}
			}
			
			if (found)
				break;
		}
		
		if (c1 == mNumTextures)
		{
			// need to create a new texture
			SASSERT(mNumTextures < MAX_MESH_MAT_PARTS,"Too many textures on one mesh part!");
			
			mVBPart[mNumTextures]=new (MEMTYPE_MESHVB) SDXMeshMatPart;
			
			mVBPart[mNumTextures]->mNumVertices = 0;
			mVBPart[mNumTextures]->mNumIndices = 0;
			mVBPart[mNumTextures]->mNumPrimitives = 0;
			for (int j=0;j<TEXTURE_TYPES;j++)
			{
				if ((vert->mTexture[j]!=-1) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mMaxBlend>0.0f) && (mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]].mTexture!=NULL))
					mVBPart[mNumTextures]->mTexture[j]=&mMeshPart->mBaseMesh->mTextures[vert->mTexture[j]];
				else
					mVBPart[mNumTextures]->mTexture[j]=NULL;
			}
			mNumTextures++;
		}
		
		// ...and add the vertex to our pseudo-vb
		
		CSKELETAL_VB	*jclvert = &verts[c0];
		
		jclvert->mPos		= *(mMeshPart->mPVertices[0] + vert->mPVert);
		jclvert->mDiffuse	= vert->mDiffuse;
		
		jclvert->mU			= vert->mU;
		jclvert->mV			= vert->mV;
		jclvert->mNormal	= vert->mNormal;

		// Note : The slot numbers get multiplied by 4 here as they are used as an
		//        offset into a set of matrices in the shader.
		
		for (int i=0;i<BONE_SLOTS;i++)
		{
			jclvert->mBoneSlot[i]=(float) mMeshPart->mBoneSlots[vert->mPVert][i]*SKELETAL_SHADER_MATRIX_ROWS;
		}		
	}

	// Now stripify the model

//	CONSOLE.StatusProgress("Building skeletal VB","[Stripifying]");
	
/*	sprintf(buffer,"%d textures found\n",mNumTextures);
	OutputDebugString(buffer);	*/

	int i;

	// Setup the stripification library

	SetCacheSize(CACHESIZE_GEFORCE3);
	SetStitchStrips(true);
	SetMinStripSize(0);
	SetListsOnly(false);

	for (i=0;i<mNumTextures;i++)
	{
		mVBPart[i]->mNumIndices=0;		
		mVBPart[i]->mNumPrimitives=0;
	}

	UWORD	*stripindexlist[MAX_MESH_MAT_PARTS];
	int		stripindices[MAX_MESH_MAT_PARTS];

	for (i=0;i<mNumTextures;i++)
	{
		stripindexlist[i]=new( MEMTYPE_MESHVB ) UWORD[ntris * 3];
		stripindices[i]=0;
	}

	for (c0 = 0; c0 < ntris; c0 ++)
	{
		CMeshTriangle	*tri = &mMeshPart->mTriangles[c0];

		// Find a suitable texture

		for (c1 = 0; c1 < mNumTextures; c1 ++)
		{
			BOOL found=TRUE;
			for (int j=0;j<TEXTURE_TYPES;j++)
			{
				ASSERT(tri->mA->mTexture[j]==tri->mB->mTexture[j]);
				ASSERT(tri->mB->mTexture[j]==tri->mC->mTexture[j]);
				
				if ((tri->mA->mTexture[j]!=-1) && (mMeshPart->mBaseMesh->mTextures[tri->mA->mTexture[j]].mMaxBlend>0.0f) && (mMeshPart->mBaseMesh->mTextures[tri->mA->mTexture[j]].mTexture!=NULL))
				{
					// Vertex has some texture on this layer
					if (mVBPart[c1]->mTexture[j]!=&mMeshPart->mBaseMesh->mTextures[tri->mA->mTexture[j]])
						found=FALSE;
				}
				else
				{
					// Vertex has no texture on this layer
					if (mVBPart[c1]->mTexture[j])
						found=FALSE;
				}
			}
			
			if (found)
				break;
		}

		ASSERT(c1 != mNumTextures); // shouldn't happen

		stripindexlist[c1][stripindices[c1]]=tri->mA-mMeshPart->mDVertices;
		stripindices[c1]++;
		stripindexlist[c1][stripindices[c1]]=tri->mB-mMeshPart->mDVertices;
		stripindices[c1]++;
		stripindexlist[c1][stripindices[c1]]=tri->mC-mMeshPart->mDVertices;
		stripindices[c1]++;
		
/*		UWORD	*index = &indices[(c1 * nverts * 6) + mVBPart[c1]->mNumIndices];
		index[0]=tri->mA - mMeshPart->mDVertices;
		index[1]=tri->mB - mMeshPart->mDVertices;
		index[2]=tri->mC - mMeshPart->mDVertices;
		mVBPart[c1]->mNumIndices+=3;
		mVBPart[c1]->mNumPrimitives++;*/
		
		mVBPart[c1]->mNumIndices++; // Keep a triangle count for the moment
	}

/*
	for (i=0;i<mNumTextures;i++)
	{
		sprintf(buffer,"Layer %d - rendermode %d - texture %s\n",i,mVBPart[i]->mTextureRenderMode,mVBPart[i]->mTexture->mTexture->GetName());
		OutputDebugString(buffer);
		sprintf(buffer,"Got %d triangles\n",mVBPart[i]->mNumIndices);
		OutputDebugString(buffer);
	}
*/
	// Now perform the stripification

	int totalindices=0;	

	for (i=0;i<mNumTextures;i++)
	{
		// Generate strips

		UWORD numprims;
		PrimitiveGroup *prim;

		GenerateStrips(stripindexlist[i],stripindices[i],&prim,&numprims);

		ASSERT(numprims==1);
		ASSERT(prim->type==PT_STRIP);

		mVBPart[i]->mNumIndices=0;

		for (int j=0;j<((int) prim->numIndices);j++)
		{
			UWORD	*index = &indices[(i * nverts * 6) + mVBPart[i]->mNumIndices];
			
			index[0]=prim->indices[j];

/*			ASSERT(index[0]>=0);
			ASSERT(index[0]<nverts);*/
			
			mVBPart[i]->mNumIndices += 1;
		}

		mVBPart[i]->mNumPrimitives=mVBPart[i]->mNumIndices-2;

		delete [] prim;
	}
/*
	for (i=0;i<mNumTextures;i++)
	{
		sprintf(buffer,"Layer %d - rendermode %d - texture %s\n",i,mVBPart[i]->mTextureRenderMode,mVBPart[i]->mTexture->mTexture->GetName());
		OutputDebugString(buffer);
		sprintf(buffer,"Stripping got %d triangles\n",mVBPart[i]->mNumPrimitives);
		OutputDebugString(buffer);
	}
*/
	for (i=0;i<mNumTextures;i++)
	{
		delete [] stripindexlist[i];
	}

	// OK - now create the buffers
	
	//	CONSOLE.StatusProgress("Building normal VB","[Building buffers]");
	
	BOOL	all_ok = TRUE;
	
	// Just create one VB for the whole mesh
	
	if (mNumTextures>0)
	{
		mVBPart[0]->mNumVertices=nverts;
		mVBPart[0]->mVBSize=mVBPart[0]->mNumVertices * sizeof(CSKELETAL_VB);

		DWORD flags=D3DUSAGE_WRITEONLY;
		D3DPOOL pool=D3DPOOL_MANAGED;
		
		if (CVertexShader::VertexShadersEmulated)
		{
			flags|=D3DUSAGE_SOFTWAREPROCESSING;
			pool=D3DPOOL_SYSTEMMEM;
		}

		if(!(FAILED( LT.D3D_CreateVertexBuffer(mVBPart[0]->mVBSize,
			flags, 0,
			pool, &mVBPart[0]->mVBuff, &mVBPart[0]->mpVBData))))
		{
			PROFILE_ALLOCATE(SVBMainMemory,mVBPart[0]->mVBSize);
			PROFILE_ALLOCATE(SVBVideoMemory,mVBPart[0]->mVBSize);
			
			MEM_MANAGER.TagMem(this,mVBPart[0]->mVBSize,"Meshes/%s/Part %d/Skeletal vertex buffer",mMeshPart->mBaseMesh->mName,mMeshPart->mPartNumber);
			
			BYTE	*data;
			
			if (!(FAILED(mVBPart[0]->mVBuff->Lock(0, 0, &data, 0))))
			{
				memcpy(data, &verts[0],  nverts * sizeof(CSKELETAL_VB));
				
				mVBPart[0]->mVBuff->Unlock();
			}
			else
				all_ok = FALSE;
		}
		else
		{
			ASSERT(FALSE);
			all_ok = FALSE;
		}
		
		// "Copy" this VB into all the other VBParts
		
		for (c0=1;c0<mNumTextures;c0++)
		{
			mVBPart[c0]->mVBSize=mVBPart[0]->mVBSize;
			mVBPart[c0]->mVBuff=mVBPart[0]->mVBuff;
			mVBPart[c0]->mNumVertices=mVBPart[0]->mNumVertices;
		}
	}
	
	for	(c0 = 0; c0 < mNumTextures; c0 ++)
	{
		mVBPart[c0]->mActive=true; // VBParts are always active by default
		// Index buffer
		if (mVBPart[c0]->mActive)
		{
			DWORD flags=D3DUSAGE_WRITEONLY;
			D3DPOOL pool=D3DPOOL_MANAGED;
			
			if (CVertexShader::VertexShadersEmulated)
			{
				flags|=D3DUSAGE_SOFTWAREPROCESSING;
				pool=D3DPOOL_SYSTEMMEM;
			}

			mVBPart[c0]->mIBSize=mVBPart[c0]->mNumIndices * 2;
			if(!(FAILED( LT.D3D_CreateIndexBuffer( mVBPart[c0]->mIBSize,
				flags, D3DFMT_INDEX16 ,
				pool, &mVBPart[c0]->mIBuff))))
			{
				PROFILE_ALLOCATE(SIBMainMemory,mVBPart[c0]->mIBSize);
				PROFILE_ALLOCATE(SIBVideoMemory,mVBPart[c0]->mIBSize);
				
				MEM_MANAGER.TagMem(this,mVBPart[c0]->mIBSize,"Meshes/%s/Part %d/VBPart %d/Index buffer",mMeshPart->mBaseMesh->mName,mMeshPart->mPartNumber,c0);
				
				BYTE	*data;
				
				if (!(FAILED(mVBPart[c0]->mIBuff->Lock(0, 0, &data, 0))))
				{
					memcpy(data, &indices[c0 * nverts * 6],  mVBPart[c0]->mNumIndices * 2);
					
					mVBPart[c0]->mIBuff->Unlock();
				}
				else
					all_ok = FALSE;
			}
			else
				all_ok = FALSE;
		}
	}
	
	mVertexBufferChunkSize=sizeof(CSKELETAL_VB);
	mFVFFlags=0; // Cannot use FVF
	mPrimitiveType = FromD3DPrimType(D3DPT_TRIANGLESTRIP);
	
	delete [] indices;
	delete [] verts;

	// Bind the skeletal animation shader to this vbuffer
/*
	DWORD declaration[]=
	{	D3DVSD_STREAM(0),
		D3DVSD_REG(0, D3DVSDT_FLOAT3),		// Position
		D3DVSD_REG(1, D3DVSDT_D3DCOLOR),	// Diffuse
		D3DVSD_REG(2, D3DVSDT_FLOAT2),		// Texture co-ords
		D3DVSD_REG(3, D3DVSDT_FLOAT3),		// Normal
		D3DVSD_REG(4, D3DVSDT_FLOAT3),		// Indices 1-3
		D3DVSD_END()
	};
*/
//	mCurrentShader=CVertexShader::GetShader("SkeletalShader",declaration,sizeof(declaration));

	CONSOLE.StatusDone("Building skeletal VB", all_ok ? true : false);	

	return	all_ok ? S_OK : E_FAIL;
}

//********************************************************************
HRESULT CDXMeshVB::InitDeviceObjects()
{
	if(!LT.D3DActive()) return S_OK; // we'll initialise later thanks...

	if (!mMeshPart)
	{
		mNumTextures = 0;
		return S_OK;
	}

	HRESULT result=S_OK;

	if (mMeshPart->mPartType==MESHPART_TYPE_SKELETAL)
	{
		if (CVertexShader::VertexShadersEnabled)
		{
			result=BuildSkeletalVB();
		}
		else
		{
			// Don't build any VBuffer
		}
	}
	else
		result=BuildNormalVB();

	return(result);
}

//********************************************************************
HRESULT CDXMeshVB::DeleteDeviceObjects()
{
	SINT	c0;

	// Only VBPart 0 has a "real" vertex buffer, which is shared with
	// all the other parts
		
	if (mVBPart[0])
	{
		if (mVBPart[0]->mVBuff)
		{
			PROFILE_FREE(SVBMainMemory,mVBPart[0]->mVBSize);
			LT.D3D_ReleaseVertexBuffer(mVBPart[0]->mVBuff, mVBPart[0]->mpVBData);
		}
	}
	
	for (c0 = 0; c0 < mNumTextures; c0 ++)
	{		
		if (mVBPart[c0])
		{
			if (mVBPart[c0]->mIBuff)
			{
				SAFE_RELEASE(mVBPart[c0]->mIBuff);
				PROFILE_FREE(SVBMainMemory,mVBPart[c0]->mIBSize);
			}
		}
	}

	for (c0 = 0; c0 < mNumTextures; c0 ++)
		SAFE_DELETE(mVBPart[c0]);

	mNumTextures = 0;

	MEM_MANAGER.UnTagMem(this);

	SAFE_RELEASE(mCurrentShader);

	return	S_OK;
}

// Minimalist render function for fast-path stuff *USE WITH CAUTION!*

void	CDXMeshVB::CoreRender(int part, int texture)
{
	RENDERINFO.SetFVF(mFVFFlags);
	mVBPart[part]->mTexture[texture]->mTexture->SelectTexture();

	LT.D3D_SetIndices(mVBPart[part]->mIBuff);
	LT.D3D_SetStreamSource(0, mVBPart[part]->mVBuff,mVertexBufferChunkSize);
	
	LT.D3D_DrawIndexedPrimitive(ToD3DPrimType(mPrimitiveType),
								0, mVBPart[part]->mNumVertices,
								0, mVBPart[part]->mNumPrimitives);
}

//********************************************************************
void	CDXMeshVB::Render(IRenderableThing *t,SINT flags,FMatrix *ori,FVector *pos)
{
/*	static float foo = -1.f;
	if (ENGINE.GetRenderTime() == foo)
		return;
	foo = ENGINE.GetRenderTime();
	Sleep(300);*/
	
	{
		if (mNumTextures<1)	// Nothing to render?
			return;

		PROFILE_FN(RDMeshVB);

		SINT	c0;

		// Figure out our animation frame
		// (for texture animation only - mesh is static!)

		int vf1=0;
		float	vframe=0;
		if (t)
		{
			int animind = t->GetRealAnimIndex();
			if (animind>=0)
			{
				vframe=t->GetRenderFrame();
				CAnimMode*	am = &mMeshPart->mBaseMesh->mAnimModes[animind];
				vframe*=am->mFrameScaler;
				vframe+=am->mStart;
				vf1=((int) floor(vframe)) % mMeshPart->mNumVFrames;
			}
		}		

		if ((mCurrentShader!=NULL) || (ENGINE.mDefaultShader!=NULL))
		{
			if (CVertexShader::VertexShadersEmulated)
			{
				// Switch to sofware processing
				LT.ForceRS(D3DRS_SOFTWAREVERTEXPROCESSING,true);
			}
			if (mCurrentShader)
				mCurrentShader->SetupShader();
			else if (ENGINE.mDefaultShader)
				ENGINE.mDefaultShader->SetupShader();
		}

		if (mCurrentShader!=NULL)
			LT.D3D_SetVertexShader(mCurrentShader->GetHandle());
		else
		{
			if (ENGINE.mDefaultShader)
				LT.D3D_SetVertexShader(ENGINE.mDefaultShader->GetHandle());
			else
			{
				RENDERINFO.SetFVF(mFVFFlags);
			}
		}
		
		if ((!mCurrentShader) && (!ENGINE.mDefaultShader))
		{
			RENDERINFO.Apply(true);
		}
		
		// Get the base render alpha		

		int basealpha=CMESHRENDERER::GetRenderAlpha();

		int texturestouse=mNumTextures;
		
		for (c0 = 0; c0 < texturestouse; c0 ++)
		{
			int layerstorender=TEXTURE_TYPES;

			if ((flags & RF_SNOWLAYER)==RF_SNOWLAYER)
			{
				layerstorender=1;
			}

			if (CMeshRenderer::GetRenderMode()==R_NoTexture)
			{
				layerstorender=1;
			}
			
			if (CMeshRenderer::GetRenderMode()==R_Shadow)
			{
				layerstorender=1;
			}

			if (CMESHRENDERER::GetSurfaceLOD()==0)
			{
				// Early out
				layerstorender=1;
			}

			if ((flags & RF_CLOAKED)!=0)
				layerstorender=0;

			if (!mVBPart[c0]->mActive)
				layerstorender=0;

			LT.D3D_SetIndices(mVBPart[c0]->mIBuff);
			LT.D3D_SetStreamSource(0, mVBPart[c0]->mVBuff,mVertexBufferChunkSize);

			for (int i=0;i<layerstorender;i++)
			{
				if (mVBPart[c0]->mTexture[i])
				{
					if (CMeshRenderer::GetRenderMode()==R_NoTexture)
					{
						LT.D3D_DrawIndexedPrimitive(ToD3DPrimType(mPrimitiveType),
							0, mVBPart[c0]->mNumVertices,
							0, mVBPart[c0]->mNumPrimitives);
					}
					else
					{
						int alpha=(int) (((float) basealpha)*mVBPart[c0]->mTexture[i]->mTextureBlend[(vf1 % mVBPart[c0]->mTexture[i]->mFrames)]);

						if (i!=TEXTURE_TYPE_DIFFUSE)
							alpha=(alpha*CMESHRENDERER::GetSurfaceLOD())/100;

						if (alpha>10)
						{
							CMESHRENDERER::SetRenderAlpha(alpha);

							if (TARGET == PC && (flags & RF_SNOWLAYER)==RF_SNOWLAYER)
							{
								CAtmospherics::GetSnowLayerTexture()->mRenderMode=TEXTURE_TYPE_SNOWLAYER;
								CAtmospherics::GetSnowLayerTexture()->SetupRenderMode();
								CAtmospherics::GetSnowLayerTexture()->SelectTexture();
								RENDERINFO.Apply(true);
							}
							else
							{
								mVBPart[c0]->mTexture[i]->mTexture->mRenderMode=i;
								mVBPart[c0]->mTexture[i]->mTexture->mU=mVBPart[c0]->mTexture[i]->mTextureU[(vf1 % mVBPart[c0]->mTexture[i]->mFrames)];
								mVBPart[c0]->mTexture[i]->mTexture->mV=mVBPart[c0]->mTexture[i]->mTextureV[(vf1 % mVBPart[c0]->mTexture[i]->mFrames)];
								mVBPart[c0]->mTexture[i]->mTexture->mUScale=mVBPart[c0]->mTexture[i]->mTextureUScale[(vf1 % mVBPart[c0]->mTexture[i]->mFrames)];
								mVBPart[c0]->mTexture[i]->mTexture->mVScale=mVBPart[c0]->mTexture[i]->mTextureVScale[(vf1 % mVBPart[c0]->mTexture[i]->mFrames)];
								mVBPart[c0]->mTexture[i]->mTexture->SetupRenderMode();
								mVBPart[c0]->mTexture[i]->mTexture->SelectTexture();
							}

						/*	LT.SRS(D3DRS_ALPHATESTENABLE,true);
							LT.SRS(D3DRS_ALPHAREF,0xFF);*/
							
							if ((mVBPart[c0]->mTexture[i]->mTexture->GetFullbright()) && (i==TEXTURE_TYPE_DIFFUSE) && (CMeshRenderer::GetRenderMode()!=R_Shadow))
							{
								// Do first pass with no alpha
								RENDERINFO.SRS(RS_ALPHABLENDENABLE,FALSE);
								RENDERINFO.SRS(RS_ALPHATESTENABLE,FALSE);
								RENDERINFO.Apply();
								LT.D3D_DrawIndexedPrimitive(ToD3DPrimType(mPrimitiveType),
									0, mVBPart[c0]->mNumVertices,
									0, mVBPart[c0]->mNumPrimitives);
								// ...and another with alpha-test but no lighting
								RENDERINFO.SRS(RS_ALPHABLENDENABLE,TRUE);
								BOOL l=RENDERINFO.GetLightingEnabled();
								RENDERINFO.SetLightingEnabled(FALSE);
								RENDERINFO.Apply();
								LT.D3D_DrawIndexedPrimitive(ToD3DPrimType(mPrimitiveType),
									0, mVBPart[c0]->mNumVertices,
									0, mVBPart[c0]->mNumPrimitives);
								RENDERINFO.SRS(RS_ALPHATESTENABLE,TRUE);
								RENDERINFO.SRS(RS_ALPHABLENDENABLE,FALSE);
								RENDERINFO.SetLightingEnabled(l ? true : false);
								RENDERINFO.Apply();
							}
							else
							{
								LT.D3D_DrawIndexedPrimitive(ToD3DPrimType(mPrimitiveType),
									0, mVBPart[c0]->mNumVertices,
									0, mVBPart[c0]->mNumPrimitives);
							}
							
							if (TARGET == PC && (flags & RF_SNOWLAYER)==RF_SNOWLAYER)
								CAtmospherics::GetSnowLayerTexture()->ShutdownRenderMode();
							else
								mVBPart[c0]->mTexture[i]->mTexture->ShutdownRenderMode();						
						}
					}
				}
			}

			if (((flags & RF_CLOAKED)!=0)&&(CMeshRenderer::GetRenderMode()!=R_Shadow)&&(mVBPart[c0]->mActive))
			{
				// Render cloaking effect
				
				ENGINE.GetCloakTexture()->mRenderMode=TEXTURE_TYPE_ENVMAP;
				ENGINE.GetCloakTexture()->SetupRenderMode();
				ENGINE.GetCloakTexture()->SelectTexture();
				RENDERINFO.SRS(RS_LIGHTING,FALSE);
				
				// Set up the animation transform
				
				float t=ENGINE.GetRenderTime()/10.0f;
				
				D3DXMATRIX mat;
				
				mat._11 = 0.5f;	mat._12 = 0.0f; mat._13 = 0.0f; mat._14 = 0.0f; 
				mat._21 = 0.0f; mat._22 = -0.5f;mat._23 = 0.0f; mat._24 = 0.0f; 
				mat._31 = 0.0f; mat._32 = 0.0f; mat._33 = 1.0f; mat._34 = 0.0f; 
				mat._41 = t; mat._42 = t/2.0f; mat._43 = 0.0f; mat._44 = 1.0f; 
				LT.D3D_X_SetTransform( D3DTS_TEXTURE0, &mat );
				
				RENDERINFO.SRS(RS_SRCBLEND,BLEND_SRCALPHA);
				RENDERINFO.SRS(RS_DESTBLEND,BLEND_ONE);
				
				RENDERINFO.Apply(true);
				
				LT.D3D_DrawIndexedPrimitive(ToD3DPrimType(mPrimitiveType),
					0, mVBPart[c0]->mNumVertices,
					0, mVBPart[c0]->mNumPrimitives);
				
				ENGINE.GetCloakTexture()->ShutdownRenderMode();
				ENGINE.GetCloakTexture()->mRenderMode=TEXTURE_TYPE_DIFFUSE;
				
				RENDERINFO.SRS(RS_LIGHTING,TRUE);
				RENDERINFO.SRS(RS_SRCBLEND,BLEND_SRCALPHA);
				RENDERINFO.SRS(RS_DESTBLEND,BLEND_INVSRCALPHA);								
			}	

			if (((flags & RF_HITEFFECT)!=0)&&(CMeshRenderer::GetRenderMode()!=R_Shadow)&&(mVBPart[c0]->mActive))
			{
				// Render hit effect over the top
				
				int oldalpha=CMeshRenderer::GetRenderAlpha();
				CMeshRenderer::SetRenderAlpha((oldalpha*CMeshRenderer::GetHitEffectStrength())/100);					
				
				ENGINE.GetHitEffectTexture()->mRenderMode=TEXTURE_TYPE_ENVMAP;
				ENGINE.GetHitEffectTexture()->SetupRenderMode();
				ENGINE.GetHitEffectTexture()->SelectTexture();
				RENDERINFO.SRS(RS_LIGHTING,FALSE);
				
				// Set up the animation transform
				
				float t=ENGINE.GetRenderTime()/5.0f;
				
				D3DXMATRIX mat;
				
				mat._11 = 0.5f;	mat._12 = 0.0f; mat._13 = 0.0f; mat._14 = 0.0f; 
				mat._21 = 0.0f; mat._22 = -0.5f;mat._23 = 0.0f; mat._24 = 0.0f; 
				mat._31 = 0.0f; mat._32 = 0.0f; mat._33 = 1.0f; mat._34 = 0.0f; 
				mat._41 = t; mat._42 = t/2.0f; mat._43 = 0.0f; mat._44 = 1.0f; 
				LT.D3D_X_SetTransform( D3DTS_TEXTURE0, &mat );
				
				RENDERINFO.SRS(RS_SRCBLEND,BLEND_SRCALPHA);
				RENDERINFO.SRS(RS_DESTBLEND,BLEND_ONE);
				
				RENDERINFO.Apply(true);
				
				LT.D3D_DrawIndexedPrimitive(ToD3DPrimType(mPrimitiveType),
					0, mVBPart[c0]->mNumVertices,
					0, mVBPart[c0]->mNumPrimitives);
				
				ENGINE.GetHitEffectTexture()->ShutdownRenderMode();
				ENGINE.GetHitEffectTexture()->mRenderMode=TEXTURE_TYPE_DIFFUSE;
				CMeshRenderer::SetRenderAlpha(oldalpha);
				
				RENDERINFO.SRS(RS_LIGHTING,TRUE);
				RENDERINFO.SRS(RS_SRCBLEND,BLEND_SRCALPHA);
				RENDERINFO.SRS(RS_DESTBLEND,BLEND_INVSRCALPHA);								
			}	
		}

		// Reset base alpha

		CMESHRENDERER::SetRenderAlpha(basealpha);		

		if (mCurrentShader!=NULL)
		{		
			if (CVertexShader::VertexShadersEmulated)
			{
				// Switch back to hardware
				LT.ForceRS(D3DRS_SOFTWAREVERTEXPROCESSING,false);
			}
		}
	}
}

//---------------------------------------------------------------------------
#ifdef RESBUILDER

void CDXMeshVB::SerializeForDX(class CChunker *c,class CResourceAccumulator *ra)
{
	int i;

	BYTE *temp=new BYTE[sizeof(CDXMeshVB)];
	CDXMeshVB *t=(CDXMeshVB *) temp;
	memcpy(t,this,sizeof(CDXMeshVB));
	
	// Clear all pointers

	SERIALIZE_CLEAR(t->mCurrentShader);
	SERIALIZE_CLEAR(t->mMeshPart);
	SERIALIZE_CLEAR(t->mNext);
		
	c->Start(MKID("PMVB"));
	
	c->Start(MKID("CMVB"));	
	c->Write(t,sizeof(CDXMeshVB),1);
	c->End();

	delete temp;

	for (i=0;i<mNumTextures;i++)
	{
		// SDXMeshMatParts

		// Random stuff

		c->Start(MKID("MMPT"));

		c->Write(&mVBPart[i]->mVBSize,sizeof(mVBPart[i]->mVBSize),1);
		c->Write(&mVBPart[i]->mIBSize,sizeof(mVBPart[i]->mIBSize),1);
		c->Write(&mVBPart[i]->mNumIndices,sizeof(mVBPart[i]->mNumIndices),1);
		c->Write(&mVBPart[i]->mNumVertices,sizeof(mVBPart[i]->mNumVertices),1);
		c->Write(&mVBPart[i]->mNumPrimitives,sizeof(mVBPart[i]->mNumPrimitives),1);
		c->Write(&mVBPart[i]->mActive,sizeof(mVBPart[i]->mActive),1);

		c->End();

		// Index buffer

		UCHAR *idata;

		c->Start(MKID("IBUF"));
		if ((FAILED(mVBPart[i]->mIBuff->Lock(0,0,&idata,0))))
		{
			SASSERT(0,"Unable to lock IB whilst serializing DXMeshVB!");
		}
		c->Write(idata,2,mVBPart[i]->mNumIndices);			
		mVBPart[i]->mIBuff->Unlock();			
		c->End();

		UCHAR *vdata;
		
		c->Start(MKID("VBUF"));
		if (i==0)
		{
			// Only write the vertex buffer for the first part
			
			if ((FAILED(mVBPart[i]->mVBuff->Lock(0,0,&vdata,0))))
			{
				SASSERT(0,"Unable to lock VB whilst serializing DXMeshVB!");		
			}
			c->Write(vdata,mVertexBufferChunkSize,mVBPart[i]->mNumVertices);		
			mVBPart[i]->mVBuff->Unlock();		
		}
		c->End();

		// Texture references

		c->Start(MKID("TEXR"));
		for (int k=0;k<TEXTURE_TYPES;k++)
		{
			int texr=-1;
			if (mVBPart[i]->mTexture[k])
			{
				for (int j=0;j<mMeshPart->mBaseMesh->mNumTextures;j++)
				{
					if (&mMeshPart->mBaseMesh->mTextures[j]==mVBPart[i]->mTexture[k])
						texr=j;
				}
				
				SASSERT(texr>=0,"MeshVB referenced texture not part of base mesh whilst serializing!");
			}
			
			c->Write(&texr,sizeof(texr),1);
		}
		c->End();
	}

	c->End();
}

//---------------------------------------------------------------------------

void CDXMeshVB::SerializeForPS2(class CChunker *c,class CResourceAccumulator *ra)
{
	int i,j;
	char buf[256];

	TRACE("Serializing mesh VB for PS2\n");
	
	c->Start(MKID("P2VB"));
	
	c->Start(MKID("MSVB"));	

	c->Write(&mNumTextures,sizeof(mNumTextures),1);

	int dvsize=sizeof(CMeshDVertex);
	
	c->Write(&dvsize,sizeof(dvsize),1);	
	
	c->End();

	// Set up stripifier

	SetCacheSize(CACHESIZE_GEFORCE3); // Since we don't have a cache!
	SetStitchStrips(false);
	SetMinStripSize(0);
	SetListsOnly(false);
		
	for (i=0;i<mNumTextures;i++)
	{
		c->Start(MKID("MMPV"));

		c->Write(&mVBPart[i]->mActive,sizeof(mVBPart[i]->mActive),1);

		// Texture references

		c->Start(MKID("TEXR"));
		for (int k=0;k<TEXTURE_TYPES;k++)
		{
			int texr=-1;
			if (mVBPart[i]->mTexture[k])
			{
				for (int j=0;j<mMeshPart->mBaseMesh->mNumTextures;j++)
				{
					if (&mMeshPart->mBaseMesh->mTextures[j]==mVBPart[i]->mTexture[k])
						texr=j;
				}
				
				SASSERT(texr>=0,"MeshVB referenced texture not part of base mesh whilst serializing!");
			}
			
			c->Write(&texr,sizeof(texr),1);
		}
		c->End();

		if (mVBPart[i]->mActive)
		{
			// Lock index buffer
			
			UWORD *idata;
			
			if ((FAILED(mVBPart[i]->mIBuff->Lock(0,0,(unsigned char **) &idata,0))))
			{
				SASSERT(0,"Unable to lock IB whilst serializing DXMeshVB for PS2!");
			}

			if (mVBPart[i]->mNumIndices>0)
			{
				SASSERT(mMeshPart->mNumDVertices>0,"Attempt to serialise VB for PS2 from mesh with no DVerts - if you are running the PC build from a resource file, DON'T!");
				SASSERT(mMeshPart->mNumPVertices>0,"Attempt to serialise VB for PS2 from mesh with no PVerts - if you are running the PC build from a resource file, DON'T!");
			}

			// Build list of indices

			UWORD *indices=new UWORD[mVBPart[i]->mNumIndices*3];
			int numindices=0;

			UWORD ia,ib,ic;

			ib=idata[0];
			ic=idata[1];

			BOOL cwwinding=TRUE;

			float texeldensity=0.0f;

			for (j=2;j<mVBPart[i]->mNumIndices;j++)
			{
				ia=ib;
				ib=ic;
				ic=idata[j];

				if ((ia!=ib) && (ia!=ic) && (ib!=ic))
				{
					// Valid triangle

					if (cwwinding)
					{
						indices[numindices++]=ia;
						indices[numindices++]=ib;
						indices[numindices++]=ic;
					}
					else
					{
						indices[numindices++]=ic;
						indices[numindices++]=ib;
						indices[numindices++]=ia;
					}

					// Calculate average texel density

					CMeshDVertex *verta=&mMeshPart->mDVertices[ia];
					CMeshDVertex *vertb=&mMeshPart->mDVertices[ib];
					CMeshDVertex *vertc=&mMeshPart->mDVertices[ic];

					FVector vapos=*(mMeshPart->mPVertices[0]+verta->mPVert);
					FVector vbpos=*(mMeshPart->mPVertices[0]+vertb->mPVert);
					FVector vcpos=*(mMeshPart->mPVertices[0]+vertc->mPVert);

					float tside1=sqrtf( ((vertb->mU-verta->mU)*(vertb->mU-verta->mU)) + ((vertb->mV-verta->mV)*(vertb->mV-verta->mV)) );
					float tside2=sqrtf( ((vertc->mU-verta->mU)*(vertc->mU-verta->mU)) + ((vertc->mV-verta->mV)*(vertc->mV-verta->mV)) );
				
					float texturearea=tside1*tside2*0.5f;

					float wside1=(vbpos-vapos).Magnitude();
					float wside2=(vcpos-vapos).Magnitude();

					float worldarea=wside1*wside2*0.5f;

					float polytexdensity=texturearea/worldarea;

					texeldensity+=polytexdensity;
				}

				cwwinding=!cwwinding;
			}

			if (numindices>0)
				texeldensity/=numindices/3.0f;

			sprintf(buf,"%d unstripped indices in part\n",numindices);
			TRACE(buf);

			sprintf(buf,"%f average texel density\n",texeldensity);
			TRACE(buf);

			// Stripify

			UWORD numprims;
			PrimitiveGroup *prim;

			GenerateStrips(indices,numindices,&prim,&numprims);

			delete [] indices;

			sprintf(buf,"%d strips produced\n",numprims);
			TRACE(buf);

			// How many indices and triangles are there?

			int numstripindices=0;
			int numtriangles=0;

			for (j=0;j<numprims;j++)
			{
				numstripindices+=prim[j].numIndices;
				numtriangles+=prim[j].numIndices-2;

				// Check if we need an extra index at the end to fix the backfacing

				if ((prim[j].numIndices & 1)!=0)
					numstripindices++;
			}

			sprintf(buf,"%d total indices, %d total vertices, %d total triangles, %g average strip length\n",numstripindices,mMeshPart->mNumDVertices,numtriangles,(float) numtriangles/(float) numprims);
			TRACE(buf);

			// simulate vertex cache:
			bool *lUsed = new bool[ mMeshPart->mNumDVertices ];
			for( j=0; j<mMeshPart->mNumDVertices; j++ )
			{
				lUsed[ j ] = FALSE;
			}
			int lVerts = 0;
			for( j=0; j<numprims; j++ )
			{
				for( k=0; k<prim[j].numIndices; k++ )
				{
					if( lUsed[ prim[j].indices[k] ] == FALSE )
					{
						lVerts++;
						lUsed[ prim[j].indices[k] ] = TRUE;
					}
				}
			}
			delete [] lUsed;

			static lTotalIndices = 0, lTotalVertices = 0;
			lTotalIndices += numstripindices;
			lTotalVertices += lVerts;

			sprintf(buf,"%d total indices, %d total vertices\n",lTotalIndices,lTotalVertices);
			TRACE(buf);
			
			// Write out

			c->Start(MKID("NVRT"));

			int numprimitives=numprims;
			c->Write(&numstripindices,sizeof(numstripindices),1);
			c->Write(&numtriangles,sizeof(numtriangles),1);
			c->Write(&texeldensity,sizeof(texeldensity),1);

			c->End();			

			c->Start(MKID("VRTS"));			

			for (j=0;j<numprims;j++)
			{
				ASSERT(prim[j].type==PT_STRIP);

				int indicestowrite=prim[j].numIndices;

				if ((prim[j].numIndices & 1)!=0)
				{
					indicestowrite++; // Add extra index to fix backfacing
				}

				for (k=0;k<indicestowrite;k++)
				{
					UWORD index;

					if (k<(int) prim[j].numIndices)
						index=prim[j].indices[k];
					else
						index=prim[j].indices[prim[j].numIndices-1];

//					SASSERT(index>=0,"Index<0");
//					SASSERT(index<mMeshPart->mNumDVertices,"Index>numIndices");
					
					CMeshDVertex *vert=&mMeshPart->mDVertices[index];
					
					c->Write(vert,dvsize,1);
					float x,y,z;
					
					// Always take position data from frame 0
					
					x=(mMeshPart->mPVertices[0]+vert->mPVert)->X;
					y=(mMeshPart->mPVertices[0]+vert->mPVert)->Y;
					z=(mMeshPart->mPVertices[0]+vert->mPVert)->Z;
					c->Write(&x,sizeof(float),1);
					c->Write(&y,sizeof(float),1);
					c->Write(&z,sizeof(float),1);

					int adc=0;
					if ((k<2) || (k>=(int) prim[j].numIndices))
						adc=1;

					c->Write(&adc,sizeof(adc),1);

					if (mMeshPart->mPartType==MESHPART_TYPE_SKELETAL)
					{
						// Write skeletal data

						for (int k=0;k<BONE_SLOTS;k++)
						{
							int bone=mMeshPart->mBoneSlots[vert->mPVert][k];
							c->Write(&bone,sizeof(bone),1);
						}
					}
				}

			}

			c->End();
		
			delete [] prim;
			
			mVBPart[i]->mIBuff->Unlock();
		}

		c->End();
	}
	
	c->End();
}

//---------------------------------------------------------------------------

void CDXMeshVB::Serialize(CChunker *c,class CResourceAccumulator *ra)
{
	if (ra->GetTargetPlatform()==PC) 
		SerializeForDX(c,ra);	
	else if (ra->GetTargetPlatform()==XBOX)
		SerializeForDX(c,ra);
	else if (ra->GetTargetPlatform()==PS2)
		SerializeForPS2(c,ra);	
}
#endif
//---------------------------------------------------------------------------

void CDXMeshVB::Deserialize(class CChunkReader *c)
{
	int i;

	// Save links

	DeviceObject *tempnextdo=mNext;
	CMESHPART *meshpart=mMeshPart;

	// Save VTable pointers

	void *pcmeshvbvtable=*((void **)this);
	void *devobjvtable=*((void **) ((DeviceObject *)this));
	
	if (c->GetNext()!=MKID("PMVB"))
	{
		SASSERT(0,"DXMeshVB deserialize format failure!");
	}

	if (c->GetNext()!=MKID("CMVB"))
	{
		SASSERT(0,"DXMeshVB deserialize format failure!");	
	}

	if (c->GetSize()!=sizeof(CDXMeshVB))
	{
		SASSERT(0,"DXMeshVB deserialize format failure!");
	}

	c->Read(this,sizeof(CDXMeshVB),1);

	// Restore links
	
	mNext=tempnextdo;
	mMeshPart=meshpart;
	
	// Restore VTable entries
	
	*((void **)this)=pcmeshvbvtable;
	*((void **) ((DeviceObject *)this))=devobjvtable;

	for (i=0;i<mNumTextures;i++)
	{
		// SDXMeshMatParts

		mVBPart[i]=new (MEMTYPE_MESHVB) SDXMeshMatPart;
		
		// Random stuff
		
		if (c->GetNext()!=MKID("MMPT"))
		{
			SASSERT(0,"DXMeshVB deserialize format failure!");
		}
		
		c->Read(&mVBPart[i]->mVBSize,sizeof(mVBPart[i]->mVBSize),1);
		c->Read(&mVBPart[i]->mIBSize,sizeof(mVBPart[i]->mIBSize),1);
		c->Read(&mVBPart[i]->mNumIndices,sizeof(mVBPart[i]->mNumIndices),1);
		c->Read(&mVBPart[i]->mNumVertices,sizeof(mVBPart[i]->mNumVertices),1);
		c->Read(&mVBPart[i]->mNumPrimitives,sizeof(mVBPart[i]->mNumPrimitives),1);
		c->Read(&mVBPart[i]->mActive,sizeof(mVBPart[i]->mActive),1);
		
		// Index buffer

		if (c->GetNext()!=MKID("IBUF"))
		{
			SASSERT(0,"DXMeshVB deserialize format failure!");
		}

		if ((FAILED( LT.D3D_CreateIndexBuffer(mVBPart[i]->mIBSize,D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&mVBPart[i]->mIBuff))))
		{
			SASSERT(0,"Unable to create IB during DXMeshVB deserialization!");
		}

		PROFILE_ALLOCATE(SIBMainMemory,mVBPart[i]->mIBSize);
		PROFILE_ALLOCATE(SIBVideoMemory,mVBPart[i]->mIBSize);

		MEM_MANAGER.TagMem(this,mVBPart[i]->mIBSize,"Meshes/%s/Part %d/VBPart %d/Deserialized index buffer",mMeshPart->mBaseMesh->mName,mMeshPart->mPartNumber,i);
		
		BYTE	*idata;
			
		if ((FAILED(mVBPart[i]->mIBuff->Lock(0, 0, &idata, 0))))
		{
			SASSERT(0,"Unable to lock IB during DXMeshVB deserialization!");
		}
		
		c->Read(idata,2,mVBPart[i]->mNumIndices);
			
		mVBPart[i]->mIBuff->Unlock();			

		// Vertex buffer

		if (c->GetNext()!=MKID("VBUF"))
		{
			SASSERT(0,"DXMeshVB deserialize format failure!");		
		}

		if (i == 0)
		{
			if ((FAILED( LT.D3D_CreateVertexBuffer(mVBPart[i]->mVBSize,D3DUSAGE_WRITEONLY,mFVFFlags,D3DPOOL_MANAGED,&mVBPart[i]->mVBuff,&mVBPart[i]->mpVBData))))
			{
				SASSERT(0,"Unable to create VB during DXMeshVB deserialization!");
			}

			PROFILE_ALLOCATE(SVBMainMemory,mVBPart[i]->mVBSize);
			PROFILE_ALLOCATE(SVBVideoMemory,mVBPart[i]->mVBSize);
			
			MEM_MANAGER.TagMem(this,mVBPart[i]->mVBSize,"Meshes/%s/Part %d/VBPart %d/Deserialized vertex buffer",mMeshPart->mBaseMesh->mName,mMeshPart->mPartNumber,i);		
			
			BYTE *vdata;
			
			if ((FAILED(mVBPart[i]->mVBuff->Lock(0, 0, &vdata, 0))))
			{
				SASSERT(0,"Unable to lock VB during DXMeshVB deserialization!");
			}
			
			c->Read(vdata,mVertexBufferChunkSize,mVBPart[i]->mNumVertices);
			
			mVBPart[i]->mVBuff->Unlock();			
		}
		else
		{
			mVBPart[i]->mVBSize=mVBPart[0]->mVBSize;
			mVBPart[i]->mVBuff=mVBPart[0]->mVBuff;
			mVBPart[i]->mNumVertices=mVBPart[0]->mNumVertices;
			c->Skip();
		}
					
		// Textures

		if (c->GetNext()!=MKID("TEXR"))
		{
			SASSERT(0,"DXMeshVB deserialize format failure!");
		}

		for (int j=0;j<TEXTURE_TYPES;j++)
		{
			int resid;
			c->Read(&resid,sizeof(resid),1);
			if (resid==-1)
				mVBPart[i]->mTexture[j]=NULL;
			else
				mVBPart[i]->mTexture[j]=&meshpart->mBaseMesh->mTextures[resid];		
		}		
	}
	
#if TARGET==XBOX

//	XGCompileDrawIndexedVertices()
#endif
}

//---------------------------------------------------------------------------

D3DPRIMITIVETYPE CDXMeshVB::ToD3DPrimType(EPrimitiveType primtype)
{
	switch (primtype)
	{
		case PRIMTYPE_POINTLIST     :return(D3DPT_POINTLIST);
		case PRIMTYPE_LINELIST      :return(D3DPT_LINELIST);
		case PRIMTYPE_LINESTRIP     :return(D3DPT_LINESTRIP);
		case PRIMTYPE_TRIANGLELIST  :return(D3DPT_TRIANGLELIST);
		case PRIMTYPE_TRIANGLESTRIP :return(D3DPT_TRIANGLESTRIP);
		case PRIMTYPE_TRIANGLEFAN   :return(D3DPT_TRIANGLEFAN);
		default						:SASSERT(0,"Unknown primitive type passed to ToD3DPrimType()");
	}
}

//=============================================-===--==---=---- ---  --   -

EPrimitiveType CDXMeshVB::FromD3DPrimType(D3DPRIMITIVETYPE primtype)
{
	switch (primtype)
	{
		case D3DPT_POINTLIST     :return(PRIMTYPE_POINTLIST);
		case D3DPT_LINELIST      :return(PRIMTYPE_LINELIST);
		case D3DPT_LINESTRIP     :return(PRIMTYPE_LINESTRIP);
		case D3DPT_TRIANGLELIST  :return(PRIMTYPE_TRIANGLELIST);
		case D3DPT_TRIANGLESTRIP :return(PRIMTYPE_TRIANGLESTRIP);
		case D3DPT_TRIANGLEFAN   :return(PRIMTYPE_TRIANGLEFAN);
		default					:SASSERT(0,"Unknown primitive type format passed to FromD3DPrimType()");
	}
}

//---------------------------------------------------------------------------

int CDXMeshVB::GetPolys()
{
	int polys=0;
	for (int i=0;i<mNumTextures;i++)
	{
		if (mVBPart[i]->mActive)
			polys+=mVBPart[i]->mNumPrimitives;
	}
	return(polys);
}

//---------------------------------------------------------------------------

int CDXMeshVB::GetParts()
{
	return(mNumTextures);
}

#endif