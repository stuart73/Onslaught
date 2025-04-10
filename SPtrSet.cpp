// SPtrSet.cpp: implementation of the SPtrSet class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
#include "stdafx.h"

#include "SPtrSet.h"

#ifndef SIMPLE_MESHES
#include "debuglog.h"
#include "profile.h"
#endif



// used to store all the deleted nodes. 
SPtrSetNode* GenericSPtrSet::mFreeList = NULL;
SPtrSetNode* GenericSPtrSet::mBlock=NULL;
int			 GenericSPtrSet::mBlockSize = 0 ;


//******************************************************************************************
GenericSPtrSet::GenericSPtrSet() :
	mFirst(NULL),
	mLast(NULL),
	mSize(0)
{
}


//******************************************************************************************
// copy constructor
//******************************************************************************************
GenericSPtrSet::GenericSPtrSet(const GenericSPtrSet& copy):
	mFirst(NULL),
	mLast(NULL),
	mSize(0)
{
	GenericListIterator iterator((GenericSPtrSet*)&copy) ;
	for (void* item = iterator.First(); item != NULL ; item = iterator.Next() ) 
	{
		Append(item) ;
	}
}




//******************************************************************************************
// assignment operator
//******************************************************************************************
GenericSPtrSet& GenericSPtrSet::operator = (GenericSPtrSet& copy)
{
	RemoveAll();
	GenericListIterator iterator((GenericSPtrSet*)&copy) ;
	for (void* item = iterator.First(); item != NULL ; item = iterator.Next() ) 
	{
		Append(item) ;
	}

	return *this;
}



//******************************************************************************************
// static function that cleans up the free list. call at end of level
//******************************************************************************************
void	GenericSPtrSet::Shutdown()
{
	while (mFreeList)
	{
		SPtrSetNode* node = mFreeList ;
		mFreeList = mFreeList->mNext ;

		// If node was allocated dynamicaly rather than in block then delete it
		if (node < mBlock || node >= (mBlock + mBlockSize))
		{
			delete node ;
		}
	}
 
	// delete initial block
	delete [] mBlock ;

	mBlock = NULL ;
	mFreeList = NULL ;
}


//******************************************************************************************
void	GenericSPtrSet::ClearAnyDynamicCreatedNodes()
{
	
	SPtrSetNode* n = mFreeList;
	SPtrSetNode* prev = NULL ;
	while (n)
	{
		SPtrSetNode* node = n ;
		n = n->mNext ;

		// If node was allocated dynamicaly rather than in block then delete it
		if (node < mBlock || node >= (mBlock + mBlockSize))
		{
			if (prev)
			{
				prev->mNext = n ;
			}
			else
			{
				mFreeList = n;
			}

			delete node ;
		}
		else
		{
			prev = node ;
		}
	}
}


//******************************************************************************************
// static function that inits the free list. call before running a level
//******************************************************************************************
void	GenericSPtrSet::Init(int size)
{
	if (mBlock != NULL)
	{
		LOG.AddMessage("Warning: Initilise SptrSet twice") ;
		return ;
	}
//	else
//	{
		mBlock = new ( MEMTYPE_SPTRSET )SPtrSetNode[size] ;
		mBlockSize = size ;
//	}

	mFreeList = mBlock ; 
	for (int i=0; i <= mBlockSize-2; i++)
	{
		mFreeList[i].mNext = &mFreeList[i+1] ;
	}
	mFreeList[mBlockSize-1].mNext = NULL ;
}


//******************************************************************************************
// adds to the start of the list
//******************************************************************************************

void	GenericSPtrSet::Add(void* ptr)
{
	if (mBlock == NULL)
	{
		// THIS IS NOW FATAL !!!
		LOG.AddMessage("FATAL ERROR: SptSet::Add when freelist has not been setup") ;
		ASSERT(0);
	}

	// has free list got any entries ?
	SPtrSetNode* node = mFreeList;
	if (node != NULL)
	{
		mFreeList=mFreeList->mNext;
	}
	else
	{
		// ok create a new node instead.
#if TARGET==PC
	static count = 0 ;

	if ((count%20)==0)
	{
		LOG.AddMessage("Warning: SPtrSet creating nodes dynamicaly !! get Stu.") ;
	}
	count++ ;

#endif

		node = new( MEMTYPE_SPTRSET ) SPtrSetNode;
	}

	node->mNext = mFirst;
	node->mItem = ptr;
	mFirst = node;
	mSize++;
	if (mSize==1)
	{
		mLast = mFirst ;
	}
}


//******************************************************************************************
// append to the end of the list
//******************************************************************************************

void	GenericSPtrSet::Append(void* ptr)
{
	if (mBlock == NULL)
	{
		LOG.AddMessage("FATAL ERROR: SptSet::Add when freelist has not been setup") ;
		ASSERT(0);
	}
	// has free list got any entries ?
	SPtrSetNode* node = mFreeList;
	if (node != NULL)
	{
		mFreeList=mFreeList->mNext;
	}
	else
	{
		// ok create a new node instead.
#if TARGET==PC
	static count = 0 ;

	if ((count%20)==0)
	{
		LOG.AddMessage("Warning: SPtrSet creating nodes dynamicaly !! get Stu.") ;
	}
	count++ ;

#endif
		node = new( MEMTYPE_SPTRSET ) SPtrSetNode;
	}

	node->mNext = NULL;
	node->mItem = ptr;

	if (mLast)
	{
		mLast->mNext = node;
	}

	mLast = node ;

	mSize++;
	if (mSize==1)
	{
		mFirst = mLast ;
	}
}


//******************************************************************************************
// removes the first instance of the item found then returns
//******************************************************************************************

void	GenericSPtrSet::Remove(void* ptr)
{
#ifndef SIMPLE_MESHES
	PROFILE_FN(SPtrSetRemove);
#endif

	SPtrSetNode* prev = NULL;
	SPtrSetNode* node = mFirst;
	while (node)
	{
		if (node->mItem == ptr)
		{
			// remove node from linked list
			if (mFirst == node)
			{
				mFirst = node->mNext;
				if (mFirst == NULL)
				{
					mLast = NULL ;
				}
			}
			else
			{
				prev->mNext = node->mNext;
				if (prev->mNext == NULL)
				{
					mLast = prev ;
				}
			}

			// add node to free list
			node->mNext = mFreeList;
			mFreeList = node;
			mSize --;
			return;
		}
		prev = node;
		node = node->mNext;
	}
	// not found?? just return..
}

//******************************************************************************************
BOOL	GenericSPtrSet::Contains(void* ptr)
{	
	GenericListIterator iterator(this) ;
	void* item;
	FOR_ALL_ITEMS_IN(iterator, item)
	{
		if (item == ptr) return TRUE ;
	}
	return FALSE ;
}


//******************************************************************************************
GenericSPtrSet::~GenericSPtrSet()
{
	RemoveAll();
}


//******************************************************************************************
// clears the linked list and add all current nodes to the free list
//******************************************************************************************

void	GenericSPtrSet::RemoveAll() 
{
	// add all nodes to the free list
	if (mSize==0) return;
	
	if (mLast)
		mLast->mNext = mFreeList;

	mFreeList = mFirst;

	mFirst = NULL;
	mLast = NULL;
	mSize = 0;
}


//******************************************************************************************
void*   GenericSPtrSet::At(int val)
{
	SPtrSetNode* current = mFirst ;
	while(current && val > 0)
	{
		current = current->mNext ;
		val--;
	}

	return current->mItem ;
}



