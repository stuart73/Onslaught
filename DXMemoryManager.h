// DXMemoryManager.h: interface for the CDXMemoryManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef DXMEMORYMANAGER_H
#define DXMEMORYMANAGER_H

#include "MemoryManager.h"

class CDXMemoryManager : public CMemoryManager  
{
public:
	CDXMemoryManager();
	~CDXMemoryManager();

	// XBOX needs a seperate heap for texture surfaces
#if TARGET == XBOX
	BOOL	Init( UINT aSize, UINT aTexDataSize, UINT aVBDataSize);
#else
	BOOL	Init( UINT aSize );
#endif 

	void	Shutdown();
	
	void  *	Alloc( UINT aSize, EMemoryType aType = MEMTYPE_GENERIC, char * apFilename = "", UINT aLine = 0 );
	void  * ReAlloc( void * apMem, UINT aSize );
	void	Free( void * apMem );

	void	Cleanup();
	BOOL	FreeAll( EMemoryType aType );

	BOOL	Validate();

	void	SetMerge( BOOL aMerge ) { mDefaultHeap.SetMerge( aMerge ); }

	void	OutputStats( char * aFilename );
	void	PrintStats() ;
	void	OutputBlocks( char * aFilename, EMemoryType aType = MEMTYPE_LIMIT );
	void	OutputMap(char *aFilename);
	BOOL	DoesExist(void * apMem) ;

	void	LogDebugStats();
	void	CalcAndShowDeltas();

	BOOL	IsThingHeapNearlyFull()
	{
#ifdef USE_THING_HEAP
		if (mThingHeap.GetFreeSize()<(THING_HEAP_NEARLY_FULL_THRESHOLD))
			return(TRUE);
		else
			return(FALSE);
#else
		return(FALSE);
#endif
	}

	BOOL	IsThingHeapFull()
	{
#ifdef USE_THING_HEAP
		if (mThingHeap.GetFreeSize()<(THING_HEAP_FULL_THRESHOLD))
			return(TRUE);
		else
			return(FALSE);
#else
		return(FALSE);
#endif
	}

	UINT	GetDefaultHeapSize() { return( mDefaultHeap.GetSize() ); }
	UINT	GetDefaultUsedSize() { return( mDefaultHeap.GetUsedSize() ); }
	UINT	GetDefaultPeakSize() { return( mDefaultHeap.GetPeakSize() ); }
#if TARGET == XBOX
	UINT	GetTexDataHeapSize() { return( mTexDataHeap.GetSize() ); }
	UINT	GetTexDataUsedSize() { return( mTexDataHeap.GetUsedSize() ); }
	UINT	GetTexDataPeakSize() { return( mTexDataHeap.GetPeakSize() ); }
	UINT	GetVBDataHeapSize() { return( mVBDataHeap.GetSize() ); }
	UINT	GetVBDataUsedSize() { return( mVBDataHeap.GetUsedSize() ); }
	UINT	GetVBDataPeakSize() { return( mVBDataHeap.GetPeakSize() ); }
	UINT	GetVBDataUsedSize(EMemoryType memtype);
#endif
	
	CMemoryHeap*   GetDefaultHeap() { return &mDefaultHeap ; }
	CMemoryHeap*   GetThingHeap() { return &mThingHeap ; }

	// heap names and sizes:
	static char		mTypeName[ MEMTYPE_LIMIT ][ 32 ];
	static UINT		mTypeSizeLimit[ MEMTYPE_LIMIT ];
	static BOOL		mInit;

private:

	// individual heaps per type:
	CMemoryHeap   *	mTypeHeap[ MEMTYPE_LIMIT ];

	CMemoryHeap		mDefaultHeap;
	CMemoryHeap		mDumpHeap;
	CMemoryHeap		mThingHeap;
	CMemoryHeap		mSoundHeap;

#if TARGET == XBOX
	CMemoryHeap		mTexDataHeap;
	CMemoryHeap		mVBDataHeap;
#endif
};

#endif // DXMEMORYMANAGER_H

