// MemoryManager.cpp: implementation of the CMemoryManager class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
#include "MemoryManager.h"
#include "assert.h"
#include "MemBuffer.h"
#include "Platform.h"
#include "DebugLog.h"
#include "Profile.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "console.h"
#include "cliparams.h"
#if TARGET==PS2
#include <libdev.h>
#endif

// The Intel versions of the game have thread-safe memory management
// JCL - quick and dirty thread-safeness
// And Jan made it slower and safer but probably just as dirty.

#ifdef _DIRECTX // some philosophical point could be made here about intel being _DIRECTX

static void GRAB_MUTEX(BOOL *mutex)
{
	// loop until we get the mutex
	while (1)
	{
		bool busy;
	
		// if mem_manager_mutex is 1, this will leave it as 1 but set busy to 1.
		// if it's zero, it will set it to 1 and set busy to 0
		__asm
		{
			mov eax, mutex
			BTS DWORD PTR [eax], 1
			MOV busy, 0
			ADC busy, 0
		}

		if (busy)
		{
			Sleep(0);
		}
		else
		{
			break;
		}
	}
}

static void RELEASE_MUTEX(BOOL *mutex)
{
	// we'd better have the mutex
	ASSERT(*mutex);

	// release it.
	*mutex = FALSE;
}

// We've got one of those classic constructor/destructor classette things
class CMutexGrabber
{
	BOOL *m_Mutex;

public:
	CMutexGrabber(BOOL *mutex)
	{
		// save it for the destructor.
		m_Mutex = mutex;

		// do the work.
		GRAB_MUTEX(mutex);
	}

	~CMutexGrabber()
	{
		RELEASE_MUTEX(m_Mutex);
	}
};
#define PROTECT_WITH_MUTEX() CMutexGrabber grabber(&m_Mutex)

#else
#define PROTECT_WITH_MUTEX() (1)
#define GRAB_MUTEX(a) {}
#define RELEASE_MUTEX(a) {}
#endif


// #define this to enable memory tags

//#define MEMORY_TAGGING

MemoryTypeData gMemTypeData[] =
{

//		Type,						Name,					Size Limit,

	{	MEMTYPE_GENERIC,			"Generic",				0xffffffff	},
	{	MEMTYPE_MESH,				"Mesh",					0xffffffff	},
	{	MEMTYPE_TEXTURE,			"Texture",				0xffffffff	},
	{	MEMTYPE_TEXTURE_DATA,		"Texture surface",		0xffffffff	},
	{	MT_PHYSICS		,			"Physics",				0xffffffff	},
	{	MT_THING,					"Thing",				0xffffffff	},
	{	MT_UNIT_THING,				"Unit Thing",			0xffffffff	},
	{	MT_TREE_THING,				"Tree",					0xffffffff  },
	{   MT_SQUAD,					"Squad",				0xffffffff  },
	{   MT_INIT_THING,				"Init Thing",			0xffffffff  },
	{	MT_BUBBLE,					"Bubble",				0xffffffff  },
	{	MT_CST,						"CST",					0xffffffff  },
	{   MT_DUNNO_THING,				"Dunno thing?",			0xffffffff  },
	{	MT_ROUND,					"Round",				0xffffffff  },
	{	MT_ROUND_DATA,				"Round Data",			0xffffffff  },
	{	MT_UNIT_DATA,				"Unit Data",			0xffffffff	},
	{	MEMTYPE_PARTICLE,			"Particle",				0xffffffff	},
	{	MEMTYPE_MEMBUFFER,			"MemBuffer",			0xffffffff	},
	{	MEMTYPE_FLEXARRAY,			"FlexArray",			0xffffffff	},
	{	MEMTYPE_CAPTURE,			"Capture",				0xffffffff	},
	{	MEMTYPE_MUSIC,				"Music",				0xffffffff	},
	{	MEMTYPE_BATTLEENGINE,		"Battle Engine",		0xffffffff	},
	{	MEMTYPE_AI,					"AI",					0xffffffff	},
	{	MEMTYPE_GUIDE,				"Guide",				0xffffffff	},
	{	MT_SCRIPT,					"Script",				0xffffffff	},
	{	MT_VM_SCRIPT,				"VM Script",			0xffffffff	},
	{   MT_INST_SCRIPT,             "Script Inst",			0xffffffff  },
	{	MEMTYPE_MOTIONCONTROLLER,	"Motion Controller",	0xffffffff	},
	{	MEMTYPE_CHUNKER,			"Chunker",				0xffffffff	},
	{	MEMTYPE_CUTSCENE,			"Cutscene",				0xffffffff	},
	{	MEMTYPE_EQUIPMENT,			"Equipment",			0xffffffff	},	
	{	MEMTYPE_VBUFTEXTURE,		"VBufTexture",			0xffffffff	},
	{	MEMTYPE_INFLUENCEMAP,		"InfleunceMap",			0xffffffff	},
	{	MEMTYPE_MAP,				"Map",					0xffffffff	},
	{	MEMTYPE_HEIGHTFIELD,		"Heightfield",			0xffffffff	},
	{	MEMTYPE_JCLTEXTURE,			"JCLTexture",			0xffffffff	},
	{	MEMTYPE_MESHTEXTURE,		"MeshTexture",			0xffffffff	},
	{	MEMTYPE_NAVIGATION,			"Navigation",			0xffffffff	},
	{	MEMTYPE_CAMERA,				"Camera",				0xffffffff	},
	{	MEMTYPE_CONTROLLER,			"Controller",			0xffffffff	},
	{	MEMTYPE_PLAYER,				"Player",				0xffffffff	},
	{	MEMTYPE_MESSAGEBOX,			"MessageBox",			0xffffffff	},
	{	MEMTYPE_MESSAGEBOX,			"MessageLog",			0xffffffff	},
	{	MEMTYPE_FEARGRID,			"FearGrid",				0xffffffff	},
	{	MEMTYPE_VBUFFER,			"VBuffer",				0xffffffff	},
	{	MEMTYPE_VBUFFER_DATA,		"VBufferData",			0xffffffff	},
	{	MEMTYPE_DYNAMIC_VBUFFER_DATA,"Dynamic VBufferData",	0xffffffff	},
	{	MEMTYPE_MAPTEX,				"MapTex",				0xffffffff	},
	{	MEMTYPE_KEMPYCUBE,			"KempyCube",			0xffffffff	},
	{	MEMTYPE_SKY,				"Sky",					0xffffffff	},
	{	MEMTYPE_WATER,				"Water",				0xffffffff	},
	{	MEMTYPE_LIGHT,				"Light",				0xffffffff	},
	{	MEMTYPE_LANDSCAPE,			"Landscape",			0xffffffff	},
	{	MEMTYPE_GAMUT,				"Gamut",				0xffffffff	},
	{	MEMTYPE_FRONTEND,			"FrontEnd",				0xffffffff	},	
	{	MEMTYPE_HUD,				"HUD",					0xffffffff	},
	{	MEMTYPE_IMPOSTER,			"Imposter",				0xffffffff	},
	{	MEMTYPE_MESHVB,				"Mesh VB",				0xffffffff	},
	{	MEMTYPE_RENDERTHING,		"Render Thing",			0xffffffff	},
	{	MEMTYPE_WEAPONMODE,			"Weapon Mode",			0xffffffff	},
	{	MEMTYPE_WEAPON,				"Weapon",				0xffffffff	},
	{	MEMTYPE_SPAWNER,			"Spawner",				0xffffffff	},
	{	MEMTYPE_EXPLOSION,			"Explosion",			0xffffffff	},
	{	MEMTYPE_COMPONENT,			"Component",			0xffffffff	},
	{	MEMTYPE_FEATURE,			"Feature",				0xffffffff	},
	{	MEMTYPE_DETAILLEVEL,		"Detail Level",			0xffffffff	},
	{	MEMTYPE_EVENTMANAGER,		"Event Manager",		0xffffffff	},
	{	MEMTYPE_SHADOW,				"Shadow",				0xffffffff	},
	{	MEMTYPE_MAPWHO,				"Map Who",				0xffffffff	},
	{	MEMTYPE_POLYBUCKET,			"Poly Bucket",			0xffffffff	},
	{	MEMTYPE_POLYBUCKET_ENTRY,	"Poly Bucket Entry",	0xffffffff	},
	{	MEMTYPE_PTRSET,				"PtrSet",				0xffffffff	},
	{	MEMTYPE_RADAR,				"Radar",				0xffffffff	},
	{	MEMTYPE_SOUND,				"Sound",				0xffffffff	},
	{	MEMTYPE_SOUND_SAMPLE,		"Sound sample",			0xffffffff	},
	{	MEMTYPE_SPTRSET,			"SPtrSet",				0xffffffff	},
	{	MEMTYPE_BYTESPRITE,			"ByteSprite",			0xffffffff	},
	{	MEMTYPE_CONSOLE,			"Console",				0xffffffff	},
	{	MEMTYPE_WORLDMESHLIST,		"World Mesh List",		0xffffffff	},
	{	MEMTYPE_VERTEXSHADER,		"Vertex Shader",		0xffffffff	},
	{	MEMTYPE_WALL,				"Wall",					0xffffffff	},
	{	MEMTYPE_IBUFFER,			"IBuffer",				0xffffffff	},
	{	MEMTYPE_TREE,				"Tree man stuff",		0xffffffff	},
	{	MEMTYPE_FONT,				"Font",					0xffffffff	},	
	{	MEMTYPE_ARRAY,				"Array",				0xffffffff	},	

	{	MT_DESTROYABLESEGMENT,		"Destroyable Segment",	0xffffffff	},
	{	MT_DISPLAYLIST,				"Display List",			0xffffffff	},
	{	MT_PS2PROFILER,				"PS2 Profiler",			0xffffffff	},
	{	MT_PRIMITIVECOMPILER,		"Primitive Compiler",	0xffffffff	},
	{   MEMTYPE_ACTIVE_READER,		"Active Reader",		0xffffffff  },
	{   MT_LANDSCAPEDATA,			"Land scape data",	    0xffffffff  },
	{   MEMTYPE_SPHERETRIGGER,		"Sphere trig contains", 0xffffffff  }, 
	{   MEMTYPE_GENERAL_VOLUME,     "General volume",		0xffffffff  },
	{   MEMTYPE_SCRIPT_THING_PTR,   "Script thing ptr",		0xffffffff  },
	{   MEMTYPE_DELETION_CALLBACK_LIST, "Deletion callback list", 0xffffffff },
	{   MT_PALETTIZER,				"Palettizer",			0xffffffff },

	{   MEMTYPE_SCRATCHPAD,			"Scratchpad",			0xffffffff },
	{   MEMTYPE_TEMP,				"Temp",					0xffffffff },
	{   MEMTYPE_MEMTAG,				"Memory tag",			0xffffffff },
	{   MEMTYPE_DUMPTEMP,			"Memory dump temporary",0xffffffff },

	{	MEMTYPE_ATMOSPHERICS,		"Atmospherics",			0xffffffff },

	{	MEMTYPE_AREA,				"Area",					0xffffffff },
	{	MEMTYPE_DIRECTOR,			"Director",				0xffffffff },
	{	MEMTYPE_GROUPMANAGER,		"Group manager",		0xffffffff },
	{	MEMTYPE_PATH,				"Path",					0xffffffff },
	{	MEMTYPE_WORLD,				"World",				0xffffffff },
	{	MEMTYPE_CONFIGURATIONS,		"Configurations",		0xffffffff },
	{	MEMTYPE_COLLISION,			"Collision",			0xffffffff },
	{	MEMTYPE_MODELVIEWER,		"Model viewer",			0xffffffff },
	{	MEMTYPE_NODE,				"Node",					0xffffffff },
	{	MEMTYPE_GAMEDISC,			"Gamedisc",				0xffffffff },
	{	MEMTYPE_STATICSHADOW,		"Static shadow",		0xffffffff },
	{	MEMTYPE_DEBUGMARKER,		"Debug marker",			0xffffffff },
	{	MEMTYPE_TEXT,				"Text",					0xffffffff },
	{	MEMTYPE_SCENENODE,			"Scene nodes",			0xffffffff },
	{	MEMTYPE_MESHCACHE,			"Meshpart cache",		0xffffffff },
	{	MEMTYPE_SCENE,				"Scene",				0xffffffff },
	{	MEMTYPE_SCRIPT,				"Script2",				0xffffffff },
	{	MEMTYPE_MIXERMAP,			"Mixer map",			0xffffffff },
	{   MEMTYPE_HELP_TEXT_DISPLAY,  "Help Text Display",    0xffffffff },
	{   MEMTYPE_STORAGE,			"Storage fallback",		0xffffffff },

	{   MEMTYPE_BINK_VIDEO,			"Bink Video",			0xffffffff },
	{   MEMTYPE_MAPWHO_ENTRY,		"Mapwho entry",			0xffffffff },
	{   MEMTYPE_MEMORYCARD,			"Memory card",			0xffffffff },
	{   MEMTYPE_POSE,				"Mesh pose data",		0xffffffff },
	{	MEMTYPE_INDEX_BUFFER,		"Index buffer data",	0xffffffff },
	
	{	MEMTYPE_TINY_HEAP,			"Tiny Block heap",		0xffffffff },
	{	MEMTYPE_FMV,				"FMV",					0xffffffff },

	{	MEMTYPE_UNKNOWN,			"Unknown",				0xffffffff	},
	{	MEMTYPE_LIMIT,				"",						0xffffffff	}
};

//*******************************************************************
//
//	CMemoryBlock
//
//*******************************************************************

void CMemoryBlock::Init( UINT aSize, BOOL aUsed, EMemoryType aType, char * aFilename, UINT aLine )
{
	mMagic = MEM_MAGIC_HEADER;

	ASSERT( (aSize&0xf) == 0 );
	mSize = aSize - MEMBLOCK_HEADER_SIZE;
	SetUsed( aUsed );
	SetBaseSet(FALSE);
	mType = aType;
	mpNext = 0;

#ifdef MEMMANAGER_DEBUG
	strncpy( mFilename, aFilename, 64 );
	mFileLine = aLine;
#endif
}

void CMemoryBlock::Resize( UINT aSize )
{
	// round size up to nearest 16 bytes:
	ASSERT( (aSize&0xf) == 0 );
	mSize = aSize - MEMBLOCK_HEADER_SIZE;
}
	
void CMemoryBlock::SetUsed( BOOL aUsed )
{
	if( aUsed )
	{
		mSize |= 1;
	}
	else
	{
		mSize &= 0xfffffffe;
	}
}

//*****************************************************************************
//
//	CMemoryHeap
//
//*****************************************************************************

CMemoryHeap::CMemoryHeap()
{
#ifdef _DIRECTX
	m_Mutex = 0;
#endif
}

// XBox needs to support allocating physical contiguous memory
// Jan says - I'm trying to share code as much as possible here rather than having two almost-
// identical functions.
#if TARGET == XBOX
BOOL CMemoryHeap::Init( UINT aSize, UINT tinysize, char *name, BOOL bPhysicalContiguous, BOOL bSupportSmallAllocs)
#else
BOOL CMemoryHeap::Init( UINT aSize, UINT tinysize, char *name, BOOL bSupportSmallAllocs)
#endif
{
	m_bSupportSmallAllocs = bSupportSmallAllocs;

	// memory usage statistics:
	int i;
	mUsedSize = mFreeSize = 0;
	mUsedMaxSize = 0;
	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		mTypeSize[ i ] = 0;
		mLastTypeSize[ i ] = 0;
	}

	// block statistics:
	mUsedBlocks = mFreeBlocks = 0;
	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		mTypeBlocks[ i ] = 0;
		mLastTypeBlocks[ i ] = 0;
	}

	for( i=0; i<16; ++i )
	{
		mpSmallFree[ i ] = 0;
	}

	if( aSize & 0xf )
	{
		aSize += 0x10;
	}
	aSize &= 0xfffffff0;

	mMerge = TRUE;

	ASSERT(strlen(name)<64);
	strcpy(mName,name);

	// Add heap to the heap list

	mNextHeap=MEM_MANAGER.mFirstHeap;
	MEM_MANAGER.mFirstHeap=this;

	// allocate memory and add to free list
	mSize = aSize;


	// The actual allocation is slightly different on XBOX
#if TARGET == XBOX
	// There are two different flavours of memory.
	m_bPhysicalContiguous = bPhysicalContiguous;
	if (bPhysicalContiguous)
	{
		mpMem = D3D_AllocContiguousMemory(mSize, D3DTEXTURE_ALIGNMENT);
		char buf[200];
		sprintf(buf, "Physical Contiguous Memory heap base=0x%8x\n",mpMem);
		OutputDebugString(buf);
	}
	else
	{
		// allocate memory and add to free list
		mpMem = malloc( mSize );
		char buf[200];
		sprintf(buf, "Memory heap base=0x%8x\n",mpMem);
		OutputDebugString(buf);
	}
#else // TARGET == XBOX
	// just a simple malloc thanks.
	mpMem = malloc( mSize );
#endif // TARGET == XBOX


	if( mpMem )
	{
		while( (UINT)mpMem & 0xf )
		{
			// nasty - will break deletion
			mpMem = (void*)( (UINT)mpMem + 1 );
			mSize--;
		}
		mSize = (mSize>>4)<<4;
		mpFree = (CMemoryBlock*)mpMem;
		mpFree->Init( mSize );
		mpFree->SetUsed( FALSE );
		mpFree->SetBaseSet(FALSE);
		mFreeSize = aSize;
		mFreeBlocks++;

		ASSERT( ( (UINT)mpMem & 0xf ) == 0 );

		if (tinysize != 0)
		{
			// Now allocate a block for the tiny stuff.
			ASSERT((tinysize & (TINY_BLOCK - 1)) == 0);
			mTinyHeap = Alloc(tinysize, MEMTYPE_TINY_HEAP, __FILE__, __LINE__);

			// And fill it
			void *freeness;
			for (freeness = mTinyHeap; freeness < (void *)(((char *)mTinyHeap) + tinysize - TINY_BLOCK); freeness = *(void **)freeness)
			{
				*(void **)freeness = (void *)(((char *)freeness) + TINY_BLOCK);
			}

			// terminate it
			*(void **)freeness = NULL;

			// and start it.
			mTinyHeapFreePtr = mTinyHeap;

			mTinyHeapEnd = (void *)(((char *)mTinyHeap) + tinysize);
		}
		else
		{
			// no tiny heap.
			mTinyHeapFreePtr = mTinyHeapEnd = mTinyHeap = NULL;
		}

		return TRUE;
	}

	return FALSE;
}

//*****************************************************************************
static bool in_free_list(void *mem, void *heap, void *free_ptr, void *heap_end)
{
	bool retval = false;

	while (free_ptr)
	{
		ASSERT(heap < free_ptr && free_ptr < heap_end); // check the free ptr points to something in the heap.
		ASSERT(((char *)free_ptr - (char *)heap) % TINY_BLOCK == 0); // free ptr should be on a block boundary.

		// so is this block in the free list?
		if (free_ptr == mem) retval = true;

		// go to the next free ptr.
		free_ptr = *(void **)free_ptr;
	};

	return retval;
}

bool CMemoryHeap::FreeTiny(void *mem)
{
	if (mem && mem >= mTinyHeap && mem < mTinyHeapEnd)
	{
		// It's one of mine!
		// While we're here, let's check for some validity.

		ASSERT(((char *)mem - (char *)mTinyHeap) % TINY_BLOCK == 0); // should be on a block boundary.

		// too damn slow:
//		ASSERT(!in_free_list(mem, mTinyHeap, mTinyHeapFreePtr, mTinyHeapEnd)); // shouldn't be already free

		// it's one of mine! Stick it back in the free list.
		*(void **)mem = mTinyHeapFreePtr;
		mTinyHeapFreePtr = mem;

		return true;
	}

	return false;
}

//*****************************************************************************
bool CMemoryHeap::ReallocTiny(void *mem, UINT newsize, void **retval)
{
	if (!IsTiny(mem)) return false;

	// Let's just get the data into temporary space.
	char temp[TINY_BLOCK];
	memcpy(temp, mem, TINY_BLOCK);

	// get rid of the current alloc
	FreeTiny(mem);

	// create a new one.
	void *data = Alloc(newsize, MEMTYPE_GENERIC, __FILE__, __LINE__);

	if (data)
	{
		UINT copysize = min(newsize, TINY_BLOCK);
		memmove(data, temp, copysize);
	}
	else
	{
		// Oh dear out of memory. The complaint will happen in the Alloc method so
		// nothing needs doing here.
	}

	*retval = data;

	return true;
}

//*****************************************************************************
void *CMemoryHeap::AllocTiny()
{
	// do we have space?
	if (mTinyHeapFreePtr)
	{
		// We have space!
		void *retval = mTinyHeapFreePtr;
		mTinyHeapFreePtr = *(void **)mTinyHeapFreePtr;

		return retval;
	}
	
	// oh dear.
	return NULL;
}

//*****************************************************************************

void CMemoryHeap::FlagAsBaseSet()
{
	CMemoryBlock *block;
	for (block= (CMemoryBlock*) mpMem; !IsLastBlock(block); block=block->GetBlockBelow())
	{
		if (block->IsUsed())
		{
			block->SetBaseSet(TRUE);
		}
	}		
}

//*****************************************************************************

void CMemoryHeap::ClearToBaseSet()
{
	printf("-- CLEARING MEMORY MAP --\n");

	BOOL anyerrors=FALSE;

	CMemoryBlock *block;
	for (block= (CMemoryBlock*) mpMem; !IsLastBlock(block); block=block->GetBlockBelow())
	{
		if (block->IsUsed())
		{
			if (!block->IsBaseSet())
			{
				printf("Deleting stray block at 0x%08x\n",(UINT) block);
#ifdef _DEBUG
			//	printf("-Allocated from %s line %d\n",block->mFilename,block->mLine);
#endif
				Free(block);

				anyerrors=TRUE;
			}
		}
	}	
	
	printf("-- DONE --\n");

	if (CLIPARAMS.mDevKit)
	{
		if (anyerrors)
			SASSERT(0,"Memory leaks!");
	}
}

//*****************************************************************************
void CMemoryHeap::Shutdown()
{
#ifndef VANILLA_MEMORYMANAGER
	// Remove ourselves from the heap list

	CMemoryHeap *c=MEM_MANAGER.mFirstHeap;
	CMemoryHeap *l=NULL;

	while (c && (c!=this))
	{
		l=c;
		c=c->mNextHeap;
	}

	ASSERT(c);

	if (l)
		l->mNextHeap=this->mNextHeap;
	else
		MEM_MANAGER.mFirstHeap=this->mNextHeap;

#if TARGET == XBOX

	// remove all blocks and free memory
	if (m_bPhysicalContiguous)
		D3D_FreeContiguousMemory(mpMem);
	else
		free(mpMem);

#else
	
	free(mpMem);

#endif
#endif
}

int alloccount=0;

#define SMALL_BLOCK_GAIN_TOLERANCE 1     // multiply by 16 bytes

//*****************************************************************************
//static BOOL	jcl_debug_check = TRUE;
static BOOL	jcl_debug_check = FALSE;

void * CMemoryHeap::Alloc( UINT aSize, EMemoryType aType, char * apFilename, UINT aLine )
{
	PROTECT_WITH_MUTEX();

	PROFILE_FN(MMAlloc);

#ifdef VANILLA_MEMORYMANAGER
	SASSERT(0,"Don't call the memory manager with VANILLA_MEMORYMANAGER defined!");
#endif

//	ASSERT(mMerge == TRUE); // can't alloc in unmerged state
	 
#if TARGET==PS2
/*	if ((alloccount % 100)==0)
		CONSOLE.Print("Alloc() number %d\n",alloccount);
	alloccount++;
	char buf[256];
	sprintf(buf,"Alloc() at %s, line %d\n",apFilename,aLine);
	Validate(buf);*/
//	ASSERT( aType != MEMTYPE_GENERIC );
#endif

#if TARGET != PS2
/*	if (aType == MEMTYPE_GENERIC)
	{
		char buf[256];
		sprintf(buf,"Blah\n"); 
		if (aSize>10000)
			TRACE(buf);
	}*/
/*		if (jcl_debug_check)
			if (aSize > 100)
				__asm int 3;*/
#endif

	// round up to next 16
	aSize += 0xf;
	aSize &= 0xfffffff0;

	// Try allocating in the tiny heap.
	if (aSize <= TINY_BLOCK)
	{
		void *retval = AllocTiny();
		if (retval) return retval;

		// otherwise, no space, drop through to default allocator.
	}

	UINT lUsedBlockSize = aSize + MEMBLOCK_HEADER_SIZE;
	

	// do some special stuff if block is small:
	if ((aSize < 256) && (m_bSupportSmallAllocs))
	{
		int min_size = (aSize >> 4) + SMALL_BLOCK_GAIN_TOLERANCE ;
		if (min_size > 16 ) min_size = 16 ;

		for( int i=aSize >> 4; i< min_size;  ++i )
		{
			if( mpSmallFree[ i ] )
			{
				CMemoryBlock * lpBlock = mpSmallFree[ i ];
				mpSmallFree[ i ]->SetUsed( TRUE );
				mpSmallFree[ i ]->SetBaseSet( FALSE );
				mpSmallFree[ i ]->mType = aType;

#ifdef MEMMANAGER_DEBUG
				strncpy( mpSmallFree[ i ]->mFilename, apFilename, 60 );
				mpSmallFree[ i ]->mFileLine = aLine;
#endif

				// find new free block:
				mpSmallFree[ i ] = mpSmallFree[ i ]->mpNext;

				mFreeSize -= lpBlock->GetBlockSize();
				mUsedSize += lpBlock->GetBlockSize();
				mTypeSize[ aType ] += lpBlock->GetBlockSize();
				mUsedBlocks++;
				mFreeBlocks--;
				mTypeBlocks[ aType ]++;

				return lpBlock->GetMem();
			}
		}
	}

	CMemoryBlock * lpFree = GetFreeBlock( aSize );	

	if( !lpFree )
	{
		lpFree = GetFreeBlock( aSize );

		// no free memory at all:
		Cleanup(false);

		lpFree = GetFreeBlock( aSize );			
		
		if( !lpFree )
		{
			// still no free memory at all!:
#if TARGET==PS2
			DPUT_GS_BGCOLOR(SCE_GS_SET_BGCOLOR(0,0,0xFF));
#endif
			LOG.AddMessage("FATAL ERROR: Out of memory!!! (trying to allocate %d bytes)", aSize) ;
			LogStats();
			MEM_MANAGER.DumpMemory("Out of memory");
#if TARGET==PS2
			printf("Out of memory!\n");
			asm("break");
#elif TARGET == XBOX
			// the out-of-memories are caught elsewhere
#else
			SASSERT(0,"Out of memory!");
#endif
			return 0;
		}
	}
	
	if( lUsedBlockSize < lpFree->GetSize() )
	{
		// no problem to split block

		// calculate new block sizes (including pad);
		UINT lFreeBlockSize = lpFree->GetBlockSize() - lUsedBlockSize;

		CMemoryBlock * lpUsed = lpFree;
		lpFree = (CMemoryBlock*)( (UINT)lpUsed + lUsedBlockSize );

		// resize free block:
		lpFree->Init( lFreeBlockSize, FALSE, aType, apFilename, aLine );
		AddToFreeList( lpFree );

		// create used block:
		lpUsed->Init( lUsedBlockSize, TRUE, aType, apFilename, aLine );

		mFreeSize -= lUsedBlockSize;
		mUsedSize += lUsedBlockSize;
		if( mUsedSize > mUsedMaxSize )
		{
			mUsedMaxSize = mUsedSize;
		}
		mTypeSize[ aType ] += lUsedBlockSize;
		mUsedBlocks++;
		mTypeBlocks[ aType ]++;

		return lpUsed->GetMem();
	}
	
	else if( aSize <= lpFree->GetSize() )
	{
		// free block too small to split. just convert whole block to used
		lUsedBlockSize = lpFree->GetBlockSize();

		// add to used list:
		CMemoryBlock * lpBlock = lpFree;
		lpFree->SetUsed( TRUE );
		lpFree->SetBaseSet( FALSE );
		lpFree->mType = aType;

#ifdef MEMMANAGER_DEBUG
		strncpy( lpFree->mFilename, apFilename, 64 );
		lpFree->mFileLine = aLine;
#endif

		mFreeSize -= lUsedBlockSize;
		mUsedSize += lUsedBlockSize;
		mTypeSize[ aType ] += lUsedBlockSize;
		mUsedBlocks++;
		mFreeBlocks--;
		mTypeBlocks[ aType ]++;

		return lpBlock->GetMem();
	}
	else
	{
		// not enough free memory:
		Cleanup(false);													// slow, but might find us the memory we need
		CMemoryBlock * lpFree = GetFreeBlock( aSize );

		if( !lpFree )
		{
			ASSERT( 0 );
			return 0;
		}
		else
		{
			return Alloc( aSize, aType, apFilename, aLine );
		}
	}
}

//*****************************************************************************
void * CMemoryHeap::ReAlloc( CMemoryBlock * apBlock, UINT aSize )
{
	// don't grab the mutex, the functions called will do that instead
#ifdef VANILLA_MEMORYMANAGER
	SASSERT(0,"Don't call the memory manager with VANILLA_MEMORYMANAGER defined!");
#endif	

#ifdef MEMMANAGER_DEBUG
	void * lpMem = Alloc( aSize, apBlock->GetType(), apBlock->mFilename, apBlock->mFileLine );
#else
	void * lpMem = Alloc( aSize, apBlock->GetType(), "", 0 );
#endif
	if( lpMem )
	{
		SINT lSize = ( apBlock->GetSize() > aSize ) ? aSize : apBlock->GetSize();
		memcpy( lpMem, apBlock->GetMem(), lSize );
	}
	Free( apBlock );

	return lpMem;
}

//*****************************************************************************
void CMemoryHeap::Free( CMemoryBlock * apBlock )
{
	PROTECT_WITH_MUTEX();

	PROFILE_FN(MMFree);

#ifdef VANILLA_MEMORYMANAGER
	SASSERT(0,"Don't call the memory manager with VANILLA_MEMORYMANAGER defined!");
#endif	

	UINT lBlockSize = apBlock->GetBlockSize();

	mFreeSize += lBlockSize;
	mUsedSize -= lBlockSize;
	mTypeSize[ apBlock->GetType() ] -= lBlockSize;
	ASSERT( mTypeSize[ apBlock->GetType() ] >= 0 );
	mUsedBlocks--;
	ASSERT( mUsedBlocks >= 0 );
	mFreeBlocks++;
	mTypeBlocks[ apBlock->GetType() ]--;

	apBlock->SetUsed( FALSE );
	apBlock->SetBaseSet( FALSE );

	AddToFreeList( apBlock );
}

//*****************************************************************************
void CMemoryHeap::AddToFreeList( CMemoryBlock * apBlock )
{
	// special case for small blocks:
	if ((apBlock->GetSize() < 256) && m_bSupportSmallAllocs)
	{
		// add free block into list in memory order (to get maximum reuse):
/*		CMemoryBlock * lpPrev = 0;
		for( CMemoryBlock * lpBlock = mpSmallFree[ apBlock->GetSize() >> 4 ]; lpBlock; lpPrev = lpBlock, lpBlock = lpBlock->mpNext )
		{
			if( (UINT)lpBlock < (UINT)apBlock )
			{
				break;
			}
		}
		if( lpPrev )
		{
			// add into list:
			apBlock->mpNext = lpPrev->mpNext;
			lpPrev->mpNext = apBlock;
		}
		else
		{*/
			apBlock->mpNext = mpSmallFree[ apBlock->GetSize() >> 4 ];
/*			if ((unsigned)apBlock->mpNext > 0xf0000000)
				_asm int 3*/
			mpSmallFree[ apBlock->GetSize() >> 4 ] = apBlock;
//		}
	}
	else
	{
		CMemoryBlock * lpPrev = 0;
		CMemoryBlock * lpBlock;

		if( mMerge )
		{
			// attempt to merge with existing free blocks:
			for( lpBlock = mpFree; lpBlock; lpPrev = lpBlock, lpBlock = lpBlock->mpNext )
			{
				ASSERT( lpBlock->IsValid() );
				if( lpBlock->GetBlockBelow() == apBlock )
				{
					// remove block from list:
					if( lpPrev )
					{
						lpPrev->mpNext = lpBlock->mpNext;
/*						if ((unsigned)(lpPrev->mpNext) > 0xf0000000)
						_asm int 3*/
					}
					else
					{
						mpFree = lpBlock->mpNext;
					}

					// merge blocks:			
					UINT lMergedSize = lpBlock->GetBlockSize() + apBlock->GetBlockSize();
					apBlock = lpBlock;
					apBlock->Resize( lMergedSize );
					mFreeBlocks--;
					break;
				}
			}

			lpPrev = 0;
			for( lpBlock = mpFree; lpBlock; lpPrev = lpBlock, lpBlock = lpBlock->mpNext )
			{
				if( apBlock->GetBlockBelow() == lpBlock )
				{
					// remove block from list:
					if( lpPrev )
					{
						lpPrev->mpNext = lpBlock->mpNext;
/*						if ((unsigned)(lpPrev->mpNext) > 0xf0000000)
							_asm int 3*/
					}
					else
					{
						mpFree = lpBlock->mpNext;
					}

					// merge blocks:			
					UINT lMergedSize = lpBlock->GetBlockSize() + apBlock->GetBlockSize();
					apBlock->Resize( lMergedSize );
					mFreeBlocks--;
					break;
				}
			}

			// add free block into list in size order:
			lpPrev = 0;
			for( lpBlock = mpFree; lpBlock; lpPrev = lpBlock, lpBlock = lpBlock->mpNext )
			{
				if( lpBlock->GetSize() >= apBlock->GetSize() )
				{
					break;
				}
			}
		}

		if( lpPrev )
		{
			// add into list:
			apBlock->mpNext = lpPrev->mpNext;
/*			if ((unsigned)(apBlock->mpNext) > 0xf0000000)
							_asm int 3*/
			lpPrev->mpNext = apBlock;
		}
		else
		{
			// add to start of list:
			apBlock->mpNext = mpFree;
/*			if ((unsigned)(apBlock->mpNext) > 0xf0000000)
					_asm int 3*/
			mpFree = apBlock;
		}
	}
}

//*****************************************************************************
//	void	SetMerge( BOOL aMerge ) { mMerge = aMerge; }
void	CMemoryHeap::SetMerge( BOOL aMerge )
{
	if (aMerge == TRUE)
	{
		if (mMerge == FALSE)
		{
			// going from non-merged to merged.

			// Cleanup, then sort the blocks
			Cleanup();

			CMemoryBlock	*fb = mpFree;
			BOOL			first_time = TRUE;

			while (fb)
			{
				// simple (shite) sort - find smallest first.
				CMemoryBlock	*b = fb;
				CMemoryBlock	*smallest = fb;
				CMemoryBlock	*prev = NULL;
				CMemoryBlock	*prev_small = NULL;
				UINT			size = 0x8fffffff;

				while (b)
				{
					if (b->GetSize() < size)
					{
						size = b->GetSize();
						smallest = b;
						prev_small = prev;
					}
					prev = b;
					b = b->mpNext;
				};

				// remove from list and put at head 
				if (prev_small)
				{
					prev_small->mpNext = smallest->mpNext;
/*					if ((unsigned)(prev_small->mpNext) > 0xf0000000)
							_asm int 3*/
					smallest->mpNext = fb;
					fb = smallest;
				}
				else
				{
					// was first one, so don't need to do anything
				}

				if (first_time)
				{
					mpFree = fb;
					first_time = FALSE;
				}

				fb = fb->mpNext;
			}
		}
	}

	mMerge = aMerge;
}

//*****************************************************************************
void CMemoryHeap::Cleanup(bool needs_mutex)
{
#ifndef VANILLA_MEMORYMANAGER
#ifdef _DIRECTX
	if (needs_mutex) GRAB_MUTEX(&m_Mutex);
#endif

	// removes all blocks from free lists,
	// then merges blocks and adds to lists again.
	// Should result in non-fragmented free blocks:
	int i;
	for( i=0; i<16; ++i )
	{
		mpSmallFree[ i ] = 0;
	}
	mpFree = 0;

	CMemoryBlock * lpBlock;
	for( lpBlock = (CMemoryBlock*)mpMem; ; lpBlock = lpBlock->GetBlockBelow() )
	{
		lpBlock->mpNext = 0;
		if( lpBlock->IsFree() )
		{
			while( (!IsLastBlock(lpBlock) )&& lpBlock->GetBlockBelow()->IsFree() )
			{
				// can merge these blocks:			
				UINT lMergedSize = lpBlock->GetBlockSize() + lpBlock->GetBlockBelow()->GetBlockSize();
				lpBlock->Resize( lMergedSize );
				mFreeBlocks--;
			}

			// special case for small blocks:
			if ((lpBlock->GetSize() < 256) && m_bSupportSmallAllocs)
			{
				// add free block into list in memory order (to get maximum reuse):
				CMemoryBlock * lpPrev = 0;
				for( CMemoryBlock * lpListBlock = mpSmallFree[ lpBlock->GetSize() >> 4 ]; lpListBlock; lpPrev = lpListBlock, lpListBlock = lpListBlock->mpNext )
				{
					if( (UINT)lpListBlock < (UINT)lpBlock )
					{
						break;
					}
				}
				if( lpPrev )
				{
					// add into list:
					lpBlock->mpNext = lpPrev->mpNext;
/*					if ((unsigned)(lpBlock->mpNext) > 0xf0000000)
							_asm int 3*/
					lpPrev->mpNext = lpBlock;
/*					if ((unsigned)(lpPrev->mpNext) > 0xf0000000)
							_asm int 3*/
				}
				else
				{
					// add to start of list:
					lpBlock->mpNext = mpSmallFree[ lpBlock->GetSize() >> 4 ];
/*					if ((unsigned)(lpBlock->mpNext) > 0xf0000000)
							_asm int 3*/
					mpSmallFree[ lpBlock->GetSize() >> 4 ] = lpBlock;
				}
			}
			else
			{
				// add free block into list in size order:
				CMemoryBlock * lpPrev = 0;
				BOOL lBreak = FALSE;
				for( CMemoryBlock * lpListBlock = mpFree; lpListBlock; lpPrev = lpListBlock, lpListBlock = lpListBlock->mpNext )
				{
					if( lpListBlock->GetSize() >= lpBlock->GetSize() )
					{
						break;
					}
				}
				if( lpPrev )
				{
					// add into list:
					lpBlock->mpNext = lpPrev->mpNext;
/*					if ((unsigned)(lpBlock->mpNext) > 0xf0000000)
							_asm int 3*/
					lpPrev->mpNext = lpBlock;
/*					if ((unsigned)(lpPrev->mpNext) > 0xf0000000)
							_asm int 3*/
				}
				else
				{
					// add to start of list:
					lpBlock->mpNext = mpFree;
/*					if ((unsigned)(lpBlock->mpNext) > 0xf0000000)
							_asm int 3*/
					mpFree = lpBlock;
				}
			}
		}

		if( IsLastBlock( lpBlock ) )
		{
			break;
		}
	}

#ifdef _DIRECTX
	if (needs_mutex) RELEASE_MUTEX(&m_Mutex);
#endif
#endif
}
	
//*****************************************************************************
BOOL CMemoryHeap::FreeAll( EMemoryType aType )
{
	return FALSE;
}

//*****************************************************************************
BOOL CMemoryHeap::Validate(char *msg)
{
	PROTECT_WITH_MUTEX();
	printf("Validating heap %s\n",mName);
	// steps through blocks in order to check for gaps:
	CMemoryBlock * lpBlock;
	for( lpBlock = (CMemoryBlock*)mpMem; !IsLastBlock( lpBlock ); lpBlock = lpBlock->GetBlockBelow() )
	{
		if( !lpBlock->IsValid() )
		{
//			CONSOLE.Print("Memory manager block validation failure at 0x%8x\n",lpBlock);
//			CONSOLE.Print("Invalid block at 0x%8x, type %s\nSize %d (%s)\nNext block = 0x%8x\n\n",lpBlock,CMEMORYMANAGER::mTypeName[lpBlock->GetType()],lpBlock->GetSize(),lpBlock->GetBlockSize(),lpBlock->GetBlockBelow());
//			if (msg)
//				CONSOLE.Print(msg);
			SASSERT(0,"Invalid block in memory manager!");
			return false;
		}

		if ((((int) lpBlock->GetBlockBelow())==0) || ((((int) lpBlock->GetBlockBelow()) & 0xF)!=0))
		{
//			CONSOLE.Print("Memory manager block validation failure at 0x%8x\n",lpBlock);
//			CONSOLE.Print("Invalid block at 0x%8x, type %s\nSize %d (%s)\nNext block = 0x%8x\n\n",lpBlock,CMEMORYMANAGER::mTypeName[lpBlock->GetType()],lpBlock->GetSize(),lpBlock->GetBlockSize(),lpBlock->GetBlockBelow());
//			if (msg)
//				CONSOLE.Print(msg);
			SASSERT(0,"Invalid next block in memory manager!");
			return FALSE;
		}

		if( lpBlock->GetBlockSize() == 0 )
		{
//			CONSOLE.Print("Memory manager block validation failure at 0x%8x\n",lpBlock);
//			CONSOLE.Print("Invalid block at 0x%8x, type %s\nSize %d (%s)\nNext block = 0x%8x\n\n",lpBlock,CMEMORYMANAGER::mTypeName[lpBlock->GetType()],lpBlock->GetSize(),lpBlock->GetBlockSize(),lpBlock->GetBlockBelow());
//			if (msg)
//				CONSOLE.Print(msg);
			SASSERT(0,"Invalid block size in memory manager!");
			return FALSE;
		}
	}

	// check free lists:
	for( UINT i=0; i<16; ++i )
	{
		for( CMemoryBlock * lpBlock = mpSmallFree[i]; lpBlock; lpBlock = lpBlock->mpNext )
		{
			if( lpBlock->GetSize() != i*16 )
			{
				return FALSE;
			}
		}
	}

	UINT lSize = 0;
	for( lpBlock = mpFree; lpBlock; lpBlock = lpBlock->mpNext )
	{
		if( lpBlock->GetSize() < 256 )
		{
			return FALSE;
		}

		if( lpBlock->GetSize() < lSize )
		{
			return FALSE;
		}

		lSize = lpBlock->GetSize();
	}
	
	return TRUE;
}

//*****************************************************************************
void CMemoryHeap::OutputStats( char * aFilename )
{
	char lBuffer[ 100 ];
	char lOutput[ 256*100 ];

	sprintf( lOutput, "Used: %d bytes\n", mUsedSize );

	sprintf( lBuffer, "Free: %d bytes\n", mFreeSize );
	strcat( lOutput, lBuffer );

	sprintf( lBuffer, "Total: %d bytes\n\n", mSize );
	strcat( lOutput, lBuffer );

	// print out details:

	UINT BySize[ MEMTYPE_LIMIT ];
	UINT i;
	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		BySize[ i ] = i;
	}
	BOOL lSwap = TRUE;
	while( lSwap )
	{
		lSwap = FALSE;
		for( i=0; i<MEMTYPE_LIMIT-1; ++i )
		{
			if( mTypeSize[ BySize[ i ] ] < mTypeSize[ BySize[ i+1 ] ] )
			{
				lSwap = TRUE;
				UINT lTemp = BySize[ i ];
				BySize[ i ] = BySize[ i+1 ];
				BySize[ i+1 ] = lTemp;
			}
		}
	}

	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		sprintf( lBuffer, "%-32s : %15d bytes : %15d blocks\n", CMEMORYMANAGER::mTypeName[BySize[i]], mTypeSize[BySize[i]], mTypeBlocks[BySize[i]] );
		strcat( lOutput, lBuffer );
	}

	char lFilename[ 256 ];
	sprintf( lFilename, "data\\Memory\\%s", aFilename );

	CMEMBUFFER lMemBuf;
	lMemBuf.InitFromMem( lFilename );
	lMemBuf.Write( lOutput, strlen( lOutput ) );
	lMemBuf.Close();
}


//*****************************************************************************
void CMemoryHeap::CalcSmallBlocksLeft(int num, int* blocks_used, int* mem_used)
{
	CMemoryBlock* block = 	mpSmallFree[ num ] ;
	while (block)
	{
		(*blocks_used)++;
		(*mem_used)+=block->GetBlockSize() ;
		block = block->mpNext ;
	}
}


//*****************************************************************************
void CMemoryHeap::PrintStats()
{
	char lBuffer[ 100 ];

	int font_height = PLATFORM.Font(FONT_DEBUG)->GetHeight();

#if TARGET == XBOX
	float font_left = 50;
#else
	float font_left = 32;
#endif

	sprintf(lBuffer, "Used: %d bytes\n", mUsedSize );
	int y = 32;
	PLATFORM.Font(FONT_DEBUG)->DrawText( font_left, float(y), 0xffffffff, ToWCHAR(lBuffer));
	y+=font_height;

	sprintf( lBuffer, "Free: %d bytes\n", mFreeSize );
	PLATFORM.Font(FONT_DEBUG)->DrawText( font_left, float(y), 0xffffffff, ToWCHAR(lBuffer));
	y+=font_height;
	sprintf( lBuffer, "Total: %d bytes\n", mSize );
	PLATFORM.Font(FONT_DEBUG)->DrawText( font_left, float(y), 0xffffffff, ToWCHAR(lBuffer));
	y+=font_height;

	int ii=0;
	for (ii=0;ii<16;ii+=2)
	{
		int num_small_blocks[2]  = {0,0};
		int small_blocks_mem_used[2] ={0,0};

		CalcSmallBlocksLeft(ii, &num_small_blocks[0], &small_blocks_mem_used[0]) ;
		CalcSmallBlocksLeft(ii+1, &num_small_blocks[1], &small_blocks_mem_used[1]) ;
		sprintf( lBuffer, "(%2d) num blocks: %5d  (bytes %8d)         (%2d) num blocks: %5d  (bytes %8d) \n\n", ii, num_small_blocks[0], small_blocks_mem_used[0], ii+1,num_small_blocks[1], small_blocks_mem_used[1] );
		PLATFORM.Font(FONT_DEBUG)->DrawText( font_left, float(y), 0xffffffff, ToWCHAR(lBuffer));
		y+=font_height;
	}

	// print out details:

	UINT BySize[ MEMTYPE_LIMIT ];
	UINT i;
	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		BySize[ i ] = i;
	}
	BOOL lSwap = TRUE;
	while( lSwap )
	{
		lSwap = FALSE;
		for( i=0; i<MEMTYPE_LIMIT-1; ++i )
		{
			if( mTypeSize[ BySize[ i ] ] < mTypeSize[ BySize[ i+1 ] ] )
			{
				lSwap = TRUE;
				UINT lTemp = BySize[ i ];
				BySize[ i ] = BySize[ i+1 ];
				BySize[ i+1 ] = lTemp;
			}
		}
	}

	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		y+=font_height;
		sprintf( lBuffer, "%-32s : %15d bytes : %15d blocks\n", CMEMORYMANAGER::mTypeName[BySize[i]], mTypeSize[BySize[i]], mTypeBlocks[BySize[i]] );
		PLATFORM.Font(FONT_DEBUG)->DrawText( font_left, float(y), 0xffffff00, ToWCHAR(lBuffer));
	}

}

//*****************************************************************************
void CMemoryHeap::LogStats()
{
	char	str[1000];

	sprintf(str, "Used: %d bytes\n", mUsedSize );
	LOG.AddMessage(str);
	TRACE(str);
	sprintf(str, "Free: %d bytes\n", mFreeSize );
	LOG.AddMessage(str);
	TRACE(str);
	sprintf(str, "Total: %d bytes\n", mSize );
	LOG.AddMessage(str);
	TRACE(str);

	// print out details:

	UINT BySize[ MEMTYPE_LIMIT ];
	UINT i;
	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		BySize[ i ] = i;
	}
	BOOL lSwap = TRUE;
	while( lSwap )
	{
		lSwap = FALSE;
		for( i=0; i<MEMTYPE_LIMIT-1; ++i )
		{
			if( mTypeSize[ BySize[ i ] ] < mTypeSize[ BySize[ i+1 ] ] )
			{
				lSwap = TRUE;
				UINT lTemp = BySize[ i ];
				BySize[ i ] = BySize[ i+1 ];
				BySize[ i+1 ] = lTemp;
			}
		}
	}

	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		if (mTypeSize[BySize[i]] > 0)
		{
			sprintf(str, "%-32s : %15d bytes : %15d blocks\n", CMEMORYMANAGER::mTypeName[BySize[i]], mTypeSize[BySize[i]], mTypeBlocks[BySize[i]] );
			LOG.AddMessage(str);
			TRACE(str);
		}
	}
}

//*****************************************************************************
void CMemoryHeap::CalcAndShowDeltas()
{
	// JCL - display differences between this time and Last time.
	SINT	c0;
	char	str[400];

	for(c0 = 0; c0 < MEMTYPE_LIMIT; c0 ++)
	{
		if ((mTypeSize  [c0] != mLastTypeSize  [c0]) ||
			(mTypeBlocks[c0] != mLastTypeBlocks[c0]))
		{
			sprintf(str, "Heap Delta: %-32s : %15d bytes : %15d blocks\n", CMEMORYMANAGER::mTypeName[c0],
							mTypeSize[c0] - mLastTypeSize[c0],
							mTypeBlocks[c0] - mLastTypeBlocks[c0]);
			TRACE(str);
		}
	}

	// and store for next time
	for(c0 = 0; c0 < MEMTYPE_LIMIT; c0 ++)
	{
		mLastTypeSize[c0]   = mTypeSize[c0];
		mLastTypeBlocks[c0] = mTypeBlocks[c0];
	}
}

//*****************************************************************************
void CMemoryHeap::OutputBlocks( char * aFilename, EMemoryType aType )
{
	char lFilename[ 256 ];
	sprintf( lFilename, "data\\Memory\\%s", aFilename );

	char lBuffer[ 256 ];

	FILE * lpFile = fopen( lFilename, "w" );
	if( !lpFile )
	{
		return;
	}

	if( aType < MEMTYPE_LIMIT )
	{
		// output stats for blocks of particular type:

		CMemoryBlock * lpBlock;
		for( lpBlock = (CMemoryBlock*)mpMem; !IsLastBlock( lpBlock ); lpBlock = lpBlock->GetBlockBelow() )
		{
			if( lpBlock->IsUsed() && lpBlock->GetType() == aType )
			{
				// output block info:
#ifdef MEMMANAGER_DEBUG
				sprintf( lBuffer, "%#.8x, %-32s, %d, %s, %d\n", lpBlock->GetMem(), CMEMORYMANAGER::mTypeName[aType], lpBlock->GetSize(), lpBlock->mFilename, lpBlock->mFileLine );
#else
				sprintf( lBuffer, "%#.8x, %-32s, %d, (no filename), (no line)\n", lpBlock->GetMem(), CMEMORYMANAGER::mTypeName[aType], lpBlock->GetSize() );
#endif
				fwrite( lBuffer, 1, strlen( lBuffer ), lpFile );
			}
		}
	}
	else
	{
		// output stats for all blocks:
		CMemoryBlock * lpBlock;
		for( lpBlock = (CMemoryBlock*)mpMem; !IsLastBlock( lpBlock ); lpBlock = lpBlock->GetBlockBelow() )
		{
			if( lpBlock->IsUsed() )
			{
				// output block info:
#ifdef MEMMANAGER_DEBUG
				sprintf( lBuffer, "%#.8x, %-32s, %d, %s, %d\n", lpBlock->GetMem(), CMEMORYMANAGER::mTypeName[lpBlock->GetType()], lpBlock->GetSize(), lpBlock->mFilename, lpBlock->mFileLine );
#else
				sprintf( lBuffer, "%#.8x, %-32s, %d, (no filename), (no line)\n", lpBlock->GetMem(), CMEMORYMANAGER::mTypeName[aType], lpBlock->GetSize() );
#endif
				fwrite( lBuffer, 1, strlen( lBuffer ), lpFile );
			}
		}
	}

	fclose( lpFile );
}

//*****************************************************************************
void CMemoryHeap::OutputMap(char *aFilename)
{
	char lFilename[ 256 ];
	sprintf( lFilename, "data\\Memory\\%s", aFilename );

	char lBuffer[ 256 ];

	FILE *test;
	test=fopen(lFilename,"w");
	if( !test )
	{
		return;
	}

	sprintf( lBuffer, "Memory map\n");

	fwrite(lBuffer,1,strlen(lBuffer),test);	
	
	CMemoryBlock * lpBlock;
	for( lpBlock = (CMemoryBlock*)mpMem; !IsLastBlock(lpBlock); lpBlock = lpBlock->GetBlockBelow() )
	{
		if( !lpBlock->IsValid() )
		{
			strcpy(lBuffer,"[INVALID!] ");
			fwrite(lBuffer,1,strlen(lBuffer),test);
		}
		
		if( lpBlock->GetBlockSize() == 0 )
		{
			strcpy(lBuffer,"[ZERO LENGTH!] ");
			fwrite(lBuffer,1,strlen(lBuffer),test);
		}		

		sprintf(lBuffer,"0x%8x %-32s : %15d", (UINT)lpBlock->GetMem(), CMEMORYMANAGER::mTypeName[ lpBlock->GetType() ], lpBlock->GetBlockSize() );

		if (lpBlock->IsFree())
			strcat(lBuffer," [free]");

		fwrite(lBuffer,1,strlen(lBuffer),test);		

#ifdef MEMMANAGER_DEBUG
		sprintf(lBuffer,"(%s : %d)\n",lpBlock->mFilename,lpBlock->mFileLine);
#endif
		
		fwrite(lBuffer,1,strlen(lBuffer),test);	
	}
		
	fclose(test);
}

//*****************************************************************************
CMemoryBlock * CMemoryHeap::GetFreeBlock( UINT aMinSize )
{
	// searches free blocks to find another free block with size at least aMinSize
	CMemoryBlock *lpPrev = 0;
	for( CMemoryBlock * lpBlock = mpFree; lpBlock; lpPrev = lpBlock, lpBlock = lpBlock->mpNext  )
	{
		if( lpBlock->GetSize() >= aMinSize )
		{
			if( lpPrev )
			{
				lpPrev->mpNext = lpBlock->mpNext;
/*				if ((unsigned)(lpPrev->mpNext) > 0xf0000000)
							_asm int 3*/
			}
			else
			{
				// block is first in free list:
				mpFree = lpBlock->mpNext;
			}
			lpBlock->mpNext = 0;
			return lpBlock;
		}
	}

	return 0;
}

//*****************************************************************************
UINT CMemoryHeap::FindLargestFree()
{
	CMemoryBlock * lpLargestBlock = mpFree;
	for( CMemoryBlock * lpBlock = mpFree; lpBlock; lpBlock = lpBlock->mpNext  )
	{
		if( lpBlock->GetSize() > lpLargestBlock->GetSize() )
		{
			lpLargestBlock = lpBlock;
		}
	}

	return lpLargestBlock->GetSize();
}

//*****************************************************************************
CMemoryBlock * CMemoryHeap::FindSmallestFree( UINT aMinSize )
{
/*	UINT lSmallestSize = mpFree->GetSize();
	CMemoryBlock * lpSmallestBlock = mpFree;
	CMemoryBlock * lpBlock = mpFree->GetBlockBelow();
	while( lpBlock != mpFree )
	{
		if( (UINT)lpBlock >= (UINT)mpMem + mSize )
		{
			// have reached bottom of memory, so loop to top:
			ASSERT( (UINT)lpBlock == (UINT)mpMem + mSize );
			lpBlock = (CMemoryBlock*)mpMem;
			continue;
		}

		ASSERT( lpBlock->IsValid() );

		if( lpBlock->IsFree() && ( lpBlock->GetSize() >= aMinSize ) && ( lpBlock->GetSize() < lSmallestSize ) )
		{
			lSmallestSize = lpBlock->GetSize();
			lpSmallestBlock = lpBlock;
		}

		lpBlock = lpBlock->GetBlockBelow();
	}

	return lpSmallestBlock;*/
	return 0;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

void CMemoryManager::TagMem(void *addr,UINT size,const char *fmt,...)
{
#ifdef MEMORY_TAGGING
	CMemoryTag *t=new (MEMTYPE_MEMTAG) CMemoryTag;

	va_list arglist;
		
	va_start(arglist, fmt);
	vsprintf(t->mTag, fmt, arglist);
	va_end(arglist);	

	for (int i=0;i<(int) strlen(t->mTag);i++)
		if (t->mTag[i]=='\\')
			t->mTag[i]='/';

	t->mAddress=addr;
	t->mSize=size;
	t->mNext=mFirstTag;
	t->mDeleted=FALSE;
	mFirstTag=t;
#endif
}

//*****************************************************************************

void CMemoryManager::UnTagMem(void *addr)
{
#ifdef MEMORY_TAGGING
	CMemoryTag *c=mFirstTag;

	while (c)
	{
		if (c->mAddress==addr)
			c->mDeleted=TRUE;
		c=c->mNext;
	}
#endif
}

//*****************************************************************************

void CMemoryHeap::DumpMap(CMEMBUFFER *mb,int heapno)
{
	int blocks=0;
	CMemoryBlock *block;
	char buffer[256];

	for (block= (CMemoryBlock*) mpMem; !IsLastBlock(block); block=block->GetBlockBelow())
		if (block->GetType()!=MEMTYPE_DUMPTEMP)
			blocks++;
	
	sprintf(buffer,"# Heap %d\n",heapno);
	mb->Write(buffer,strlen(buffer));

	sprintf(buffer,"%s\n",mName);
	mb->Write(buffer,strlen(buffer));
	
	sprintf(buffer,"# Size\n");
	mb->Write(buffer,strlen(buffer));
	
	sprintf(buffer,"%d\n",mSize);
	mb->Write(buffer,strlen(buffer));
	
	sprintf(buffer,"# NumBlocks\n");
	mb->Write(buffer,strlen(buffer));	
	
	sprintf(buffer,"%d\n",blocks);
	mb->Write(buffer,strlen(buffer));	
	
	for (block= (CMemoryBlock*) mpMem; !IsLastBlock(block); block=block->GetBlockBelow())
	{
		if (block->GetType()!=MEMTYPE_DUMPTEMP)
		{
			sprintf(buffer,"# Heap %d Block %d\n",heapno,blocks);
			mb->Write(buffer,strlen(buffer));
			
			if (block->IsValid())
				sprintf(buffer,"1\n");
			else
				sprintf(buffer,"0\n");
			mb->Write(buffer,strlen(buffer));
			
			sprintf(buffer,"%d\n",block->GetSize());
			mb->Write(buffer,strlen(buffer));
			
			sprintf(buffer,"%d\n",block->GetBlockSize());
			mb->Write(buffer,strlen(buffer));
			
			sprintf(buffer,"%d\n",(UINT) block->GetMem());
			mb->Write(buffer,strlen(buffer));
			
			sprintf(buffer,"%d\n",block->GetType());
			mb->Write(buffer,strlen(buffer));
			
			if (block->IsFree())
				sprintf(buffer,"1\n");
			else
				sprintf(buffer,"0\n");
			mb->Write(buffer,strlen(buffer));
			
#ifdef MEMMANAGER_DEBUG
			sprintf(buffer,"%s\n",block->mFilename);
			mb->Write(buffer,strlen(buffer));
			sprintf(buffer,"%d\n",block->mFileLine);
			mb->Write(buffer,strlen(buffer));
#else
			sprintf(buffer,"Unknown\n");
			mb->Write(buffer,strlen(buffer));
			sprintf(buffer,"0\n");
			mb->Write(buffer,strlen(buffer));
#endif		
			
			blocks++;
		}
	}	
}

//*****************************************************************************

void CMemoryManager::FlagAsBaseSet()
{
	CMemoryHeap *c=mFirstHeap;
	while (c)
	{
		c->FlagAsBaseSet();
		c=c->mNextHeap;
	}
}

//*****************************************************************************

void CMemoryManager::FlagBlockAsBaseSet( void * apMem )
{
	CMemoryBlock * lpBlock = CMemoryBlock::GetBlock( apMem );
	ASSERT( lpBlock->IsValid() && lpBlock->IsUsed() );

	lpBlock->SetBaseSet( TRUE );
}

//*****************************************************************************

void CMemoryManager::ClearToBaseSet()
{
	CMemoryHeap *c=mFirstHeap;
	while (c)
	{
		c->ClearToBaseSet();
		c=c->mNextHeap;
	}
}

//*****************************************************************************

void CMemoryManager::DumpMemory(char *tracename)
{
#if TARGET==PS2
	if (!CLIPARAMS.mDevKit)
		return;
#endif
	if (mDumpingMemory)
		return;

	mDumpingMemory=TRUE;

	char buf[256];
	sprintf(buf,"Writing memory dump '%s'\n",tracename);
	TRACE(buf);

	CMEMBUFFER *mb=new (MEMTYPE_DUMPTEMP) CMEMBUFFER;
	char filename[256];
#if TARGET==XBOX
	sprintf(filename,"dump%d.mem",mTraceNumber);
#else
	sprintf(filename,"MemoryDumps\\dump%d.mem",mTraceNumber);
#endif
	mb->InitFromMem(filename,MEMTYPE_DUMPTEMP);

	char buffer[8192];

	// Trace name

	sprintf(buffer,"#Trace name\n");
	mb->Write(buffer,strlen(buffer));
	
	sprintf(buffer,"%s\n",tracename);
	mb->Write(buffer,strlen(buffer));	

	// Memory types

	sprintf(buffer,"#MemTypes\n");
	mb->Write(buffer,strlen(buffer));

	sprintf(buffer,"%d\n",MEMTYPE_LIMIT);
	mb->Write(buffer,strlen(buffer));

	for (int i=0;i<MEMTYPE_LIMIT;i++)
	{
		sprintf(buffer,"%s\n",CMEMORYMANAGER::mTypeName[i]);
		mb->Write(buffer,strlen(buffer));
	}

	// Memory map

	int heaps=0;
	CMemoryHeap *c=mFirstHeap;
	while (c)
	{
		heaps++;
		c=c->mNextHeap;
	}

	sprintf(buffer,"#Heaps\n");
	mb->Write(buffer,strlen(buffer));	

	sprintf(buffer,"%d\n",heaps);
	mb->Write(buffer,strlen(buffer));

	c=mFirstHeap;
	heaps=0;
	while (c)
	{
		c->DumpMap(mb,heaps);
		heaps++;
		c=c->mNextHeap;
	}

	// Tags

	int tags=0;

	CMemoryTag *ctag=mFirstTag;
	while (ctag)
	{
		tags++;
		ctag=ctag->mNext;
	}

	
	sprintf(buffer,"# NumTags\n");
	mb->Write(buffer,strlen(buffer));	
	
	sprintf(buffer,"%d\n",tags);
	mb->Write(buffer,strlen(buffer));	
	
	ctag=mFirstTag;
	while (ctag)
	{
		sprintf(buffer,"# Tag\n");
		mb->Write(buffer,strlen(buffer));	
		
		sprintf(buffer,"%s\n",ctag->mTag);
		mb->Write(buffer,strlen(buffer));
		
		
		sprintf(buffer,"%d\n",ctag->mAddress);
		mb->Write(buffer,strlen(buffer));
		
		sprintf(buffer,"%d\n",ctag->mSize);
		mb->Write(buffer,strlen(buffer));
		
		if (ctag->mDeleted)
			sprintf(buffer,"1\n");
		else
			sprintf(buffer,"0\n");
		mb->Write(buffer,strlen(buffer));
		
		ctag=ctag->mNext;
	}

	mb->Close();
	SAFE_DELETE(mb);

	mTraceNumber++;

	// Update the trace numbering file
	
	mb=new (MEMTYPE_DUMPTEMP) CMEMBUFFER;
#if TARGET!=XBOX
	mb->InitFromMem("MemoryDumps\\trace.no",MEMTYPE_DUMPTEMP);
#else
	mb->InitFromMem("trace.no",MEMTYPE_DUMPTEMP);
#endif

	sprintf(buffer,"%d\n",mTraceNumber);
	mb->Write(buffer,strlen(buffer));

	mb->Close();
	SAFE_DELETE(mb);
	mDumpingMemory=FALSE;
}

