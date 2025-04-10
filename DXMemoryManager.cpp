// DXMemoryManager.cpp: implementation of the CDXMemoryManager class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"

#ifdef _DIRECTX

#include "DXMemoryManager.h"
#include "debuglog.h"
#include "console.h"

CDXMemoryManager MEM_MANAGER;

char CDXMemoryManager::mTypeName[ MEMTYPE_LIMIT ][ 32 ];
UINT CDXMemoryManager::mTypeSizeLimit[ MEMTYPE_LIMIT ];
BOOL CDXMemoryManager::mInit=FALSE;

extern MemoryTypeData gMemTypeData[];

//*****************************************************************************
//
//	CMemoryManager
//
//*****************************************************************************

CDXMemoryManager::CDXMemoryManager()
{
	int i;

	// individual heaps per type:
	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		mTypeHeap[ i ] = &mDefaultHeap;
	}

	// heap names and sizes:
	for( i=0; i<MEMTYPE_LIMIT; ++i )
	{
		// find name:
		strcpy( mTypeName[ i ], "Name not found" );
		mTypeSizeLimit[ i ] = 0xffffffff;
		for( UINT j=0; gMemTypeData[ j ].mType != MEMTYPE_LIMIT; ++j )
		{
			if( gMemTypeData[ j ].mType == i )
			{
				strcpy( mTypeName[ i ], gMemTypeData[ j ].mName );
				mTypeSizeLimit[ i ] = gMemTypeData[ j ].mMaxSize;
				break;
			}
		}
	}
}

//*****************************************************************************
CDXMemoryManager::~CDXMemoryManager()
{
}

//*****************************************************************************

#if TARGET==PC
BOOL CDXMemoryManager::Init( UINT aSize )
#else
BOOL CDXMemoryManager::Init(UINT aSize, UINT aTexDataSize, UINT aVBDataSize)
#endif
{
	mInit=TRUE;
	mDumpingMemory=FALSE;

	// Default heap is in regular malloc'd memory
//	mInit &= mDefaultHeap.Init(aSize,"Default heap", false);
#if TARGET==XBOX
	mInit &= mDefaultHeap.Init(aSize,300*1024,"Default heap", false, true);
#else
	mInit &= mDefaultHeap.Init(aSize,300*1024,"Default heap", true);
#endif
	
#if TARGET==XBOX
	if( !mDumpHeap.Init(20*1024,0,"Dump memory heap",false))
	{
		return FALSE;
	}
#else 
	if( !mDumpHeap.Init(2300*1024,0,"Dump memory heap"))
	{
		return FALSE;
	}	
#endif

	mTypeHeap[ MEMTYPE_DUMPTEMP ] = &mDumpHeap;	

#if TARGET == PC
	if (!mSoundHeap.Init(30 * 1024 * 1025 + 512 * 1024, 0, "Sound memory heap", false))
#else
	if (!mSoundHeap.Init(6600 * 1024, 0, "Sound memory heap", false))
#endif
	{
		return FALSE;
	}

	mTypeHeap[MEMTYPE_SOUND_SAMPLE] = &MEM_MANAGER.mSoundHeap;

#ifdef USE_THING_HEAP
	if( !mThingHeap.Init(THING_HEAP_SIZE,200*1024,"Thing memory heap",false))
	{
		return FALSE;
	}

	mTypeHeap[MT_TREE_THING] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_UNIT_THING] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_THING] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_ROUND] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_WEAPON] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_WEAPONMODE] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_AI] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_ACTIVE_READER] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_SQUAD] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_UNIT_DATA] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_DUNNO_THING] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_INIT_THING] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_ROUND_DATA] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_NAVIGATION] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_EXPLOSION] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_GUIDE] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_COLLISION] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_DELETION_CALLBACK_LIST] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_GENERAL_VOLUME] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_RENDERTHING] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_CST] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_MOTIONCONTROLLER] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MEMTYPE_MAPWHO_ENTRY] = &MEM_MANAGER.mThingHeap;
	mTypeHeap[MT_BUBBLE] = &MEM_MANAGER.mThingHeap;

#endif

#if TARGET==XBOX
	// Texture and vertex buffer data needs to be allocated in contiguous write-combined memory
	mInit &= mTexDataHeap.Init(aTexDataSize, 0,"Texture data heap", true);
	mInit &= mVBDataHeap.Init(aVBDataSize, 0,"Vertex buffer data heap", true);

	mTypeHeap[MEMTYPE_TEXTURE_DATA] = &MEM_MANAGER.mTexDataHeap;
	mTypeHeap[MEMTYPE_VBUFFER_DATA] = &MEM_MANAGER.mVBDataHeap;
	mTypeHeap[MEMTYPE_DYNAMIC_VBUFFER_DATA] = &MEM_MANAGER.mVBDataHeap;
#endif

	return mInit;
}

//*****************************************************************************
void CDXMemoryManager::Shutdown()
{
	mInit=FALSE;
	mDefaultHeap.Shutdown();

#if TARGET == XBOX
	mTexDataHeap.Shutdown();
#endif
}


//*****************************************************************************
void * CDXMemoryManager::Alloc( UINT aSize, EMemoryType aType, char * apFilename, UINT aLine )
{
	void	*data;
#ifdef VANILLA_MEMORYMANAGER
	data = malloc(aSize);

#else
	ASSERT(mInit);
	data = mTypeHeap[ aType ]->Alloc( aSize, aType, apFilename, aLine );
#endif

	if (!data)
	{
		// out of memory! Let's print a deceiving error message
		DWORD colour = 0xffff0000;

		if (mTypeHeap[aType] == &mDefaultHeap) colour = 0xffff0000;
		else
		if (mTypeHeap[aType] == &mDumpHeap   ) colour = 0xffff00ff;
		else
		if (mTypeHeap[aType] == &mThingHeap  ) colour = 0xffffff00;
		else
		if (mTypeHeap[aType] == &mSoundHeap  ) colour = 0xff0000ff;

#if TARGET == XBOX
		if (mTypeHeap[aType] == &mTexDataHeap) colour = 0xff00ff00;
		else
		if (mTypeHeap[aType] == &mVBDataHeap ) colour = 0xff00ffff;
#endif
		CONSOLE.RenderDiscFailureTextAndHang(colour);
	}

	return data;
}

//*****************************************************************************
void  * CDXMemoryManager::ReAlloc( void * apMem, UINT aSize )
{
	void	*data;
#ifdef VANILLA_MEMORYMANAGER
	data = realloc(apMem,aSize);
#else

	// maybe it's a tiny block?
	if (mDefaultHeap.ReallocTiny(apMem, aSize, &data)) return data;
	if (mThingHeap  .ReallocTiny(apMem, aSize, &data)) return data;

	// otherwise just go through the normal realloc process
	ASSERT(mInit);
	CMemoryBlock * lpBlock = CMemoryBlock::GetBlock( apMem );
	ASSERT( lpBlock->IsValid() && lpBlock->IsUsed() );

	data = mTypeHeap[ lpBlock->GetType() ]->ReAlloc( lpBlock, aSize );
#endif

	return data;
}

BOOL CDXMemoryManager::DoesExist(void * apMem)
{
	ASSERT(mInit);
	// frees memory pointer passed
	if( !apMem )
	{
		return FALSE;
	}

	// check block:
	CMemoryBlock * lpBlock = CMemoryBlock::GetBlock( apMem );

	if (lpBlock->IsValid() == FALSE)
	{
		return TRUE ;
	}

	if (lpBlock->IsUsed()) return TRUE  ;
	LOG.AddMessage("Memory does not exist") ;
	int* fred = NULL ;
	*fred = 0 ;
	return FALSE ;
}


//*****************************************************************************
void CDXMemoryManager::Free( void * apMem )
{
#ifdef VANILLA_MEMORYMANAGER
	free(apMem);
#else
	ASSERT(mInit);
	// frees memory pointer passed
	if( !apMem )
	{
		return;
	}

	// Maybe it's a tiny block?
	if (mDefaultHeap.FreeTiny(apMem)) return;
	if (mThingHeap  .FreeTiny(apMem)) return;

	// check block:
	CMemoryBlock * lpBlock = CMemoryBlock::GetBlock( apMem );
	ASSERT( lpBlock->IsValid() && lpBlock->IsUsed() );

	if (lpBlock->GetType() != MEMTYPE_SCRATCHPAD)
	{
		char buf[20] = "";
	}

	mTypeHeap[ lpBlock->GetType() ]->Free( lpBlock );
#endif
}

//*****************************************************************************
void CDXMemoryManager::Cleanup()
{
#ifndef VANILLA_MEMORYMANAGER
	ASSERT(mInit);
	mDefaultHeap.Cleanup();

#if TARGET == XBOX
	mTexDataHeap.Cleanup();
#endif
#endif
	mSoundHeap.Cleanup();
}

//*****************************************************************************
BOOL CDXMemoryManager::FreeAll( EMemoryType aType )
{
	ASSERT(mInit);
	return FALSE;
}

//*****************************************************************************
BOOL CDXMemoryManager::Validate()
{
#ifndef VANILLA_MEMORYMANAGER
	ASSERT(mInit);
/*	for( int i=0; i<MEMTYPE_LIMIT; ++i )
	{
		if( !mTypeHeap[ i ]->Validate() )
		{
			return FALSE;
		}
	}*/
	mDefaultHeap.Validate();

#if TARGET == XBOX
	mTexDataHeap.Validate();
	mVBDataHeap.Validate();
#endif
#endif
	return TRUE;
}

//*****************************************************************************
void CDXMemoryManager::OutputMap( char *aFilename )
{
	mDefaultHeap.OutputMap( aFilename );
}

//*****************************************************************************

// let's print stats from different heaps.
char heapnr;

void	CDXMemoryManager::PrintStats()
{
	switch(heapnr)
	{
#if TARGET == XBOX
	case 2:
		mTexDataHeap.PrintStats();
		break;

	case 3:
		mVBDataHeap.PrintStats();
		break;
#endif

	case 1:
		mThingHeap.PrintStats();
		break;

	case 0:
	default:
		mDefaultHeap.PrintStats() ;
		break;
	}
}

//*****************************************************************************
void CDXMemoryManager::OutputStats( char * aFilename )
{
	mDefaultHeap.OutputStats( aFilename );
}

//*****************************************************************************
void CDXMemoryManager::OutputBlocks( char * aFilename, EMemoryType aType )
{
	mDefaultHeap.OutputBlocks( aFilename, aType );
}

//*****************************************************************************
void CDXMemoryManager::CalcAndShowDeltas()
{
	TRACE("** Memory Deltas **\n");
	mDefaultHeap.CalcAndShowDeltas();
	mDumpHeap.CalcAndShowDeltas();
	mThingHeap.CalcAndShowDeltas();

#if TARGET == XBOX
	mTexDataHeap.CalcAndShowDeltas();
	mVBDataHeap.CalcAndShowDeltas();
#endif

	TRACE("**  End Deltas   **\n");
}

#if TARGET == XBOX
//*****************************************************************************
UINT CDXMemoryManager::GetVBDataUsedSize(EMemoryType type)
{
	return mVBDataHeap.GetTypeSize(type);
}
#endif

//*****************************************************************************
void CDXMemoryManager::LogDebugStats()
{
	TRACE("-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	TRACE("Logging Stats\n");
	TRACE("-=-=-=-=-=-=-=-=-=-=-=-=-\n");

	mDefaultHeap.LogStats();
	mDumpHeap.LogStats();
	mThingHeap.LogStats();

#if TARGET == XBOX
	mTexDataHeap.LogStats();
	mVBDataHeap.LogStats();
#endif

	TRACE("--Heap Info--\n");

	char	str[500];
	sprintf(str, "Default heap - Peak %d, Size %d\n",MEM_MANAGER.GetDefaultPeakSize(), MEM_MANAGER.GetDefaultHeapSize());
	TRACE(str);
	sprintf(str, "Thing heap - Peak %d, Size %d\n", mThingHeap.GetPeakSize(), mThingHeap.GetSize());
	TRACE(str);
#if TARGET == XBOX
	sprintf(str, "Texture heap - Peak %d, Size %d\n",MEM_MANAGER.GetTexDataPeakSize(), MEM_MANAGER.GetTexDataHeapSize());
	TRACE(str);
	sprintf(str, "VBuffer heap - Peak %d, Size %d\n",MEM_MANAGER.GetVBDataPeakSize() , MEM_MANAGER.GetVBDataHeapSize());
	TRACE(str);
#endif

	// JCL - walk the native heap
#if 0
	_HEAPINFO	hinfo;
	hinfo._pentry = NULL;

	int		heapstatus;
	SINT	total_used = 0, total = 0;
	SINT	largest_size = 0;

	while( ( heapstatus = _heapwalk( &hinfo ) ) == _HEAPOK )
	{ 
/*	   char foo[500];
	   sprintf(foo, "%6s block at %Fp of size %4.4X\n",
        ( hinfo._useflag == _USEDENTRY ? "USED" : "FREE" ),
          hinfo._pentry, hinfo._size );
	   TRACE(foo);*/
		if (hinfo._size < 40000000) // skip the pools!
		{
			if (hinfo._useflag == _USEDENTRY)
			{
				total_used += hinfo._size;
				if (hinfo._size > largest_size)
					largest_size = hinfo._size;
			}
			total += hinfo._size;
		}
	}

	char foo[500];
	sprintf(foo, "** Total system memory used : %d / %d.  Largest = %d\n", total_used, total, largest_size);
	TRACE(foo);

	switch( heapstatus )
	{
	case _HEAPEMPTY:
	  TRACE( "OK - empty heap\n" );
	  break;
	case _HEAPEND:
	  TRACE( "OK - end of heap\n" );
	  break;
	case _HEAPBADPTR:
	  TRACE( "ERROR - bad pointer to heap\n" );
	  break;
	case _HEAPBADBEGIN:
	  TRACE( "ERROR - bad start of heap\n" );
	  break;
	case _HEAPBADNODE:
	  TRACE( "ERROR - bad node in heap\n" );
	  break;
	}
#endif

	TRACE("-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	TRACE("Done Logging Stats\n");
	TRACE("-=-=-=-=-=-=-=-=-=-=-=-=-\n");
}

#endif