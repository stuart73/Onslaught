#include	"common.h"
#include	"stdafx.h"
#include	"Mesh.h"

#ifndef SIMPLE_MESHES
#include    "MeshPart.h"
#include    "MemoryManager.h"
#include	"texture.h"
#include	"Engine.h"
#include	"Console.h"
#include	"DebugLog.h"
#include	"motioncontroller.h"
#include    "Game.h"
#include	"texture.h"
#include	"cliparams.h"
#endif


#include <stdio.h>
#include <string.h>
#include "chunker.h"
#include "renderablething.h"

CMESH					*CMesh::mFirstMesh = NULL;
CMeshTexture			*CMesh::mDefaultTexture;
BOOL					 CMesh::mInLevel = FALSE;

//******************************************************************************************
//******************************************************************************************
//******************************************************************************************
CMesh::CMesh()
{
	mNumParts = 0;
	mParts = NULL;

	mNumEmitters = 0;
	mEmitters = NULL;

	mNumAnimModes = 0;
	mAnimModes = NULL;

	mNumTextures = 0;
	mTextures = NULL;

	mRadius	= 0.5f;

	mLastFrameCollisionGenerated = -2;

	mNextInChain=NULL;

	mSequences=NULL;
	mNumSequences=0;

	mRefCount=0;

	mResourceID=-1;

	strcpy(mName,"");

	mBoundingBox=new (MEMTYPE_MESH) CBoundingBox;

	AddToList();
}

//******************************************************************************************

void CMesh::AddToList()
{
	mNextMesh = mFirstMesh;
	mFirstMesh = (CMESH *) this;
}

//******************************************************************************************

void CMesh::RemoveFromList()
{
	CMesh *c=mFirstMesh;
	CMesh *prev=NULL;
	
	while ((c) && (c!=this))
	{
		prev=c;
		c=c->mNextMesh;
	}
	
	if (c)
	{
		if (prev)
			prev->mNextMesh=this->mNextMesh;
		else
			mFirstMesh=this->mNextMesh;
	}	
}

//******************************************************************************************
CMesh::~CMesh()
{
	// Remove ourselves from the mesh list

	RemoveFromList();

	SAFE_DELETE_ARRAY(mTextures);

	if (mParts)
	{
#if TARGET == XBOX
		delete[] mParts[0];
#else
		SINT	c0;

		for (c0 = 0; c0 < mNumParts; c0 ++)
			SAFE_DELETE(mParts[c0]);
#endif


		SAFE_DELETE_ARRAY(mParts);
	}

	if (mEmitters)
		SAFE_DELETE_ARRAY(mEmitters);

	SAFE_DELETE_ARRAY(mAnimModes);

	if (mNextInChain)
		SAFE_RELEASE(mNextInChain);

	SAFE_DELETE(mBoundingBox);
}

//******************************************************************************************
BOOL	CMesh::InitAll()
{
	ASSERT(mFirstMesh == NULL);

	return TRUE;	
}

//******************************************************************************************
BOOL	CMesh::InitDefaultTexture()
{
//	mDefaultTexture=NULL;
	mDefaultTexture=new( MEMTYPE_MESHTEXTURE ) CMeshTexture();
	mDefaultTexture->ClearToDefaults();
	mDefaultTexture->mTexture = MESH_TEXTURE_SURFACE::GetTextureByName("meshtex\\default.tga");

	return TRUE;
}

//******************************************************************************************

void	CMesh::ShutdownAll()
{
	CMESH *m;
	
	m = mFirstMesh;
		
	while (m)
	{
		m->mRefCount=0;

		m=m->mNextMesh;
	};		

	ReleaseAll();
}

//******************************************************************************************
void	CMesh::ReleaseAll()
{
	SAFE_DELETE(mDefaultTexture);

	// Release any meshes with a refcount of 0 (as meshes may hold refs to each other, 
	// this needs to be done repeatedly)

	BOOL done;
	CMESH *m;

	do
	{
		done=TRUE;

		m = mFirstMesh;
		
		while (m)
		{
			CMESH	*t = m->mNextMesh;
			
			if (m->GetRefCount()==0)
			{
				done=FALSE;
				delete	m;
			}
			
			m = t;
		};
		
	} while (!done);

	m=mFirstMesh;

	TRACE("\n-------------------\n");
	TRACE("Mesh resource leaks\n");
	TRACE("-------------------\n\n");
	
	while (m)
	{
		char buf[256];
		sprintf(buf,"Mesh '%s' leaked : refcount=%d\n",m->mName,m->GetRefCount());
		TRACE(buf);
		m=m->mNextMesh;
	}
	
	TRACE("\n-------------------\n\n");	
}

//******************************************************************************************
void	CMesh::LoadLevelResources( SINT level )
{
	char statusmessage[256];
	sprintf(statusmessage,"Loading mesh resources");
	CONSOLE.Status(statusmessage);

	CONSOLE.StatusDone(statusmessage);
}

//******************************************************************************************
void	CMesh::FreeLevelResources()
{
	mInLevel = FALSE;

	// Release any meshes with a refcount of 0 (as meshes may hold refs to each other, 
	// this needs to be done repeatedly)
	
	BOOL done;
	CMESH *m;
	
	do
	{
		done=TRUE;
		
		m = mFirstMesh;
		
		while (m)
		{
			CMESH	*t = m->mNextMesh;
			
			if (m->GetRefCount()==0)
			{
				done=FALSE;
				delete	m;
			}
			
			m = t;
		};
		
	} while (!done);	
	
	m=mFirstMesh;
	
	TRACE("\n-------------------------------\n");
	TRACE("Mesh end-of-level resource leaks\n");
	TRACE("-------------------------------\n\n");
	
	while (m)
	{
		char buf[256];
		sprintf(buf,"Mesh '%s' leaked : refcount=%d\n",m->mName,m->GetRefCount());
		TRACE(buf);
		m=m->mNextMesh;
	}
	
	TRACE("\n------------------------------\n\n");	
}

//******************************************************************************************
void	CMesh::CheckForAnimations(char* animdesc, CAnimMode* amode)
{
	#define	AM_CHECK(X)	else if(!stricmp(animdesc,#X)) amode->mMode=AM_##X
					if(0) {}
					AM_CHECK(NONE);	
					AM_CHECK(WALK);	
					AM_CHECK(RUN);	
					AM_CHECK(STAND);
					AM_CHECK(SHOOT);
					AM_CHECK(SHOOT1);
					AM_CHECK(SHOOT2);
					AM_CHECK(FLY);
					AM_CHECK(HOVER);
					AM_CHECK(SHOOTWALK);
					AM_CHECK(IDLE);
}

//******************************************************************************************

// Removes any unnecessary textures

void	CMesh::OptimiseTextures()
{
	int initialtextures=mNumTextures;
	int i=0;
	while (i<mNumTextures)
	{
		int match=-1;
		for (int j=0;j<i;j++)
		{
			BOOL identical=TRUE;
			if (mTextures[i].mTexture!=mTextures[j].mTexture)
				identical=FALSE;

			if (mTextures[i].mFrames!=mTextures[j].mFrames)
				identical=FALSE;

			if (identical)
			{
				for (int k=0;k<mTextures[i].mFrames;k++)
				{
					if (mTextures[i].mTextureBlend[k]!=mTextures[j].mTextureBlend[k])
						identical=FALSE;
					if (mTextures[i].mTextureU[k]!=mTextures[j].mTextureU[k])
						identical=FALSE;
					if (mTextures[i].mTextureV[k]!=mTextures[j].mTextureV[k])
						identical=FALSE;
					if (mTextures[i].mTextureUScale[k]!=mTextures[j].mTextureUScale[k])
						identical=FALSE;
					if (mTextures[i].mTextureVScale[k]!=mTextures[j].mTextureVScale[k])
						identical=FALSE;
				}
			}
			
			if (identical)
			{
				match=j;	
			}
		}

		if (match!=-1)
		{
			// Texture is identical to an existing one

			int j;

			// Remove from any meshparts

			for (j=0;j<mNumParts;j++)
			{
				for (int k=0;k<mParts[j]->mNumDVertices;k++)
				{
					for (int l=0;l<TEXTURE_TYPES;l++)
					{
						if (mParts[j]->mDVertices[k].mTexture[l]==i)
							mParts[j]->mDVertices[k].mTexture[l]=match;
					}
				}
			}

			// Remove from list
/*
			SAFE_RELEASE(mTextures[i].mVBuffer);
			SAFE_RELEASE(mTextures[i].mTexture);
			SAFE_DELETE(mTextures[i].mTextureBlend);
			SAFE_DELETE(mTextures[i].mTextureU);
			SAFE_DELETE(mTextures[i].mTextureV);
			SAFE_DELETE(mTextures[i].mTextureUScale);
			SAFE_DELETE(mTextures[i].mTextureVScale);

			for (j=i+1;j<mNumTextures;j++)
			{
				mTextures[j-1]=mTextures[j];
			}

			// Shuffle all texture references down
			
			for (j=0;j<mNumParts;j++)
			{
				for (int k=0;k<mParts[j]->mNumDVertices;k++)
				{
					for (int l=0;l<TEXTURE_TYPES;l++)
					{
						if (mParts[j]->mDVertices[k].mTexture[l]>i)
							mParts[j]->mDVertices[k].mTexture[l]=mParts[j]->mDVertices[k].mTexture[l]-1;
					}
				}
			}

			mNumTextures--;*/
			i++;
		}
		else
			i++;
	}

	char buf[256];
	sprintf(buf,"OptimiseTextures() reduced texture count from %d to %d\n",initialtextures,mNumTextures);
	TRACE(buf);
}

//******************************************************************************************

// Loads a mesh (including chained meshes) from a named file

UINT	MESH	 = (MKID("MESH"));

BOOL	CMesh::InitFromFile(const char *name, SINT num_skins)
{
	SINT	noff = 0;

	char buffer[256];
	sprintf(buffer,"Warning : Loading mesh %s manually\n",name);
	TRACE(buffer);

#if TARGET==PS2
	TRACE("(not even trying!)\n");
	return(FALSE); 
#endif
	
	char statusmessage[256];
	sprintf(statusmessage,"Loading mesh %s",name);

	CONSOLE.Status(statusmessage);

	// Not great.....
#ifndef SIMPLE_MESHES
	// strip down
	SINT i = 0;
	while (name[i])
	{
		if (name[i] == '\\')
			noff = i + 1;
		i ++;
	}
	char	fname[256] = "data\\Meshes\\";
#else
	char	fname[256] = "";
#endif

	ASSERT(name[0]);
	strcat(fname, name + noff);

	strcpy(mName, name + noff);

	ASSERT(strlen(mName) < sizeof(mName) && "Need to make the mName member bigger");

	CMEMBUFFER	mem;
	if (!(mem.InitFromFile(fname)))
	{
		CONSOLE.StatusDone(statusmessage,false);		
		mem.Close();
		return FALSE;
	}

	BOOL result=InitFromMem(mem,num_skins);

	mem.Close();

	CONSOLE.StatusDone(statusmessage);

	return(result);
}

// Loads a mesh (included chained meshes) from the memory stream given

BOOL	CMesh::InitFromMem(CMEMBUFFER &mem, SINT num_skins)
{
#if TARGET == XBOX
	SASSERT(0, "Code removed");
#else
	UINT	ver;

	mem.Read(&ver, 4);
	if (ver != MESH)
	{
		return FALSE;
	}

	mem.Read(&ver, 4);

	// switch based on version
	//************************************************************
	if ((ver == MKID("2.01")) || (ver == MKID("2.02")) || (ver == MKID("2.03")))
	{
		// Full, hierarchical mesh.
		SINT c0, c1;

		// load textures
		SINT				numt;

		mem.Read(&numt, 4);

		mNumTextures=numt;

		if (numt)
		{
			mTextures=new( MEMTYPE_MESHTEXTURE ) CMeshTexture[numt];

			ASSERT(mTextures);

			for (c0 = 0; c0 < numt; c0 ++)
			{
				char text[257], name[350];
				mem.Read(text, 256); // texture name

				mTextures[c0].ClearToDefaults();

#ifdef PARTICLE_ED
				sprintf(name, "c:\\dev\\onslaught2\\data\\textures\\meshtex\\%s", text);
#else
				sprintf(name, "meshtex\\%s", text);
#endif	

				if (strlen(text)>0)
				{
#ifdef SIMPLE_MESHES
					mTextures[c0].mTexture = MESH_TEXTURE_SURFACE::GetTextureByName(name);
#else
					if (strnicmp(text,"A8",2)==0)
						mTextures[c0].mTexture = MESH_TEXTURE_SURFACE::GetTextureByName(name,TEXFMT_A8R8G8B8);
					else if (strnicmp(text,"A4",2)==0)
						mTextures[c0].mTexture = MESH_TEXTURE_SURFACE::GetTextureByName(name,TEXFMT_A4R4G4B4);
					else
						mTextures[c0].mTexture = MESH_TEXTURE_SURFACE::GetTextureByName(name,TEXFMT_A1R5G5B5);
#endif
					ASSERT(mTextures[c0].mTexture);
				}
				else
				{
					mTextures[c0].mTexture = NULL;
					char buffer[256];
					sprintf(buffer,"Warning! Mesh %s contains a null texture name!\n",mName);
					TRACE(buffer);
				}
				mTextures[c0].SetupVBuffer();				
			}
		}

		// and the parts
		mem.Read(&mNumParts, 4);
		ASSERT((mNumParts > 0) && (mNumParts < 100)); // whatever

		// allocate memory
		mParts = new( MEMTYPE_MESH ) CMESHPART*[mNumParts];
		if (!mParts)
		{
			return FALSE;
		}

		for (c0 = 0; c0 < mNumParts; c0 ++)
		{
			mParts[c0] = new( MEMTYPE_MESH ) CMESHPART;
			if (!mParts[c0])
			{
				return FALSE;
			}
		}

		CONSOLE.Status("Loading mesh parts (old-style)");

		// and read each part.
		for (c0 = 0; c0 < mNumParts; c0 ++)
		{
//			CONSOLE.StatusPercentage("Loading mesh parts (old-style)",c0,mNumParts);
			CMESHPART	*mp = mParts[c0];

			mp->mPartNumber = c0;
			mp->mBaseMesh = (CMESH*)(this);

			strcpy(mp->mName,"");

			SINT	vd, vp, t, naf, nvf;
			mem.Read(&vd, 4);
			mem.Read(&vp, 4);
			mem.Read(&nvf, 4);

			if ((ver == MKID("2.02")) || (ver == MKID("2.03")))
				mem.Read(&naf, 4);
			else
				naf = nvf;

			mem.Read(&t, 4);

			if (!(mp->InitMemory(vd, vp, t, nvf, naf)))
			{
				return FALSE;
			}
	
			// read offsets

			mp->mOffsetPos.Load(mem) ;
			mp->mOrientation.Load(mem) ;

		//	mem.Read(&mp->mOffsetPos  , sizeof(mp->mOffsetPos  ));
		//	mem.Read(&mp->mOrientation, sizeof(mp->mOrientation));

			if (c0 == 0)
				mp->mOrientation = ID_FMATRIX;	//!MESH - always set the base orientation to ID

			// Older formats never have any HFrames
			// Therefore we set up a single HFrame containing the
			// orientation, and map every VFrame to it

			mp->mNumHFrames=1;

			// Equally, older formats have no concept of a part type, so
			// assume all parts are meshes

			mp->mPartType=MESHPART_TYPE_MESH;

			mp->mVHFrameMap = new( MEMTYPE_MESH ) UBYTE[mp->mNumVFrames];
			mp->mHFrameOrientations = new( MEMTYPE_MESH ) FMatrix[mp->mNumHFrames];
			mp->mHFramePositions = new( MEMTYPE_MESH ) FVector[mp->mNumHFrames];

			ASSERT(mp->mVHFrameMap);
			ASSERT(mp->mHFrameOrientations);
			ASSERT(mp->mHFramePositions);

			// VFrame->HFrame map (all point at the same HFrame)

			for (int i=0;i<mp->mNumVFrames;i++)
			{
				mp->mVHFrameMap[i]=0;
			}

			// Single HFrame

			mp->mHFrameOrientations[0]=mp->mOrientation;
			mp->mHFramePositions[0]=mp->mOffsetPos;

			// read children
			mem.Read(&mp->mNumChildren, 4);

			ASSERT(mp->mNumChildren >= 0);

			if (mp->mNumChildren)
				mp->mChildren = new (MEMTYPE_MESH) CMESHPART*[mp->mNumChildren] ;
			else
				mp->mChildren = NULL;
			
			SINT	c;
			for (c1 = 0; c1 < mp->mNumChildren; c1 ++)
			{
				mem.Read(&c, 4);
				if ((c < 0) || (c >= mNumParts))
					mp->mChildren[c1] = NULL;
				else
					mp->mChildren[c1] = mParts[c];
			}

			// read parent
			mem.Read(&c, 4);
			if ((c < 0) || (c >= mNumParts))
				mp->mParent = NULL;
			else
				mp->mParent = mParts[c];

			int numskins=0;
			// read the mesh data
			if (ver == MKID("2.01"))
				mp->Read201(mem, (CMESH*)(this), mTextures, numt, numskins);
			else if (ver == MKID("2.02"))
				mp->Read202(mem, (CMESH*)(this), mTextures, numt, numskins);
			else
				mp->Read203(mem, (CMESH*)(this), mTextures, numt, numskins);

			//ASSERT(numskins==1); // Multiple skins aren't supported ATM

			// read collision volumes
			SINT	nc;

			mem.Read(&nc, 4);

			ASSERT(nc == 0); //! Write this!!
		}

		CONSOLE.StatusDone("Loading mesh parts (old-style)");		

		// initialise emitters (here, not in the mesh part structure)

		mem.Read(&mNumEmitters, 4);
		if (mNumEmitters)
		{
			mEmitters = new( MEMTYPE_MESH ) CEmitter[mNumEmitters];
			if (!mEmitters)
			{
				return FALSE;
			}
#ifndef SIMPLE_MESHES			
			MEM_MANAGER.TagMem(mEmitters,sizeof(CEmitter)*mNumEmitters,"Meshes/%s/Emitters",mName);
#endif

			// load Emitters
			for (c0 = 0; c0 < mNumEmitters; c0++)
			{
				CEmitter	*e = &mEmitters[c0];
				SINT		c;

				e->mSubpartEmitter=false;
				e->mNumber=c0;
				strcpy(e->mName,"Emit");

				// read part
				mem.Read(&c, 4);
				if ((c < 0) || (c >= mNumParts))
					e->mMeshPart = NULL;
				else
					e->mMeshPart = mParts[c];

				// and offset

				e->mPos.Load(mem) ;
				e->mOrientation.Load(mem) ;

			//	mem.Read(&e->mPos        , sizeof(FVector));
			//	mem.Read(&e->mOrientation, sizeof(FMatrix));

				// convert to local space
				if (e->mMeshPart)
				{
					e->mPos -= e->mMeshPart->mOffsetPos;
					e->mOrientation = e->mOrientation * e->mMeshPart->mOrientation.Transpose(); //! order??
					e->mTrackingVertex = e->mMeshPart->FindVertex(e->mPos);
				}
			}
		}
		if(!mem.IsMoreData()) mNumAnimModes = 0; // support for old files with no concept of animmodes
		else									// newer ones will just read a 0 for no anim data
		{
			char	animdesc[17];
			animdesc[16]=0;						// null termination for full 16 character names
			mem.Read(&mNumAnimModes, 4);
			if(mNumAnimModes)
			{
				mAnimModes = new( MEMTYPE_MESH ) CAnimMode[mNumAnimModes];
				CAnimMode* amode = mAnimModes;
				for(int i=0; i<mNumAnimModes; amode++, i++)
				{
					amode->mMode = AM_INVALID;
					mem.Read(animdesc,16);
					mem.Read(&amode->mStart,4);
					mem.Read(&amode->mLast,4);
					int	irate;
					mem.Read(&irate,4);

//					irate *= 3;

					CheckForAnimations(animdesc, amode );
		
					amode->mFrameScaler = amode->mLast-amode->mStart;
					if(IsAnimLooped(amode->mMode))
					{
						amode->mFrameScaler++;	// makes looped animations morph last frame to first
					}

					if(amode->mFrameScaler)
					{
						amode->mRate = fabsf(1.0f/(amode->mFrameScaler*(float)irate));
					}
					else	
						amode->mRate = 0;
				}
			}
		}
		// finally, go through the parts and convert the position and matrix offsets into local space
		// assumes that part 0 is the top node
		mParts[0]->HierarchyAdjust(ZERO_FVECTOR, ID_FMATRIX, NULL);
	}
	else if (ver >= MKID("2.04"))
	{
		// 2.04+ version file

		SINT c0, c1;
		int numtexturelayers;

		if (ver >= MKID("2.05"))
		{
			int globaldatasize;
			mem.Read(&globaldatasize,sizeof(globaldatasize));

			if (globaldatasize>0)
			{
				mem.Read(&numtexturelayers,sizeof(numtexturelayers));
				ASSERT(numtexturelayers<=TEXTURE_TYPES); // Eek! We have too many texture layers!
				globaldatasize-=sizeof(numtexturelayers);
			}
		}
		else
			numtexturelayers=1;

		// load textures
		SINT				numt;

		mem.Read(&numt, 4);

		mNumTextures=numt;

		if (numt)
		{
			mTextures = new( MEMTYPE_MESHTEXTURE ) CMeshTexture[numt];

			ASSERT(mTextures);

			for (c0 = 0; c0 < numt; c0 ++)
			{
				char text[257], name[350];
				mem.Read(text, 256); // texture name

				mTextures[c0].ClearToDefaults();
				
#ifdef PARTICLE_ED
				sprintf(name, "c:\\dev\\onslaught2\\data\\textures\\meshtex\\%s", text);
#else
				sprintf(name, "meshtex\\%s", text);
#endif

#ifndef SIMPLE_MESHES
				if (stricmp(text,"ScreenContents.tga")==0)
					mTextures[c0].mTexture = ENGINE.GetScreenTexture();
				else
				{
					if (strnicmp(text,"A8",2)==0)
						mTextures[c0].mTexture = CTEXTURE::GetTextureByName(name,TEXFMT_A8R8G8B8);
					else if (strnicmp(text,"A4",2)==0)
						mTextures[c0].mTexture = CTEXTURE::GetTextureByName(name,TEXFMT_A4R4G4B4);
					else
						mTextures[c0].mTexture = CTEXTURE::GetTextureByName(name,TEXFMT_A1R5G5B5);
				}
#else
				mTextures[c0].mTexture = CTEXTURE::GetTextureByName(name);
#endif

				if (ver >= MKID("2.07"))
				{
					// Read additional material properties

					int frames,framesinfile;

					mem.Read(&framesinfile,sizeof(framesinfile));

					//frames=framesinfile;
					frames=1; // Hack - to deal with broken exporter exporting all frames all the time... should be fixed!

					mTextures[c0].SetNumFrames(frames);

					mTextures[c0].mMaxBlend=0.0f;

					for (int i=0;i<framesinfile;i++)
					{
						float a,u,v,us,vs;
					
						mem.Read(&a,sizeof(a));
						mem.Read(&u,sizeof(u));
						mem.Read(&v,sizeof(v));
						mem.Read(&us,sizeof(us));
						mem.Read(&vs,sizeof(vs));

						if (i<frames)
						{
							mTextures[c0].mTextureBlend[i]=a;
							mTextures[c0].mTextureU[i]=u;
							mTextures[c0].mTextureV[i]=v;
							mTextures[c0].mTextureUScale[i]=us;
							mTextures[c0].mTextureVScale[i]=vs;
							if (a>mTextures[c0].mMaxBlend)
								mTextures[c0].mMaxBlend=a;
						}
					}
				}

				ASSERT(mTextures[c0].mTexture);
				mTextures[c0].SetupVBuffer();				
			}
		}

		// and the parts
		mem.Read(&mNumParts, 4);
		ASSERT((mNumParts > 0) && (mNumParts < 10000)); // whatever

		// allocate memory
		mParts = new( MEMTYPE_MESH ) CMESHPART*[mNumParts];
		if (!mParts)
		{
			return FALSE;
		}

		for (c0 = 0; c0 < mNumParts; c0 ++)
		{
			mParts[c0] = new( MEMTYPE_MESH ) CMESHPART;
			if (!mParts[c0])
			{
				return FALSE;
			}
		}

		CONSOLE.Status("Loading mesh parts");		

		// and read each part.
		for (c0 = 0; c0 < mNumParts; c0 ++)
		{	
			CMESHPART	*mp = mParts[c0];
			
			mp->mBaseMesh = (CMESH*)(this);
			
			mp->mPartNumber = c0;

			strcpy(mp->mName,"");

			// Read the mesh part type

			mem.Read(&mp->mPartType, sizeof(mp->mPartType));

			char statusmessage[64];

			sprintf(statusmessage,"[%d%%] ",(c0*100)/mNumParts);

			if (mp->mPartType==MESHPART_TYPE_MESH)
				strcat(statusmessage,"[normal]");
			if (mp->mPartType==MESHPART_TYPE_SKELETAL)
				strcat(statusmessage,"[skeletal]");
			if (mp->mPartType==MESHPART_TYPE_NULL)
				strcat(statusmessage,"[null]");
			if (mp->mPartType==MESHPART_TYPE_CAMERA)
				strcat(statusmessage,"[camera]");
			if (mp->mPartType==MESHPART_TYPE_EMITTER)
				strcat(statusmessage,"[emitter]");
			if (mp->mPartType==MESHPART_TYPE_REFERENCE)
				strcat(statusmessage,"[reference]");
			
//			CONSOLE.StatusProgress("Loading mesh parts",statusmessage);

			// Read name

			if (ver >= MKID("2.05"))
				mem.Read(&mp->mName,sizeof(mp->mName));
			
			if ((mp->mPartType==MESHPART_TYPE_MESH) || 
				(mp->mPartType==MESHPART_TYPE_SKELETAL))
			{
				// Normal mesh

				SINT	vd, vp, t, naf, nvf;
				mem.Read(&vd, 4);
				mem.Read(&vp, 4);
				mem.Read(&nvf, 4);

				mem.Read(&naf, 4);

				mem.Read(&t, 4);

				if (!(mp->InitMemory(vd, vp, t, nvf, naf)))
				{
					return FALSE;
				}

				// Number of HFrames

				mem.Read(&mp->mNumHFrames,sizeof(mp->mNumHFrames));

				// Allocate some memory for these

				mp->mVHFrameMap = new( MEMTYPE_MESH ) UBYTE[mp->mNumVFrames];
				mp->mHFrameOrientations = new( MEMTYPE_MESH ) FMatrix[mp->mNumHFrames];
				mp->mHFramePositions = new( MEMTYPE_MESH ) FVector[mp->mNumHFrames];

				ASSERT(mp->mVHFrameMap);
				ASSERT(mp->mHFrameOrientations);
				ASSERT(mp->mHFramePositions);

				// This can't be a camera, so no FOV data
				mp->mHFrameFOVs=NULL;

				// VFrame->HFrame map

				int i;
				float lVHFrameMap;
				for (i=0;i<mp->mNumVFrames;i++)
				{
					mem.Read(&lVHFrameMap,sizeof(lVHFrameMap));
					SASSERT( (lVHFrameMap < 256.f) && (lVHFrameMap >= 0.f ), "VHFrameMap out of bounds!!" );
					mp->mVHFrameMap[i] = (UBYTE)lVHFrameMap;
				}

				// HFrames

				for (i=0;i<mp->mNumHFrames;i++)
				{
				
					mp->mHFramePositions[i].Load(mem) ;

					//mem.Read(&mp->mHFramePositions[i],sizeof(mp->mHFramePositions[i]));
					
					float x,y,z,w;
					float sx,sy,sz,sw;
					FVector scale;

					mem.Read(&x,sizeof(x));
					mem.Read(&y,sizeof(y));
					mem.Read(&z,sizeof(z));
					mem.Read(&w,sizeof(w));

					scale.Load(mem) ;
				//	mem.Read(&scale,sizeof(scale));
					mem.Read(&sx,sizeof(sx));
					mem.Read(&sy,sizeof(sy));
					mem.Read(&sz,sizeof(sz));
					mem.Read(&sw,sizeof(sw));

					z=-z;

					// ** QuatToMatrix

					float xx,xy,xz,xw,yy,yz,yw,zz,zw;
					FMatrix m;

					m.MakeID();
					xx      = x * x;
					xy      = x * y;
					xz      = x * z;
					xw      = x * w;

					yy      = y * y;
					yz      = y * z;
					yw      = y * w;

					zz      = z * z;
					zw      = z * w;

					m.Row[0].X  = 1 - 2 * ( yy + zz );
					m.Row[0].Y  =     2 * ( xy - zw );
					m.Row[0].Z  =     2 * ( xz + yw );

					m.Row[1].X  =     2 * ( xy + zw );
					m.Row[1].Y  = 1 - 2 * ( xx + zz );
					m.Row[1].Z  =     2 * ( yz - xw );

					m.Row[2].X  =     2 * ( xz - yw );
					m.Row[2].Y  =     2 * ( yz + xw );
					m.Row[2].Z  = 1 - 2 * ( xx + yy );					
					
					mp->mHFrameOrientations[i]=m;

					// ** End

					sz=-sz;

					// Scale rotation

					m.MakeID();
					xx      = sx * sx;
					xy      = sx * sy;
					xz      = sx * sz;
					xw      = sx * sw;

					yy      = sy * sy;
					yz      = sy * sz;
					yw      = sy * sw;

					zz      = sz * sz;
					zw      = sz * sw;

					m.Row[0].X  = 1 - 2 * ( yy + zz );
					m.Row[0].Y  =     2 * ( xy - zw );
					m.Row[0].Z  =     2 * ( xz + yw );

					m.Row[1].X  =     2 * ( xy + zw );
					m.Row[1].Y  = 1 - 2 * ( xx + zz );
					m.Row[1].Z  =     2 * ( yz - xw );

					m.Row[2].X  =     2 * ( xz - yw );
					m.Row[2].Y  =     2 * ( yz + xw );
					m.Row[2].Z  = 1 - 2 * ( xx + yy );	

					// ** End

					FMatrix stm;
					stm.MakeID();

					stm.Row[0] *= scale.X;
					stm.Row[1] *= scale.Y;
					stm.Row[2] *= scale.Z;

					// ** NOTE : This applies scale into the orientation matrix
					//           this is a hack whilst we don't have proper scale
					//           support, and is only intended to allow flipped objects
					//           to work properly.

					mp->mHFrameOrientations[i]= mp->mHFrameOrientations[i] * m.Inverse() * stm * m;
				}

				/*if (c0 == 0)
					mp->mHFrameOrientations[0] = ID_FMATRIX;	//!MESH - always set the base orientation to ID*/

				mp->mOrientation=mp->mHFrameOrientations[0]; // Hack for the moment
				mp->mOffsetPos=mp->mHFramePositions[0];

				// read children
				mem.Read(&mp->mNumChildren, 4);

				ASSERT(mp->mNumChildren >= 0);
				
				if (mp->mNumChildren)
					mp->mChildren = new (MEMTYPE_MESH) CMESHPART*[mp->mNumChildren];
				else
					mp->mChildren = NULL;
				
				SINT	c;
				for (c1 = 0; c1 < mp->mNumChildren; c1 ++)
				{
					mem.Read(&c, 4);
					if ((c < 0) || (c >= mNumParts))
						mp->mChildren[c1] = NULL;
					else
						mp->mChildren[c1] = mParts[c];
				}

				// read parent
				mem.Read(&c, 4);
				if ((c < 0) || (c >= mNumParts))
					mp->mParent = NULL;
				else
					mp->mParent = mParts[c];

				// read the mesh data
				mp->Read204(mem, (CMESH*)(this), mTextures, numt, 1, numtexturelayers,ver);

				// read collision volumes
				SINT	nc;

				mem.Read(&nc, 4);

				ASSERT(nc == 0); //! Write this!!
			}
			else if ((mp->mPartType==MESHPART_TYPE_NULL) || (mp->mPartType==MESHPART_TYPE_CAMERA) || (mp->mPartType==MESHPART_TYPE_EMITTER) || (mp->mPartType==MESHPART_TYPE_REFERENCE))
			{
				// "Null" mesh part - no contents (just part orientation data)

				// Number of VFrames

				SINT nvf;

				mem.Read(&nvf, 4);

				if (!(mp->InitMemory(0,0,0,nvf,0)))
				{
					return FALSE;
				}
			
				// Number of HFrames

				mem.Read(&mp->mNumHFrames,sizeof(mp->mNumHFrames));

				// Allocate some memory for these

				mp->mVHFrameMap = new( MEMTYPE_MESH ) UBYTE[mp->mNumVFrames];
				mp->mHFrameOrientations = new( MEMTYPE_MESH ) FMatrix[mp->mNumHFrames];
				mp->mHFramePositions = new( MEMTYPE_MESH ) FVector[mp->mNumHFrames];

				ASSERT(mp->mVHFrameMap);
				ASSERT(mp->mHFrameOrientations);
				ASSERT(mp->mHFramePositions);

				if (mp->mPartType==MESHPART_TYPE_CAMERA)
				{
					// Allocate memory for the FOV data
					mp->mHFrameFOVs = new( MEMTYPE_MESH ) float[mp->mNumHFrames];
					ASSERT(mp->mHFrameFOVs);
				}
				else
				{
					// No FOV data
					mp->mHFrameFOVs=NULL;
				}

				// VFrame->HFrame map
				
				int i;
				float lVHFrameMap;
				for (i=0;i<mp->mNumVFrames;i++)
				{
					mem.Read(&lVHFrameMap,sizeof(lVHFrameMap));
					SASSERT( (lVHFrameMap < 256.f) && (lVHFrameMap >= 0.f ), "VHFrameMap out of bounds!!" );
					mp->mVHFrameMap[i] = (UBYTE)lVHFrameMap;
				}

				// HFrames

				for (i=0;i<mp->mNumHFrames;i++)
				{
					mp->mHFramePositions[i].Load(mem) ;

				//	mem.Read(&mp->mHFramePositions[i],sizeof(mp->mHFramePositions[i]));
					
					float x,y,z,w;
					float sx,sy,sz,sw;
					FVector scale;

					mem.Read(&x,sizeof(x));
					mem.Read(&y,sizeof(y));
					mem.Read(&z,sizeof(z));
					mem.Read(&w,sizeof(w));
					scale.Load(mem) ;
				//	mem.Read(&scale,sizeof(scale));
					mem.Read(&sx,sizeof(sx));
					mem.Read(&sy,sizeof(sy));
					mem.Read(&sz,sizeof(sz));
					mem.Read(&sw,sizeof(sw));

					if (mp->mPartType==MESHPART_TYPE_CAMERA)
					{
						// Read camera FOV
						float fov;
						mem.Read(&fov,sizeof(fov));
						mp->mHFrameFOVs[i]=fov;
					}

					z=-z;

					// ** QuatToMatrix

					float xx,xy,xz,xw,yy,yz,yw,zz,zw;
					FMatrix m;

					m.MakeID();
					xx      = x * x;
					xy      = x * y;
					xz      = x * z;
					xw      = x * w;

					yy      = y * y;
					yz      = y * z;
					yw      = y * w;

					zz      = z * z;
					zw      = z * w;

					m.Row[0].X  = 1 - 2 * ( yy + zz );
					m.Row[0].Y  =     2 * ( xy - zw );
					m.Row[0].Z  =     2 * ( xz + yw );

					m.Row[1].X  =     2 * ( xy + zw );
					m.Row[1].Y  = 1 - 2 * ( xx + zz );
					m.Row[1].Z  =     2 * ( yz - xw );

					m.Row[2].X  =     2 * ( xz - yw );
					m.Row[2].Y  =     2 * ( yz + xw );
					m.Row[2].Z  = 1 - 2 * ( xx + yy );					
					
					mp->mHFrameOrientations[i]=m;

					// ** End

					sz=-sz;

					// Scale rotation

					m.MakeID();
					xx      = sx * sx;
					xy      = sx * sy;
					xz      = sx * sz;
					xw      = sx * sw;

					yy      = sy * sy;
					yz      = sy * sz;
					yw      = sy * sw;

					zz      = sz * sz;
					zw      = sz * sw;

					m.Row[0].X  = 1 - 2 * ( yy + zz );
					m.Row[0].Y  =     2 * ( xy - zw );
					m.Row[0].Z  =     2 * ( xz + yw );

					m.Row[1].X  =     2 * ( xy + zw );
					m.Row[1].Y  = 1 - 2 * ( xx + zz );
					m.Row[1].Z  =     2 * ( yz - xw );

					m.Row[2].X  =     2 * ( xz - yw );
					m.Row[2].Y  =     2 * ( yz + xw );
					m.Row[2].Z  = 1 - 2 * ( xx + yy );	

					// ** End

					FMatrix stm;
					stm.MakeID();

					stm.Row[0] *= scale.X;
					stm.Row[1] *= scale.Y;
					stm.Row[2] *= scale.Z;

					// ** NOTE : This applies scale into the orientation matrix
					//           this is a hack whilst we don't have proper scale
					//           support, and is only intended to allow flipped objects
					//           to work properly.

					mp->mHFrameOrientations[i]= mp->mHFrameOrientations[i] * m.Inverse() * stm * m;
				}

				mp->mOrientation=mp->mHFrameOrientations[0]; // Hack for the moment
				mp->mOffsetPos=mp->mHFramePositions[0];

/*				if (c0 == 0)
					mp->mOrientation = ID_FMATRIX;	//!MESH - always set the base orientation to ID*/

				// Read children

				mem.Read(&mp->mNumChildren, 4);

				ASSERT(mp->mNumChildren >= 0);

				if (mp->mNumChildren)
					mp->mChildren = new (MEMTYPE_MESH) CMESHPART*[mp->mNumChildren];
				else
					mp->mChildren = NULL;
				
				SINT	c;
				for (c1 = 0; c1 < mp->mNumChildren; c1 ++)
				{
					mem.Read(&c, 4);
					if ((c < 0) || (c >= mNumParts))
						mp->mChildren[c1] = NULL;
					else
						mp->mChildren[c1] = mParts[c];
				}

				// Read parent
				mem.Read(&c, 4);
				if ((c < 0) || (c >= mNumParts))
					mp->mParent = NULL;
				else
					mp->mParent = mParts[c];

				if (mp->mPartType==MESHPART_TYPE_REFERENCE)
				{
					// Read reference 
					int refno;
					mem.Read(&refno,sizeof(refno));
 
					ASSERT(refno>=0);
					ASSERT(refno<mNumParts);

					mp->mReferenceTo=mParts[refno];
				}

				mp->mBaseMesh = (CMESH*)(this);

				// read collision volumes
				SINT	nc;

				mem.Read(&nc, 4);

				ASSERT(nc == 0); //! Write this!!
			}
			else
			{
				// Unknown mesh part
				char buffer[256];
				sprintf(buffer,"Unknown mesh part type %s (%d)\n",mp->mName,mp->mPartType);
				TRACE(buffer);
				SASSERT(0,"Unknown mesh part type - you probably need a newer executable!");
			}
		}

		CONSOLE.StatusDone("Loading mesh parts");

		// initialise emitters (here, not in the mesh part structure)

		mem.Read(&mNumEmitters, 4);
		if (mNumEmitters)
		{
			mEmitters = new( MEMTYPE_MESH ) CEmitter[mNumEmitters];
			if (!mEmitters)
			{
				return FALSE;
			}

			// load Emitters
			for (c0 = 0; c0 < mNumEmitters; c0++)
			{
				CEmitter	*e = &mEmitters[c0];
				SINT		part;
				char		name[256];
				SINT		number;

				// Emitter name

				mem.Read(&name,256);

				// Emitter number

				mem.Read(&number,sizeof(number));

				// Emitter part ID

				mem.Read(&part, sizeof(part));
				if ((part < 0) || (part >= mNumParts))
					e->mMeshPart = NULL;
				else
					e->mMeshPart = mParts[part];

				e->mSubpartEmitter=true;

				// New-style emitters have no position or identity

				e->mPos.X=0;
				e->mPos.Y=0;
				e->mPos.Z=0;
				strcpy(e->mName,name);
				e->mNumber=number;

				e->mOrientation.MakeID();
			}
		}

		if(!mem.IsMoreData()) mNumAnimModes = 0; // support for old files with no concept of animmodes
		else									// newer ones will just read a 0 for no anim data
		{
			char	animdesc[17];
			animdesc[16]=0;						// null termination for full 16 character names
			mem.Read(&mNumAnimModes, 4);
			if(mNumAnimModes)
			{
				mAnimModes = new( MEMTYPE_MESH ) CAnimMode[mNumAnimModes];
				CAnimMode* amode = mAnimModes;
				for(int i=0; i<mNumAnimModes; amode++, i++)
				{
					amode->mMode = AM_INVALID;
					mem.Read(animdesc,16);
					mem.Read(&amode->mStart,4);
					mem.Read(&amode->mLast,4);
					int	irate;
					mem.Read(&irate,4);

					strcpy(amode->mModeName,animdesc);

//					irate *= 3;

					CheckForAnimations(animdesc, amode );

					// Auto-allocate a new mode ID if we didn't recognise it

					if (amode->mMode==AM_INVALID)
					{
						amode->mMode=(EAnimMode) (1000+i);
					}				
		
					amode->mFrameScaler = amode->mLast-amode->mStart;
					if(IsAnimLooped(amode->mMode))
					{
						amode->mFrameScaler++;	// makes looped animations morph last frame to first
					}

					if(amode->mFrameScaler)
							amode->mRate = fabsf(1.0f/(amode->mFrameScaler*(float)irate));
					else	amode->mRate = 0;
				}
			}
		}

		// Cutscene data

		mem.Read(&mNumSequences,sizeof(mNumSequences));
		if (mNumSequences>0)
		{
			mSequences=new( MEMTYPE_MESH ) CCutsceneSequence[mNumSequences];
			for (int i=0;i<mNumSequences;i++)
			{
				int camera;
				mem.Read(&camera,sizeof(camera));
				mem.Read(&mSequences[i].mStart,sizeof(mSequences[i].mStart));
				mem.Read(&mSequences[i].mEnd,sizeof(mSequences[i].mEnd));
				ASSERT(camera>=0);
				ASSERT(camera<mNumParts);
				mSequences[i].mCamera=mParts[camera];
				ASSERT(mSequences[i].mCamera->mPartType==MESHPART_TYPE_CAMERA);
			}
		}
	
		// Disallow meshes with no texture at the moment as the renderer doesn't cope

		if (numt==0)
		{
			SASSERT(0,"Mesh with no texture found!");
		}

		// Check to see if there are any chained meshes

		if (mem.IsMoreData())
		{
			// There are!
			//OutputDebugString("Chained mesh found!\n");

			mNextInChain=new( MEMTYPE_MESH ) CMESH();
			if (!mNextInChain->InitFromMem(mem,num_skins))
			{
				TRACE("Error loading chained mesh!");
				return FALSE;
			}
			mNextInChain->AddRef();
		}
	}
	else
	{
		// Unknown format(?)
		return FALSE;
	}
	

//	mWrap = TRUE; // probably

	int i;

	// Optimise texture usage

	OptimiseTextures();
	
	// Perform poly reduction

/*	for (i=0;i<mNumParts;i++)
		mParts[i]->ReducePolys();*/

	// Build the substitute chains

	for (i=0;i<mNumParts;i++)
	{
		mParts[i]->BuildChain();
	}

	// See if we can reduce the number of frames in the animation
	
#ifndef SIMPLE_MESHES
	if (CLIPARAMS.mDecimateMeshes)
		ReduceFrames();
#endif

	// Try to optimise the parts
	
#ifndef SIMPLE_MESHES
	if (!CLIPARAMS.mNoMeshPartReduction)
		OptimiseParts();
#endif

	// Initialise bounding volumes

	if (mParts)
	{
		SINT	c0;
		
		for (c0 = 0; c0 < mNumParts; c0 ++)
		{
			if (mParts[c0])
			{
				mParts[c0]->GenerateSimpleBoundingBox();
				
				// Put a fake bounding box on cameras
				
				if (mParts[c0]->mPartType==MESHPART_TYPE_CAMERA)
				{
					mParts[c0]->mBoundingBox->mOrigin.Set(0,0,0);
					mParts[c0]->mBoundingBox->mAxes.Set(0.05f,0.05f,0.1f);
					mParts[c0]->mBoundingBox->CalculateRadius();
				}
				
				// Put a fake bounding box on emitters
				
				if (mParts[c0]->mPartType==MESHPART_TYPE_EMITTER)
				{
					mParts[c0]->mBoundingBox->mOrigin.Set(0,0,0);
					mParts[c0]->mBoundingBox->mAxes.Set(0.1f,0.1f,0.1f);
					mParts[c0]->mBoundingBox->CalculateRadius();
				}
			}
		}
	}

	// Build MeshVBs

#ifndef SIMPLE_MESHES
	for (i=0;i<mNumParts;i++)
	{
		mParts[i]->mMeshVB->InitDeviceObjects();			// ## GC This Fixes the invisible plane bug!!!
	}
#endif

	// Build the position caches 

	for (i=0;i<mNumParts;i++)
	{
		mParts[i]->BuildCache();
	}

	// Build a bounding box for the entire mesh

	if (mParts)
	{
		float	minx =  999999.f;
		float	miny =  999999.f;
		float	minz =  999999.f;
		float	maxx = -999999.f;
		float	maxy = -999999.f;
		float	maxz = -999999.f;
		float	maxdist = 0;
		float	maxcuefactor = 0;

		mBoundingBox->mValid=false;

		for (int i=0;i<mNumParts;i++)
		{
			if (mParts[i])
			{
				CMeshPart *datapart=mParts[i]->GetMeshDataPart();
			
				
				if (datapart != NULL &&
					datapart->mBoundingBox->mValid &&
					datapart->mPartType!=MESHPART_TYPE_EMITTER)
				{
					FMatrix ori;
					FVector pos;
					mParts[i]->GetWorldTransforms(0,0,0,NULL,&ori,&pos,false,true);
					
					FVector xa=FVector(datapart->mBoundingBox->mAxes.X,0,0);
					FVector ya=FVector(0,datapart->mBoundingBox->mAxes.Y,0);
					FVector za=FVector(0,0,datapart->mBoundingBox->mAxes.Z);
					
					FVector v=ori*datapart->mBoundingBox->mOrigin;
					
					FVector vp[8];
					
					vp[0]=pos+v+(ori * ( xa+ya+za));
					vp[1]=pos+v+(ori * ( xa+ya-za));
					vp[2]=pos+v+(ori * ( xa-ya-za));
					vp[3]=pos+v+(ori * ( xa-ya+za));
					
					vp[4]=pos+v+(ori * (-xa+ya+za));
					vp[5]=pos+v+(ori * (-xa+ya-za));
					vp[6]=pos+v+(ori * (-xa-ya-za));
					vp[7]=pos+v+(ori * (-xa-ya+za));
					
					for (int j=0;j<8;j++)
					{
						if (vp[j].X < minx)	minx = vp[j].X;
						if (vp[j].Y < miny)	miny = vp[j].Y;
						if (vp[j].Z < minz)	minz = vp[j].Z;
						if (vp[j].X > maxx)	maxx = vp[j].X;
						if (vp[j].Y > maxy)	maxy = vp[j].Y;
						if (vp[j].Z > maxz)	maxz = vp[j].Z;
						
						float	d = vp[j].MagnitudeSq();
						if (d > maxdist)
							maxdist = d;					
					}
					
					if (mParts[i]->mCueFactor>maxcuefactor)
						maxcuefactor=mParts[i]->mCueFactor;
					mBoundingBox->mValid=true;
				}
			}
		}

		mBoundingBox->mOrigin = (FVector(minx, miny, minz) + FVector(maxx, maxy, maxz)) / 2.f;
		mBoundingBox->mAxes   = (FVector(minx, miny, minz) - FVector(maxx, maxy, maxz)) / 2.f;
		mBoundingBox->mAxes.X = FABS(mBoundingBox->mAxes.X);
		mBoundingBox->mAxes.Y = FABS(mBoundingBox->mAxes.Y);
		mBoundingBox->mAxes.Z = FABS(mBoundingBox->mAxes.Z);

		mRadius = sqrtf(maxdist);

		if (!mBoundingBox->mValid)
		{
			mBoundingBox->mOrigin.Set(0,0,0);
			mBoundingBox->mAxes.Set(0,0,0);
			mRadius=0.0f;
		}

		mBoundingBox->CalculateRadius();

		mCueFactor = maxcuefactor;
	}

	// Calculate part base positions

	CalculateBases();

#endif

	return TRUE;

}

//******************************************************************************************

int			CMesh::GetNumEmitters()
{
	if (mNextInChain)
		return(mNumEmitters+mNextInChain->GetNumEmitters());
	else
		return(mNumEmitters);
}

//******************************************************************************************

void		CMesh::GetEmitterNameByID(int n,char *name,int *num)
{
	if (n<mNumEmitters)
	{
		strcpy(name,mEmitters[n].mName);
		(*num)=mEmitters[n].mNumber;
	}
	else
	{
		if (mNextInChain)
		{
			mNextInChain->GetEmitterNameByID(n-mNumEmitters,name,num);
		}
		else
		{
			strcpy(name,"");
			(*num)=-1;
		}
	}
}

//******************************************************************************************

CMESHPART	*CMesh::GetEmitterSubpartByID(int n)
{
	if (n<mNumEmitters)
	{
		return(mEmitters[n].mMeshPart);
	}
	else
	{
		if (mNextInChain)
		{
			return(mNextInChain->GetEmitterSubpartByID(n-mNumEmitters));
		}
	}

	return(NULL);
}

//******************************************************************************************

CMESHPART	*CMesh::GetMovieCameraSubpart(int frame)
{
	for (int i=0;i<mNumSequences;i++)
	{
		if ((mSequences[i].mStart<=frame) && 
		    (mSequences[i].mEnd>=frame))
		{
			return(mSequences[i].mCamera);
		}
	}

	// We didn't find a valid camera, so try to see if 
	// we can locate it elsewhere in this mesh chain

	if (mNextInChain)
		return(mNextInChain->GetMovieCameraSubpart(frame));
	else
		return(NULL);
}

//******************************************************************************************

EAnimMode	CMesh::GetAnimModeByName(char *name)
{
	CAnimMode *am=mAnimModes;
	for (int i=0;i<mNumAnimModes; i++)
	{
		if (stricmp(name,am->mModeName)==0)
		{
			return(am->mMode);
		}
		am++;
	}
	return(AM_INVALID);
}

//******************************************************************************************

CAnimMode	*CMesh::GetAnimModePtr(EAnimMode mode)
{
	CAnimMode *am=mAnimModes;
	for (int i=0;i<mNumAnimModes; i++)
	{
		if (am->mMode==mode)
			return(am);
		am++;
	}
	return(NULL);
}



//******************************************************************************************
char* CMesh::GetName()
{
	CMESH	*m = mFirstMesh;
	while (m)
	{
		if (m==this)
		{
			return m->mName ;
		}

		m = m->mNextMesh;
	}

	static char default_name[] = "unknown mesh name";
	
	return default_name;
}



//******************************************************************************************
CMESH	*CMesh::GetMesh(const char *name, SINT num_skins)
{
	// do we have this mesh already?
	CMESH	*m = mFirstMesh;

	while (m)
	{
		if (!(stricmp(name, m->mName)))
		{
			m->AddRef();
			return m;
		}

		m = m->mNextMesh;
	}

	// nope - try to load it.
	CMESH	*n = new( MEMTYPE_MESH ) CMESH;

	if (n)
	{
		// try load from disk
		if (n->InitFromFile(name, num_skins))
		{
			if( mInLevel )
			{
				LOG.AddMessage( "Mesh '%s' not found in level resource file", name );
			}

			n->AddRef();

			return n;
		}

		n->ForceDelete();
	}

#ifndef SIMPLE_MESHES
	// return the default mesh, because I've broken some of the earlier data formats.
	return(NULL); //ENGINE.GetDefaultMesh();
#else
	return NULL;
#endif
}

//******************************************************************************************
float	CMesh::GetFrameIncrement(EAnimMode a, SINT* realindex)
{
	CAnimMode* am=mAnimModes;
	for(int i=0; i<mNumAnimModes; am++, i++)
	{
		if(am->mMode == a)
		{
			*realindex = i; // nasty
			return am->mRate;
		}
	}
	*realindex = -1;
	return 0;
}

//******************************************************************************************

int		CMesh::GetEmitterIDByName(char *name,int num)
{
	for (int i=0;i<mNumEmitters;i++)
	{
		if ((stricmp(mEmitters[i].mName,name)==0) &&
			(mEmitters[i].mNumber==num))
		{
			return(i);
		}
	}

	if (mNextInChain)
	{
		int n=mNextInChain->GetEmitterIDByName(name,num);

		if (n==-1)
			return(-1);
		else
			return(n+mNumEmitters);
	}

	return(-1);
}

//******************************************************************************************

CMESHPART	*CMesh::GetEmitterSubpart(char *name,int num)
{
	for (int i=0;i<mNumEmitters;i++)
	{
		if ((stricmp(mEmitters[i].mName,name)==0) &&
		    (mEmitters[i].mNumber==num))
		{
			return(mEmitters[i].mMeshPart);
		}
	}
	
	if (mNextInChain)
		return(mNextInChain->GetEmitterSubpart(name,num));
	else
		return(NULL);
}


//******************************************************************************************

CMESHPART	*CMesh::GetMeshSubpart(char *name)
{
	for (int i=0;i<mNumParts;i++)
	{
		if (stricmp(mParts[i]->mName,name)==0)
		{
			return(mParts[i]);
		}
	}
	return NULL;
}

//******************************************************************************************
// Calculates the base positions and orientations for all mesh parts
//******************************************************************************************

void CMesh::CalculateBases()
{
	if (mParts)
	{
		for (int i=0;i<mNumParts;i++)
		{
			mParts[i]->GetWorldTransforms(0,0,0,NULL,&mParts[i]->mBaseOrientation,&mParts[i]->mBasePosition,true,false);
		}
	}
}

//******************************************************************************************
#ifndef SIMPLE_MESHES
#ifdef RESBUILDER
void	CMesh::AccumulateResources(CResourceAccumulator * accumulator,DWORD flags)
{
	accumulator->AddMesh(this,flags);

	int i;
	for( i=0; i<mNumTextures; ++i )
	{
		mTextures[ i ].AccumulateResources( accumulator,flags );
	}

	if (mNextInChain)
		mNextInChain->AccumulateResources(accumulator,flags);
}
#endif
#endif

//******************************************************************************************
void	CMesh::BuildAllPolyBuckets()
{
	if (mParts)
	{
		for (int i=0;i<mNumParts;i++)
		{
			mParts[i]->BuildPolyBucket();
		}
	}
}


//******************************************************************************************
FVector  CMesh::GetRandomVertexPos(CMeshPart*& part)
{
#ifdef SIMPLE_MESHES
	return FVector(0, 0, 0);
#else
	int count = 0 ;
	if (mNumParts == 0)
	{
		LOG.AddMessage("WARNING: trying to get random vertex pos on a mesh that has no vertices") ;
		return FVector(0.0f,0.0f,0.0f) ;
	}

	if (mNumParts<16)
	{
		// Quick and dirty
		
		CMeshPart *dp;
		
		int giveupcounter=15;
		
		do
		{
			int partnumber=GAME.Random() % mNumParts;
			
			part=mParts[partnumber];
			dp=part->GetMeshDataPart();
			giveupcounter--;
		} while (((!dp) || (dp->mPartType!=MESHPART_TYPE_MESH)) && (giveupcounter>0));

		if (giveupcounter==0)
			return FVector(0,0,0);
		else
			return dp->GetRandomVertexPos();
	}
	else
	{
		// ok work pick a random mesh part.  note that bigger mesh parts have high chance of being picked
		
		SINT s0;
		
		CMeshPart* mp_ar[150];
		float mp_size[150] ;
		int found_mp =0 ;
		float total_size = 0;
		
		for (s0=0;s0< mNumParts;s0++)
		{
			part = mParts[s0] ;
			if (part->GetMeshDataPart() == NULL ||
				part->GetMeshDataPart()->mPartType != MESHPART_TYPE_MESH ||
				part->mBoundingBox == NULL)
			{
				continue ;
			}
			
			if (found_mp<150)
			{
				float radius = part->mBoundingBox->mRadius ;
				mp_ar[found_mp] = part ;
				mp_size[found_mp] = radius ;
				found_mp++;
				total_size+=radius ;
			}
			else
			{
				LOG.AddMessage("WARNING: Not enough mp allocated to calc random mp") ;
			}
			
		}
		
		float tt= 0.0f ;
		for (s0=0;s0< found_mp;s0++)
		{
			mp_size[s0] = mp_size[s0] / total_size ;
			tt+=mp_size[s0];
		}
		
		while (count < 100)
		{
			float rr = GAME.FloatRandom() ;
			for (s0=0;s0< found_mp;s0++)
			{
				SINT s0 = GAME.Random() % found_mp;
				part = mp_ar[s0] ;
				if (mp_size[s0] >= rr)
				{
					return part->GetRandomVertexPos() ;
				}
			}
			count++;
		}
		
		// if can't find a part return part 0
		if (found_mp >0)
		{
			return mp_ar[0]->GetRandomVertexPos() ;
		}
		
		LOG.AddMessage("WARNING: trying to get random vertex pos on a mesh that has no vertices") ;
		return FVector(0.0f,0.0f,0.0f) ;
	}
#endif
}


//******************************************************************************************
#ifndef SIMPLE_MESHES
#ifdef RESBUILDER
static bool name_is(const char *name, const char *tester)
{
	return !strnicmp(name, tester, strlen(tester));
}

void CMesh::Serialize(class CChunker *c,CResourceAccumulator *ra,DWORD flags)
{
	mResourceID=ra->GetResourceID();
	
	char buffer[256];
	sprintf(buffer,"Serializing mesh %s with resource ID %d\n",mName,mResourceID);
	TRACE(buffer);
	
	SASSERT((void *) &mTextures==(void *) this,"It looks like you've added a vtable to CMesh! This needs special code in Serialize()/Deserialize()!");

	BYTE *temp=new (MEMTYPE_SCRATCHPAD) BYTE[sizeof(CMESH)];
	CMESH *t=(CMESH *) temp;
	memcpy(t,this,sizeof(CMESH));

	// Clear all pointers

	SERIALIZE_CLEAR(t->mAnimModes);
	SERIALIZE_CLEAR(t->mEmitters);
	SERIALIZE_CLEAR(t->mNextInChain);
	SERIALIZE_CLEAR(t->mNextMesh);
	SERIALIZE_CLEAR(t->mParts);
	SERIALIZE_CLEAR(t->mSequences);
	SERIALIZE_CLEAR(t->mTextures);

	// let's check the names against a couple of known meshes
	extern bool allowed_to_compress;
	bool old_compress = allowed_to_compress;
	if (name_is(mName, "boss_gill-m") || name_is(mName, "m_thunder"))
		allowed_to_compress = false;

	if (ra->GetTargetPlatform()==PS2) // Chained meshes are only used for LOD, so ignore on PS2 as we can't afford the RAM
	{
		t->mNextInChain=NULL;
	}

	int i;

	c->Start(MKID("PMSH"));

	c->Write(mName,sizeof(mName),1);

	UINT optional=0;
	if ((flags & RES_OPTIONAL)!=0)
		optional=1;

	c->Write(&optional,sizeof(optional),1);
	
	c->Start(MKID("CMSH"));
	
	c->Write(t,sizeof(CMESH),1);
	
	c->End();

	delete temp;

	// Textures

	c->Start(MKID("CMST"));	
	c->Write(mTextures,sizeof(CMeshTexture),mNumTextures);
	c->End();

	for (i=0;i<mNumTextures;i++)
	{
		c->Start(MKID("MSHT"));

		c->Start(MKID("TEXB"));
		c->Write(mTextures[i].mTextureBlend,sizeof(float),mTextures[i].mFrames);
		c->Write(mTextures[i].mTextureU,sizeof(float),mTextures[i].mFrames);
		c->Write(mTextures[i].mTextureV,sizeof(float),mTextures[i].mFrames);
		c->Write(mTextures[i].mTextureUScale,sizeof(float),mTextures[i].mFrames);
		c->Write(mTextures[i].mTextureVScale,sizeof(float),mTextures[i].mFrames);
		
		if (mTextures[i].mTexture->mResourceID<0)
		{
			SASSERT(0,"Attempt to serialise CMeshTexture reference to unserialised texture!");	
		}
		
		int resid=mTextures[i].mTexture->mResourceID;
		c->Write(&resid,sizeof(resid),1);
		c->End();

		c->End();
	}

	// Parts
	
	for (i=0;i<mNumParts;i++)
	{
		mParts[i]->Serialize(c,ra);
	}

	// Sequences

	if (mNumSequences>0)
	{
		c->Start(MKID("CCUS"));	
		c->Write(mSequences,sizeof(CCutsceneSequence),mNumSequences);
		c->End();

		for (i=0;i<mNumSequences;i++)
		{
			if (mSequences[i].mCamera)
			{	
				int campart=mSequences[i].mCamera->mPartNumber;
				c->Start(MKID("CAMP"));
				c->Write(&campart,sizeof(campart),1);
				c->End();
			}
		}
	}


	// Anim modes

	if (mNumAnimModes>0)
	{
		c->Start(MKID("CAMD"));	
		c->Write(mAnimModes,sizeof(CAnimMode),mNumAnimModes);
		c->End();
	}

	// Bounding box

	c->Start(MKID("BBOX"));	
	mBoundingBox->Serialize(c,ra);
	c->End();	

	// Emitters

	if (mNumEmitters>0)
	{
		c->Start(MKID("CEMT"));	
	
		int esize=sizeof(CEmitter);

		c->Write(&esize,sizeof(esize),1);

		for (i=0;i<mNumEmitters;i++)
		{
			c->Write(&mEmitters[i],sizeof(CEmitter),1);

			if (mEmitters[i].mMeshPart)
			{	
				int part=mEmitters[i].mMeshPart->mPartNumber;
				c->Write(&part,sizeof(part),1);
			}			
		}

		c->End();
	}

	// Next chained mesh

	if (ra->GetTargetPlatform()!=PS2) // Chained meshes are only used for LOD, so ignore on PS2 as we can't afford the RAM
	{
		if (mNextInChain)
			mNextInChain->Serialize(c,ra,flags);
	}
	
	c->End();

	allowed_to_compress = old_compress;
}
#endif
//******************************************************************************************

CMESH *CMesh::Deserialize(class CChunkReader *c)
{
	CMESH *n=new (MEMTYPE_MESH) CMESH;
	
	// Let's not delete this bounding box jus now. But NULL it for paranoia's sake.
	CBoundingBox *old_bbox = n->mBoundingBox;
	n->mBoundingBox = NULL;

	//CMESH *originalmesh;
	int i;

	// Save pointers
	CMESH *next=n->mNextMesh;

	if (c->GetNext()!=MKID("PMSH"))
	{
		SASSERT(0,"CMesh deserialize format failure!");
	}

	char name[MESH_NAME_LEN];
	c->Read(&name,sizeof(name),1);
	UINT optional;
	c->Read(&optional,sizeof(optional),1);

	if (optional==1)
	{
		// Check if we want to load this mesh...
		if (!GAME.DoWeWantMesh(name))
		{
			char buf[256];
			sprintf(buf,"Skipping deserialisation of mesh %s as it is unnecessary\n",name);
			TRACE(buf);
			c->Skip();

			// we'd better not leak this!
			SAFE_DELETE(old_bbox);

			return(NULL);
		}
	}
	
#if TARGET == PS2
	char buf[256];
	sprintf(buf,"Deserialising mesh %s...\n",name);
	TRACE(buf);
#endif

	if (c->GetNext()!=MKID("CMSH"))
	{
		SASSERT(0,"CMesh deserialize format failure!");
	}

	if (c->GetSize()!=sizeof(CMesh))
	{
		SASSERT(0,"CMesh deserialize format failure!");
	}
	
	c->Read(n,sizeof(CMesh),1);

	// Restore pointers (needs to be done now to avoid the mesh list being in an odd state)
	
	n->mNextMesh=next;	

	// Allocate memory

	if (n->mNumAnimModes>0)
		n->mAnimModes = new( MEMTYPE_MESH ) CAnimMode[n->mNumAnimModes];
	else
		n->mAnimModes=NULL;

	if (n->mNumEmitters>0)
		n->mEmitters = new (MEMTYPE_MESH) CEmitter[n->mNumEmitters];
	else
		n->mEmitters=NULL;

	static int emittertotal=0;
	emittertotal+=sizeof(CEmitter)*n->mNumEmitters;
	printf("%dK total in emitters so far\n",emittertotal/1024);	

	if (n->mNumTextures>0)
		n->mTextures = new (MEMTYPE_MESH) CMeshTexture[n->mNumTextures];
	else
		n->mTextures=NULL;

	if (n->mNumSequences>0)
		n->mSequences = new (MEMTYPE_MESH) CCutsceneSequence[n->mNumSequences];
	else
		n->mSequences=NULL;

	// Textures
	
	if (c->GetNext()!=MKID("CMST"))
	{
		SASSERT(0,"CMeshTexture deserialize format failure!");
	}
	
	c->Read(n->mTextures,sizeof(CMeshTexture),n->mNumTextures);

	for (i=0;i<n->mNumTextures;i++)
	{
		if (c->GetNext()!=MKID("MSHT"))
		{
			SASSERT(0,"CMeshTexture deserialize format failure!");
		}

		n->mTextures[i].mTextureBlend=NULL;
		n->mTextures[i].mTextureU=NULL;
		n->mTextures[i].mTextureV=NULL;
		n->mTextures[i].mTextureUScale=NULL;
		n->mTextures[i].mTextureVScale=NULL;

		n->mTextures[i].mVBuffer=NULL;
		
		n->mTextures[i].SetNumFrames(n->mTextures[i].mFrames);

		if (c->GetNext()!=MKID("TEXB"))
		{
			SASSERT(0,"CMeshTexture deserialize format failure!");
		}
		c->Read(n->mTextures[i].mTextureBlend,sizeof(float),n->mTextures[i].mFrames);
		
		c->Read(n->mTextures[i].mTextureU,sizeof(float),n->mTextures[i].mFrames);
		
		c->Read(n->mTextures[i].mTextureV,sizeof(float),n->mTextures[i].mFrames);
		
		c->Read(n->mTextures[i].mTextureUScale,sizeof(float),n->mTextures[i].mFrames);
		
		c->Read(n->mTextures[i].mTextureVScale,sizeof(float),n->mTextures[i].mFrames);
		
		int resid;
		c->Read(&resid,sizeof(resid),1);
		
		n->mTextures[i].mTexture=CTEXTURE::GetTextureByResourceID(resid);
		
		SASSERT(n->mTextures[i].mTexture,"CMeshTexture referenced unrecognised texture whilst deserializing!");

		n->mTextures[i].mTexture->AddRef();

		n->mTextures[i].mVBuffer=NULL;

		n->mTextures[i].SetupVBuffer();
	}

	// Create all the parts first (as they may reference each other)

	n->mParts = new( MEMTYPE_MESH ) CMESHPART*[n->mNumParts];

#if TARGET == XBOX
	n->mParts[0] = new (MEMTYPE_MESH) CMESHPART[n->mNumParts];
	for (i=1;i<n->mNumParts;i++)
		n->mParts[i]=n->mParts[0] + i;
#else
	for (i=0;i<n->mNumParts;i++)
		n->mParts[i]=new (MEMTYPE_MESH) CMESHPART;
#endif
	
	// Deserialize parts
	
	for (i=0;i<n->mNumParts;i++)
	{
		CMeshPart::Deserialize(c,n->mParts[i],n);
	}
	
	// Sequences

	if (n->mNumSequences>0)
	{
		if (c->GetNext()!=MKID("CCUS"))
		{
			SASSERT(0,"CCutsceneSequence deserialize format failure!");	
		}
		
		c->Read(n->mSequences,sizeof(CCutsceneSequence),n->mNumSequences);

		for (i=0;i<n->mNumSequences;i++)
		{
			if (n->mSequences[i].mCamera)
			{
				int campart;
				
				if (c->GetNext()!=MKID("CAMP"))
				{
					SASSERT(0,"CCutsceneSequence deserialize format failure!");
				}
				
				c->Read(&campart,sizeof(campart),1);
				
				n->mSequences[i].mCamera=n->GetPart(campart);
			}
		}
	}
	
	// Anim modes
	
	if (n->mNumAnimModes>0)
	{
		if (c->GetNext()!=MKID("CAMD"))
		{
			SASSERT(0,"CAnimMode deserialize format failure!");
		}
		
		c->Read(n->mAnimModes,sizeof(CAnimMode),n->mNumAnimModes);
	}

	// Bounding box. We've carefully saved ourselves some memory
	if (c->GetNext()!=MKID("BBOX"))
	{
		SASSERT(0,"CMesh deserialize format failure!");
	}

	n->mBoundingBox=CBoundingBox::Deserialize(c, old_bbox);
			
	// Emitters
	
	if (n->mNumEmitters>0)
	{
		if (c->GetNext()!=MKID("CEMT"))
		{
			SASSERT(0,"CEmitter deserialize format failure!");
		}

		int esize;
		c->Read(&esize,sizeof(esize),1);

		int correctedsize=esize;
#if TARGET==PS2
		while ((correctedsize & 0xF)!=0)
			correctedsize++;
#endif
		
		if (correctedsize!=sizeof(CEmitter))
		{
			printf("Size=%d, sizeof=%d\n",esize,sizeof(CEmitter));
			SASSERT(0,"CEmitter deserialize format failure!");
		}
				
		for (i=0;i<n->mNumEmitters;i++)
		{
			c->Read(&n->mEmitters[i],esize,1);

			if (n->mEmitters[i].mMeshPart)
			{
				int part;
				c->Read(&part,sizeof(part),1);
				n->mEmitters[i].mMeshPart=n->GetPart(part);
			}
		}
	}
	
	// Next chained mesh
	
	if (n->mNextInChain)
	{
		n->mNextInChain=CMesh::Deserialize(c);
		n->mNextInChain->AddRef();
	}

/*	char buffer[256];
	sprintf(buffer,"Loaded mesh %s from resource file\n",n->mName);
	TRACE(buffer);*/

	// Clear data

	n->mRefCount=0;	

	// Code to test mesh loading
/*		
	if (strlen(n->mName)>0)
	{
		char name[256];
		strcpy(name,n->mName);
		
		strcat(n->mName,"-res");
		
		originalmesh=CMESH::GetMesh(name);

		printf("Originalmesh=%x\n",originalmesh);

		printf("Loaded %s\n",originalmesh->mName);

		CMESH *omesh=originalmesh;
		CMESH *nmesh=n;

		while ((omesh) && (nmesh))
		{
			SASSERT(omesh->mNumTextures==nmesh->mNumTextures,"NumTextures mismatch");

			int i;
			for (i=0;i<nmesh->mNumTextures;i++)
			{
				if (nmesh->mTextures[i].mTexture!=omesh->mTextures[i].mTexture)
					SASSERT(0,"Texture mismatch!");

				SASSERT(omesh->mTextures[i].mFrames==nmesh->mTextures[i].mFrames,"Frames mismatch")

				for (int j=0;j<omesh->mTextures[i].mFrames;j++)
				{
					SASSERT(omesh->mTextures[i].mTextureBlend[j]==nmesh->mTextures[i].mTextureBlend[j],"Blend mismatch");
					SASSERT(omesh->mTextures[i].mTextureU[j]==nmesh->mTextures[i].mTextureU[j],"U mismatch");
					SASSERT(omesh->mTextures[i].mTextureV[j]==nmesh->mTextures[i].mTextureV[j],"V mismatch");
					SASSERT(omesh->mTextures[i].mTextureUScale[j]==nmesh->mTextures[i].mTextureUScale[j],"U scale mismatch");
					SASSERT(omesh->mTextures[i].mTextureVScale[j]==nmesh->mTextures[i].mTextureVScale[j],"V scale mismatch");
				}
			}

			SASSERT(omesh->mNumParts==nmesh->mNumParts,"NumParts mismatch");

			for (i=0;i<nmesh->mNumParts;i++)
			{
				SASSERT(omesh->mParts[i]->mNumDVertices==nmesh->mParts[i]->mNumDVertices,"NDVerts mismatch");

				for (int j=0;j<nmesh->mParts[i]->mNumDVertices;j++)
				{
					SASSERT(omesh->mParts[i]->mDVertices[j].mPVert==nmesh->mParts[i]->mDVertices[j].mPVert,"PVert mismatch");
					for (int k=0;k<TEXTURE_TYPES;k++)
						SASSERT(omesh->mParts[i]->mDVertices[j].mTexture[k]==nmesh->mParts[i]->mDVertices[j].mTexture[k],"DVert texture mismatch");
				}
			}

			omesh=omesh->mNextInChain;
			nmesh=nmesh->mNextInChain;
		}

		SASSERT(omesh==nmesh,"Chain mismatch!");
		SASSERT(omesh==NULL,"Chain mismatch!");

		originalmesh->Release();
		originalmesh->ForceDelete();
	}*/

	return(n);
}
#endif

//******************************************************************************************
#ifdef LINK_EDITOR
void	CMesh::AccumulateResources(CTextureAccumulator &ta, CMeshAccumulator &ma)
{
	SASSERT(0,"Don't call CMesh::AccumulateResources! Ask Ben why!"); // DON'T CALL THIS!

	// add the mesh
	ma.AddMesh(this);

	// add all textures

	SINT	c0, c1;

	if (mParts)
	{
		for (c0 = 0; c0 < mNumParts; c0 ++)
		{
			if (mParts[c0])
			{
				for (c1 = 0; c1 < mParts[c0]->mNumDVertices; c1 ++)
				{
					SINT	c2;
					for (c2 = 0; c2 < mNumSkins; c2 ++)
						ta.AddTexture(mParts[c0]->mDVertices[c1].mTexture[c2], TATT_MESH);
				}
			}
		}
	}
}
#endif

//******************************************************************************************
FVector	CMesh::TransformLocalToWorld(FVector pos, IRenderableThing *t, CMESHPART *mp,  BOOL include_offset)
{
	// Convert a local coordinate to world mesh coords.
	FMatrix	lori;

	while (mp)
	{
		lori  = mp->mOrientation;
		if (t)
		{
#ifndef SIMPLE_MESHES
			CMotionController *c=t->GetMotionController();
			if (c)
				c->UpdateRenderPositionPre(mp,NULL,&lori,true);
#endif
/*			FMatrix	m = t->GetRenderOrientation(mp->mPartNumber);
			lori = lori * m;*/
		}

		pos  = lori * pos;

		if (include_offset)
		{
			pos += mp->mOffsetPos;  // suspension??
		}

#ifndef SIMPLE_MESHES
		if (t)
		{
			CMotionController *c=t->GetMotionController();
			if (c)
			{
				FVector dummy ;
				c->UpdateRenderPositionPost(mp,&dummy,&lori,true);
			}
		}
#endif

		if (t && (!t->RenderUseHierarchy(mp->mPartNumber)))
			break; // we're done here

		mp = mp->mParent;
	}

	return pos;
}



//******************************************************************************************
FVector	CMesh::TransformWorldToLocal(FVector pos, IRenderableThing *t, CMESHPART *mp, BOOL include_offset)
{
	// Convert a world coordinate to local mesh coordinates

	// needs to be done recursively - top to bottom.
	if (mp->mParent)
	{
		if ((!t) || (t->RenderUseHierarchy(mp->mPartNumber)))  // maybe not use heirarchy
			pos = TransformWorldToLocal(pos, t, mp->mParent, include_offset);
	}



	FMatrix	lori = mp->mOrientation;
	if (t)
	{
#ifndef SIMPLE_MESHES
		CMotionController *c=t->GetMotionController();
		if (c)
		{
			FVector dummy ;
			c->UpdateRenderPositionPre(mp,&dummy,&lori,true);
		}
#endif
/*		FMatrix	m = t->GetRenderOrientation(mp->mPartNumber);
		lori = lori * m;*/
	}

	if (include_offset)
	{
		pos -= mp->mOffsetPos;
	}
	pos = lori.Transpose() * pos;

#ifndef SIMPLE_MESHES
	if (t)
	{
		CMotionController *c=t->GetMotionController();
		if (c)
		{
			FVector dummy ;
			c->UpdateRenderPositionPost(mp,&dummy,&lori,true);
		}

		
	}
#endif

	return pos;
}


//******************************************************************************************
BOOL	CMesh::SimpleWrite(char *fname)
{
	CMEMBUFFER	mem;

	if (!mem.InitFromMem(fname))
		return FALSE;

	DWORD	id;
	SINT	i, c0, c1, c2;
	
	id = MKID("MESH");
	mem.Write(&id, 4);

	id = MKID("2.07");
	mem.Write(&id, 4);

	// Amount of global data
	i = 4;
	mem.Write(&i, 4);

	i = 1;
	mem.Write(&i, 4);

	// Textures
	mem.Write(&mNumTextures, 4);
	char	tname[256];
	for (c0 = 0; c0 < mNumTextures; c0 ++)
	{
		memset(tname, 0, 256);
		if (mTextures[c0].mTexture)
		{
			// scan for \\'s
			c1 = 0;
			char *c = mTextures[c0].mTexture->GetName();
			while (*c)
			{
				if (c[0] == '\\')
					c1 = c - mTextures[c0].mTexture->GetName() + 1;
				c++;
			};
			strcpy(tname, &mTextures[c0].mTexture->GetName()[c1]);
		}

		mem.Write(tname, 256);

		i = 1;
		mem.Write(&i, 4);

		float a,u,v,us,vs;
		a = 1.f;
		u = 0.f;
		v = 0.f;
		us = 1.f;
		vs = 1.f;

		mem.Write(&a, 4);
		mem.Write(&u, 4);
		mem.Write(&v, 4);
		mem.Write(&us, 4);
		mem.Write(&vs, 4);	
	}

	// Mesh Parts
	mem.Write(&mNumParts, 4);

	for (c0 = 0; c0 < mNumParts; c0 ++)
	{
		CMESHPART *p = mParts[c0];

		ASSERT(p);

		// start
		mem.Write(&p->mPartType, 4);
		mem.Write(&p->mName,sizeof(p->mName));

		// Numbers
		mem.Write(&p->mNumDVertices, 4);
		mem.Write(&p->mNumPVertices, 4);
		mem.Write(&p->mNumVFrames, 4);
		mem.Write(&p->mNumAFrames, 4);
		mem.Write(&p->mNumTriangles, 4);
		mem.Write(&p->mNumHFrames, 4);

		// VFrame -> HFrame Map
		ASSERT(p->mVHFrameMap);
		mem.Write(&p->mVHFrameMap, p->mNumVFrames * 1);

		// HFrames
		for (c1 = 0; c1 < p->mNumHFrames; c1 ++)
		{
			FVector		v  = FVector(0, 0, 0);
			v.W = 1;
			FVector3	v2 = FVector3(1, 1, 1);

			mem.Write(&v , 12);
			mem.Write(&v , 16);
			mem.Write(&v2, 12);
			mem.Write(&v , 16);
		}

		// Children
		mem.Write(&p->mNumChildren, 4);

		for (c1 = 0; c1 < p->mNumChildren; c1 ++)
			mem.Write(&p->mChildren[c1], 4);

		if (!p->mParent)
		{
			i = -1;
			mem.Write(&i, 4);
		}
		else
			mem.Write(&p->mParent->mPartNumber, 4);

		// DVertices
		ASSERT(p->mDVertices);
		for (c1 = 0; c1 < p->mNumDVertices; c1 ++)
		{
			CMeshDVertex *dv = &p->mDVertices[c1];

			mem.Write(&dv->mPVert, 4);
			mem.Write(&dv->mU, 4);
			mem.Write(&dv->mV, 4);
			mem.Write(&dv->mDiffuse, 4);
			mem.Write(&dv->mTexture[0], 4);
		}

		// VFrames
/*		ASSERT(p->mVFrames);
		mem.Write(p->mVFrames, p->mNumVFrames * 4);*/

		// PVerts
		for (c1 = 0; c1 < p->mNumAFrames; c1 ++)
			for (c2 = 0; c2 < p->mNumPVertices; c2 ++)
			{
				mem.Write(&p->mPVertices[c1][c2], 12);
			}

		// Tris
		ASSERT(p->mTriangles);
		
		for (c1 = 0; c1 < p->mNumTriangles; c1 ++)
		{
			CMeshTriangle *tri = &p->mTriangles[c1];

			// B & C flipped...

			i = tri->mA - p->mDVertices;
			mem.Write(&i, 4);
			i = tri->mC - p->mDVertices;
			mem.Write(&i, 4);
			i = tri->mB - p->mDVertices;
			mem.Write(&i, 4);

			mem.Write(&tri->mA->mNormal, 12);
			mem.Write(&tri->mC->mNormal, 12);
			mem.Write(&tri->mB->mNormal, 12);
		}

		// Collision volumes
		i = 0;
		mem.Write(&i, 4);
	}

	// Emitters
	i = 0;
	mem.Write(&i, 4);

	// Anim Modes
	i = 0;
	mem.Write(&i, 4);

	// Cutscene Sequences
	i = 0;
	mem.Write(&i, 4);

	if (!mem.Close())
		return FALSE;

	return TRUE;
}

//******************************************************************************************

CMESH *CMesh::GetMeshByResourceID(int resid)
{
	CMESH *c=mFirstMesh;

	while (c)
	{
		if (c->mResourceID==resid)
			return(c);

		c=c->mNextMesh;
	}

	return(NULL);
}

//******************************************************************************************

int rfsaved=0;

void CMesh::ReduceFrames()
{
#if TARGET != XBOX
	char buf[256];
	int i;

	sprintf(buf,"Attempting to reduce mesh %s from %d frames\n",mName,mParts[0]->mNumVFrames);
	TRACE(buf);

	// First check if mesh has more frames than marked as animation

	int endofanim=1;

	for (i=0;i<mNumAnimModes;i++)
	{
		if (mAnimModes[i].mLast>endofanim)
			endofanim=mAnimModes[i].mLast;
	}

	if (endofanim<mParts[0]->mNumVFrames)
	{
		sprintf(buf,"Bakabaka! Mesh has more frames than animation ^-^;;\n");
		TRACE(buf);
		sprintf(buf,"%d frames, animation ends at %d\n",mParts[0]->mNumVFrames,endofanim);
		TRACE(buf);
		for (int i=0;i<mNumParts;i++)
			mParts[i]->mNumVFrames=endofanim;
	}

	BOOL ok=TRUE;

	float threshold=0.5f; //0.05f;
	int minframes;
	
	minframes=CMotionController::CheckMinFrames(this);

	// Check for skeletal animation

	for (i=0;i<mNumParts;i++)
	{
		if (mParts[i]->mPartType==MESHPART_TYPE_SKELETAL)
			minframes=64;
	}

	sprintf(buf,"Minimum frames suggested = %d\n",minframes);
	TRACE(buf);

	if (minframes==-1)
		minframes=16;

	if (mNumAnimModes==0)
		minframes=1;

	int droppedhframes=0;
		
	while (ok)
	{
		for (i=1;i<(mParts[0]->mNumVFrames-1);i+=2)
		{
			// See if each part is roughly where we expect it to be
			
			for (int j=0;(j<mNumParts) && (ok);j++)
			{
				FMatrix actualori,predictedori,ori1,ori2;
				FVector actualpos,predictedpos,pos1,pos2;
				
				mParts[j]->GetWorldTransforms(i,i,0.0f,NULL,&actualori,&actualpos,TRUE,FALSE,TRUE);
				mParts[j]->GetWorldTransforms(i-1,i-1,0.0f,NULL,&ori1,&pos1,TRUE,FALSE,TRUE);
				mParts[j]->GetWorldTransforms(i+1,i+1,0.0f,NULL,&ori2,&pos2,TRUE,FALSE,TRUE);
				
				predictedpos=pos1+((pos2-pos1)*0.5f);
				predictedori=ori1+((ori2-ori1)*0.5f);

				// The bounding box won't have been built yet

				mParts[j]->GenerateSimpleBoundingBox();
				mParts[j]->mBoundingBox->CalculateRadius();

				float rad=0.1f;
				if (mParts[j]->mBoundingBox->mRadius>0.0f)
					rad=mParts[j]->mBoundingBox->mRadius;
				float pthreshold=threshold/rad;
				
				FVector posdifference=predictedpos-actualpos;
				FMatrix oridifference=predictedori-actualori;
				
				if ((fabsf(posdifference.X)>pthreshold) ||
					(fabsf(posdifference.Y)>pthreshold) ||
					(fabsf(posdifference.Z)>pthreshold))
					ok=FALSE;
				
				if ((fabsf(oridifference.Row[0].X)>pthreshold) ||
					(fabsf(oridifference.Row[0].Y)>pthreshold) ||
					(fabsf(oridifference.Row[0].Z)>pthreshold) ||
					(fabsf(oridifference.Row[1].X)>pthreshold) ||
					(fabsf(oridifference.Row[1].Y)>pthreshold) ||
					(fabsf(oridifference.Row[1].Z)>pthreshold) ||
					(fabsf(oridifference.Row[2].X)>pthreshold) ||
					(fabsf(oridifference.Row[2].Y)>pthreshold) ||
					(fabsf(oridifference.Row[2].Z)>pthreshold))
					ok=FALSE;
			}
		}

		if (mNumAnimModes==0)
			ok=TRUE;

		if ((mParts[0]->mNumVFrames/2)<minframes)
			ok=FALSE;

		if (ok)
		{
			sprintf(buf,"Reducing from %d to %d frames\n",mParts[0]->mNumVFrames,mParts[0]->mNumVFrames/2);
			TRACE(buf);

			for (i=0;i<mNumParts;i++)
			{
				int j;

				for (j=0;j<mParts[i]->mNumVFrames;j++)
				 mParts[i]->mVHFrameMap[(j/2)]=mParts[i]->mVHFrameMap[j];
				mParts[i]->mNumVFrames=mParts[i]->mNumVFrames/2;

				// Eliminate unused HFrames

				for (j=0;j<mParts[i]->mNumHFrames;j++)
				{
					BOOL used=FALSE;
					for (int k=0;(k<mParts[i]->mNumVFrames) && (!used);k++)
						if (((int) mParts[i]->mVHFrameMap[k])==j)
							used=TRUE;

					if (!used)
					{
						// Remove redundant HFrame

						for (int k=j+1;k<mParts[i]->mNumHFrames;k++)
						{
							// Shift down

							if (mParts[i]->mHFrameFOVs)
								mParts[i]->mHFrameFOVs[k-1]=mParts[i]->mHFrameFOVs[k];
							mParts[i]->mHFrameOrientations[k-1]=mParts[i]->mHFrameOrientations[k];
							mParts[i]->mHFramePositions[k-1]=mParts[i]->mHFramePositions[k];

							// Remap
							
							for (int l=0;l<mParts[i]->mNumVFrames;l++)
							{
								if (((int) mParts[i]->mVHFrameMap[l])==k)
									SASSERT( k>=0 && k<256, "VHFrameMap out of bounds!!" );
									mParts[i]->mVHFrameMap[l]=(UBYTE) k-1;
							}
						}

						mParts[i]->mNumHFrames--;

						if (mParts[i]->mHFrameFOVs)
							rfsaved+=sizeof(float);
						rfsaved+=sizeof(FMatrix)+sizeof(FVector);
						droppedhframes++;
					}
				}
			}

			for (i=0;i<mNumAnimModes;i++)
			{
				mAnimModes[i].mStart=mAnimModes[i].mStart/2;
				mAnimModes[i].mLast=mAnimModes[i].mLast/2;
				mAnimModes[i].mFrameScaler=mAnimModes[i].mFrameScaler/2;
				mAnimModes[i].mRate=mAnimModes[i].mRate/2.0f;
			}

			rfsaved+=(mNumParts*mParts[0]->mNumVFrames*sizeof(float)); // VHFrameMap saving
		}
	}

	sprintf(buf,"Dropped %d HFrames\n",droppedhframes);
	TRACE(buf);	

	sprintf(buf,"ReduceFrames() saved %dK\n",rfsaved/1024);
	TRACE(buf);

	TRACE("Done\n");
#endif
}

//******************************************************************************************

void CMesh::ReducePolys()
{
	char buf[256];
	sprintf(buf,"Reducing mesh %s polys\n",mName);
	TRACE(buf);

	for (int i=0;i<mNumParts;i++)
	{
		mParts[i]->ReducePolys();
	}

	TRACE("Done\n");
}

//******************************************************************************************

int totalparts=0;
int removedparts=0;

void CMesh::OptimiseParts()
{
	SASSERT(TARGET != XBOX, "This can't be used on the xbox because the mesh format is too simple");

#if TARGET != XBOX
#ifndef SIMPLE_MESHES

	int i,j,k;

	totalparts+=mNumParts;

	char buf[256];
	sprintf(buf,"Optimising mesh %s parts\n",mName);
	TRACE(buf);

	// Collapse mesh parts into one another

	for (i=0;i<mNumParts;i++)
	{
		for (j=0;j<mNumParts;j++)
		{
			if ((mParts[i]->mPartType==MESHPART_TYPE_MESH) && (mParts[j]->mPartType==MESHPART_TYPE_MESH) && (i!=j))
			{
				BOOL oktomerge=TRUE;

/*				char buf[256];
				sprintf(buf,"Testing merge of %s (%d) into %s (%d)\n",mParts[j]->mName,mParts[j]->mNumHFrames,mParts[i]->mName,mParts[i]->mNumHFrames);
				TRACE(buf);*/

				// Are these parts static relative to one another?

				if ((mParts[i]->mNumHFrames!=1) && (mParts[j]->mNumHFrames!=1))
				{
					// Not static themselves, so nope...
					oktomerge=FALSE;
				}
				else
				{
					// Can we find a common parent of I and J with a static path to both?

					oktomerge=FALSE;					

					CMeshPart *mpi=mParts[i];

					while (mpi)
					{
						CMeshPart *mpj=mParts[j];

						while (mpj)
						{
							if (mpi==mpj)
								oktomerge=TRUE;

							if (!CMotionController::CheckCanBeMergedInto((CMESHPART *) mpj))
								mpj=NULL;
							else if (mpj->mNumHFrames>1) // Stop looking after we've examined one non-static node
								mpj=NULL;
							else
								mpj=mpj->mParent;
						}

						if (!CMotionController::CheckCanBeMergedInto((CMESHPART *) mpi))
							mpi=NULL;
						else if (mpi->mNumHFrames>1) // Stop looking after we've examined one non-static node
							mpi=NULL;
						else
							mpi=mpi->mParent;
					}
/*
					if (!oktomerge)
					{
						char buf[256];
						sprintf(buf,"Merge of %s into %s failed - no common static parent\n",mParts[j]->mName,mParts[i]->mName);
						TRACE(buf);
					}*/
				}

				// Are these part of a substitute chain?

				if (mParts[i]->mIsChained)
					oktomerge=FALSE;
				if (mParts[j]->mIsChained)
					oktomerge=FALSE;

				// Are they vertex-animated?

				if (mParts[i]->mNumAFrames!=1)
					oktomerge=FALSE;
				if (mParts[j]->mNumAFrames!=1)
					oktomerge=FALSE;

				// Are they the targets of references or skeletons?

				for (k=0;k<mNumParts;k++)
				{
					if (mParts[k]->mPartType==MESHPART_TYPE_REFERENCE)
					{
						if (mParts[k]->mReferenceTo==mParts[i])
							oktomerge=FALSE;
						if (mParts[k]->mReferenceTo==mParts[j])
							oktomerge=FALSE;
					}
					if (mParts[k]->mPartType==MESHPART_TYPE_SKELETAL)
					{
						for (int l=0;l<mParts[k]->mNumBones;l++)
						{
							if (mParts[k]->mBones[l]==mParts[i])
								oktomerge=FALSE;
							if (mParts[k]->mBones[l]==mParts[j])
								oktomerge=FALSE;
						}
					}
				}

				// What do the motion controllers say?

				if (!CMotionController::CheckCanBeRemoved(mParts[j]))
					oktomerge=FALSE;

				if (!CMotionController::CheckCanBeMergedInto(mParts[i]))
					oktomerge=FALSE;

				// Are they on our list of excluded parts?
				
				if (stricmp(mParts[i]->mName,"Nexus")==0)
					oktomerge=FALSE;

				if (stricmp(mParts[j]->mName,"Nexus")==0)
					oktomerge=FALSE;

				if (oktomerge)
				{
					// Merge the vertices from [j] into [i]
					
					mParts[i]->MergeVertices(mParts[j]);
					
					// [j] is now a dummy part (this will ger removed later)
					
					mParts[j]->mNumPVertices=0;
					mParts[j]->mNumDVertices=0;
					mParts[j]->mNumTriangles=0;
					mParts[j]->mPartType=MESHPART_TYPE_NULL;
				}
			}
		}
	}

	// Remove any unnecessary child parts

	BOOL done=FALSE;

	while (!done)
	{
		done=TRUE;

		for (i=0;i<mNumParts;i++)
		{
			if ((mParts[i]->mPartType==MESHPART_TYPE_NULL) && ((mParts[i]->mParent) || (mParts[i]->mNumChildren==1)))
			{
				// "Dangling" null part - can be removed

				BOOL oktoremove=TRUE;
				
				// Is this part of a substitute chain?
				
				if (mParts[i]->mIsChained)
					oktoremove=FALSE;

				// Is it the targets of a references or skeleton?
				
				for (k=0;k<mNumParts;k++)
				{
					if (mParts[k]->mPartType==MESHPART_TYPE_REFERENCE)
					{
						if (mParts[k]->mReferenceTo==mParts[i])
							oktoremove=FALSE;
					}
					if (mParts[k]->mPartType==MESHPART_TYPE_SKELETAL)
					{
						for (int l=0;l<mParts[k]->mNumBones;l++)
						{
							if (mParts[k]->mBones[l]==mParts[i])
								oktoremove=FALSE;
						}
					}
				}
				
				// Is it one of our list of excluded parts?

				if (stricmp(mParts[i]->mName,"Nexus")==0)
					oktoremove=FALSE;

				// Is it supplying animation for other parts?

				if ((mParts[i]->mNumHFrames>1) && (mParts[i]->mNumChildren>0))
					oktoremove=FALSE;

				// Is it an emitter in disguise? (older meshes don't use MESHPART_TYPE_EMITTER)
				
				for (int k=0;k<mNumEmitters;k++)
				{
					if (mEmitters[k].mMeshPart==mParts[i])
						oktoremove=FALSE;
				}				

				// Check with motion controllers

				if (!CMotionController::CheckCanBeRemoved(mParts[i]))
					oktoremove=FALSE;
				
				if (!mParts[i]->mParent)
					if (CMotionController::CheckAreTransformsImportant(this))
						oktoremove=FALSE;
				
				if (oktoremove)
				{
					char buf[256];
					sprintf(buf,"Removing part %s\n",mParts[i]->mName);
					TRACE(buf);
					
					// Apply position and orientation to all children

					for (j=0;j<mParts[i]->mNumChildren;j++)
					{
						CMeshPart *child=mParts[i]->mChildren[j];
						for (int k=0;k<child->mNumHFrames;k++)
						{
							child->mHFramePositions[k]=mParts[i]->mHFramePositions[0] + (mParts[i]->mHFrameOrientations[0] * child->mHFramePositions[k]);
							child->mHFrameOrientations[k]=mParts[i]->mHFrameOrientations[0] * child->mHFrameOrientations[k];
						}
					}

					// Remove part from children's parent pointer

					for (j=0;j<mParts[i]->mNumChildren;j++)
					{
						mParts[i]->mChildren[j]->mParent=mParts[i]->mParent;
					}

					// Remove part from parent's child list				
					
					CMeshPart *parent=mParts[i]->mParent;
					
					if (parent)
					{
						// Part is not the root node

						for (j=0;j<parent->mNumChildren;j++)
						{
							if (parent->mChildren[j]==mParts[i])
							{
								for (int k=j+1;k<parent->mNumChildren;k++)
									parent->mChildren[k-1]=parent->mChildren[k];
								parent->mNumChildren--;
								j--;
							}
						}
						
						// Add children to parent's child list
						
						CMESHPART **newchildlist=new(MEMTYPE_MESH) CMESHPART *[parent->mNumChildren+mParts[i]->mNumChildren];
						
						for (j=0;j<parent->mNumChildren;j++)
							newchildlist[j]=parent->mChildren[j];
						for (j=0;j<mParts[i]->mNumChildren;j++)
							newchildlist[parent->mNumChildren+j]=mParts[i]->mChildren[j];

						SAFE_DELETE(parent->mChildren);
						parent->mChildren=newchildlist;
						parent->mNumChildren=parent->mNumChildren+mParts[i]->mNumChildren;

						// Delete part
						
						for (j=i+1;j<mNumParts;j++)
						{
							mParts[j-1]=mParts[j];
							mParts[j-1]->mPartNumber--;
						}
					}
					else
					{
						// Part must be the root node

						SASSERT(mParts[i]->mNumChildren==1,"Attempt to remove root node with multiple children!");
						SASSERT(i==0,"Root node is not part 0?!");

						// Make child part the first part

						CMeshPart *temp=mParts[0];

						int childnum=temp->mChildren[0]->mPartNumber;

						mParts[0]=temp->mChildren[0];
						mParts[0]->mPartNumber=0;

						// Shuffle parts down 

						for (j=childnum+1;j<mNumParts;j++)
						{
							mParts[j-1]=mParts[j];
							mParts[j-1]->mPartNumber--;
						}
					}
					
					removedparts++;					
					done=FALSE;
					mNumParts--;
					i--;
				}
			}
		}
	}

	sprintf(buf,"OptimiseParts() removed %d of %d parts so far\n",removedparts,totalparts);
	TRACE(buf);
#endif // SIMPLE_MESHES
#endif // TARGET != XBOX
}