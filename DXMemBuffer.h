#ifndef DXMEMBUFFER_H
#define DXMEMBUFFER_H

#ifndef SIMPLE_MESHES
#include "memorymanager.h"


#else
enum EMemoryType
{
	MEMTYPE_MEMBUFFER,
};
#endif


#include "waitingthread.h"

class	CThing;

class	CDXMemBuffer : public IMemBuffer
{
public:
	CDXMemBuffer();
	~CDXMemBuffer();

	BOOL	InitFromMem(const char *fname,EMemoryType memtype=MEMTYPE_MEMBUFFER);
	BOOL	InitFromFile(const char *fname,EMemoryType memtype=MEMTYPE_MEMBUFFER,BOOL mungepath=TRUE,UINT startskip = 0);

	void	LoadNextBlock();

	SINT	ConvertFromWritingToReading();	// returns length

	SINT	Read(void *data, SINT size);
	void	ReadString(char *data, SINT maxchars);
	void	Write(void *data, SINT size);

	SINT	Skip(SINT size);

	BOOL	Close();

	BOOL	IsMoreData() {ASSERT(mReading); return (mPtr < mData + mDataSize);};

	void	DeclareInvalidData(CThing *t);

	char	*GetData() {return mData;}
//	SINT	GetDataSize() {return mDataSize;}
	UINT	GetFileSize();

	BOOL	EndOfFile();

	UINT	WhereAmI() { return(mPos); };	

	static void SetNextReadBufferSize(UINT size); // pass in zero to set back to default.

protected:
	HANDLE	mFile;
	char	*mData;
	char	*mPtr;

	// OK OK so it's not a CRC but you get the idea; something that we load to check the validity
	// of the data in the file.
	char	*mCRCData;
	char	*mCRCDataUpTo;

	// only when saving, we leave the file open
	FILE	*mCRCFile;

	SINT	mSize, mDataSize;
	BOOL	mReading;
	BOOL	mEOF;
	BOOL	mLastBlock;
	char	mFileName[ 256 ];
	UINT	mPos;
	EMemoryType mMemType;

	static UINT mNextReadBufferSize;
};


#endif