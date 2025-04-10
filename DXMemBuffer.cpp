#include	"Common.h"
#include	"stdafx.h"
#include	"cliparams.h"
#include	"console.h"

// do we use xbox internal buffering or try to dma directly into our structures?
#define MB_BUFFERING 0

#ifdef _DIRECTX

#include	"MemBuffer.h"  //(sic)

#ifndef SIMPLE_MESHES
#include    "debuglog.h"
#endif

//#include	"Globals.h"

//******************************************************************************************
//#define		MEM_BUFFER_SIZE		1024000
#define		READ_MEM_BUFFER_SIZE		(64 * 1024)
#if TARGET==PC
#define		WRITE_MEM_BUFFER_SIZE		(2 * 1024 * 1024)
#else
#define		WRITE_MEM_BUFFER_SIZE		(10 * 1024)
#endif

UINT CDXMemBuffer::mNextReadBufferSize = READ_MEM_BUFFER_SIZE;

//******************************************************************************************

void CDXMemBuffer::SetNextReadBufferSize(UINT size)
{
	// has to be a cluster size. Well, it doesn't at the moment but will one day, maybe.
	ASSERT(!(size & 0x7fff));

	if (size == 0)
	{
		// default
		mNextReadBufferSize = READ_MEM_BUFFER_SIZE;
	}
	else
	{
		mNextReadBufferSize = size;
	}
}

//******************************************************************************************
CDXMemBuffer::CDXMemBuffer()
{
	mData = NULL;
	mCRCData = NULL;
	mCRCDataUpTo = NULL;
	mCRCFile = NULL;
}
//******************************************************************************************
CDXMemBuffer::~CDXMemBuffer()
{
//	ASSERT(!(mData)); // should be closed by now...
	delete [] mData;
	delete [] mCRCData;
};

//******************************************************************************************
BOOL	CDXMemBuffer::InitFromMem(const char *fname,EMemoryType memtype)
{
	mData =	new( memtype ) char[WRITE_MEM_BUFFER_SIZE];
	mMemType = memtype;

	if (!mData)
	{
		TRACE("Could not allocate memory for write buffer\n");
		return FALSE;
	}

	mPtr = mData;
	mSize = WRITE_MEM_BUFFER_SIZE;
	mReading = FALSE;
	mEOF = FALSE;
	mDataSize = 0;
	mPos=0;

	// cache file name (for close):
#if TARGET == PC
	strncpy( mFileName, fname, 256 );
#elif TARGET == XBOX
	sprintf(mFileName, "T:\\%s", fname);
	LOG.AddMessage("*CDXMemBuffer* Trying to open file %s\n", mFileName);
#endif


//	mFile = fopen(mFileName, "wb");
	mFile = CreateFile(mFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

//	if (!mFile)
	if (mFile == INVALID_HANDLE_VALUE)
	{
		TRACE("Membuffer could not open file for writing\n");
#if TARGET==XBOX
		LOG.AddMessage("*CDXMemBuffer* Membuffer could not open file for writing");
#endif
		SAFE_DELETE(mData);
		return(FALSE);
	}

	char crcname[MAX_PATH];
	sprintf(crcname, "%s.crc", mFileName);
	mCRCFile = fopen(crcname, "wb");

	return TRUE;
}

void data_bad()
{
	CONSOLE.RenderDiscFailureTextAndHang();
}

//******************************************************************************************
BOOL	CDXMemBuffer::InitFromFile(const char *fname,EMemoryType memtype,BOOL mungepath,UINT startskip)
{
	mData =	new( memtype ) char[mNextReadBufferSize];
	mMemType = memtype;

	if (!mData)
	{
		TRACE("Could not allocate memory for read buffer\n");
		return FALSE;
	}

	mPtr = mData;
	mSize = mNextReadBufferSize;
	mReading = TRUE;
	mEOF = FALSE;
	mLastBlock = FALSE;
	mDataSize = 0;
	mPos=0;

	DWORD flags;

	if (MB_BUFFERING) flags = FILE_ATTRIBUTE_NORMAL                         ;
	else			  flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING;


	// load file, and the CRC data too.
	char crcdataname[256];
#if TARGET == PC
	mFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, flags, NULL);
	sprintf(crcdataname, "%s.crc", fname);
//	mFile = fopen(fname, "rb");
#elif TARGET == XBOX
	// if we're given an absolute path, we don't want to go adding to it.
	if (mungepath && fname[1] != ':')
	{
		char temp[256];
		sprintf(temp, "%s\\%s\0", CLIPARAMS.mBasePath, fname);
//		mFile = fopen(temp, "rb");
		mFile = CreateFile(temp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, flags, NULL);
		sprintf(crcdataname, "%s.crc", temp);
	}
	else
	{
		mFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, flags, NULL);
		sprintf(crcdataname, "%s.crc", fname);
//		mFile=fopen(fname,"rb");
	}
#endif

	if (mFile == INVALID_HANDLE_VALUE)
	{
#if TARGET == XBOX		
		LOG.AddMessage("*CDXMemBuffer*: Failed to open file %s", fname);
#endif		
		delete mData;
		mData = NULL;
		return FALSE;
	}

	// Read CRC data.
	// Stuff the CRC's - surely the DVD player will see the problem?
	FILE *crc = NULL;//fopen(crcdataname, "rb");

	if (crc)
	{
		DWORD crc_size = GetFileSize();

		if (crc_size == DWORD(-1))
		{
			ASSERT(0);
			crc_size = 1;
		}
		else
		{
			// round it up to the next buffer size.
			crc_size += mNextReadBufferSize - 1;
			crc_size /= mNextReadBufferSize;

			// we've got one byte per block of resource.
		}

		mCRCData = new char[crc_size];
		mCRCDataUpTo = mCRCData;

		if (fread(mCRCData, 1, crc_size, crc) != crc_size)
		{
			// oh dear, that didn't load either.
			data_bad();
		}
	}
	else
	{
		// If we don't have the CRC file, we don't care because all this CRC stuff is frankly bollocks anyway.
		// How can bad data get past the DVD controller? It can't.
	}

	// Sometimes we don't want the first bit of the file, why load it?
	DWORD amount_moved;
	if (startskip)
	{
		amount_moved = SetFilePointer(mFile, startskip / mNextReadBufferSize * mNextReadBufferSize, NULL, FILE_BEGIN);
	}
	else
	{
		amount_moved = 0;
	}

	if (amount_moved == 0xffffffff) data_bad();

	// and do the move in our counter too.
	mPos += amount_moved;

	// read first block:
	LoadNextBlock();

	mPtr = mData;

#if TARGET == XBOX		
		LOG.AddMessage("DXMemBuffer: Success opening file %s", fname);
#endif

	if (startskip - amount_moved > 0)
	{
		// There's a bit more skipping to do.
		Skip(startskip - amount_moved);
	}

	return TRUE;
}

//******************************************************************************************
SINT	CDXMemBuffer::ConvertFromWritingToReading()
{
	mReading	= TRUE;
	mPtr		= mData;

	return mDataSize;
}
//******************************************************************************************

static char eval_crc(const char *data, int num_bytes)
{
	char retval = 0;
	
	while (num_bytes)
	{
		retval += *data++;

		num_bytes--;
	}

	return retval;
}

// if it's a really bad winerror we'll just hang with an error message.
static void how_bad_is_that()
{
	switch(GetLastError())
	{
	case ERROR_HANDLE_EOF:
		// That's fine.
		return;

	default:
		// It's pointless having all these errors listed because of the "default" above
		// but now I've searched for them I feel like leaving them
	case ERROR_ARENA_TRASHED:
	case ERROR_INVALID_BLOCK:
	case ERROR_BAD_FORMAT:
	case ERROR_INVALID_DATA:
	case ERROR_BAD_UNIT:
	case ERROR_NOT_READY:
	case ERROR_CRC:
	case ERROR_NOT_DOS_DISK:
	case ERROR_READ_FAULT:
	case ERROR_GEN_FAILURE:
	case ERROR_OPEN_FAILED:
	case ERROR_DISK_OPERATION_FAILED:
		data_bad();
	}
}

//******************************************************************************************
void	CDXMemBuffer::LoadNextBlock()
{
	if (!ReadFile(mFile, mData, mNextReadBufferSize, (DWORD *)&mDataSize, NULL))
	{
		// failed.
		how_bad_is_that();

		mDataSize = 0;
	}

	if( mDataSize != mSize )
	{
		mLastBlock = TRUE;
	}
	mPtr = mData;

	// Either way, we now have some data, and we have to check it for CRC.
	if (mCRCData && eval_crc(mData, mDataSize) != *mCRCDataUpTo)
	{
		// the crc is wrong!
		data_bad();
	}

	// go to next block of CRC data.
	mCRCDataUpTo++;
}

//******************************************************************************************
UINT	CDXMemBuffer::GetFileSize()
{
	ASSERT(mReading);

	return ::GetFileSize(mFile, NULL);

	/*
	UINT retval;


	UINT current=ftell(mFile);
	fseek(mFile,0,SEEK_END);
	UINT l=ftell(mFile);
	fseek(mFile,current,SEEK_SET);

	mThread.ReleaseMutex();

	return(l);*/
}

//******************************************************************************************
// very very much like reading because most of the Skip's are very tiny
SINT	CDXMemBuffer::Skip(SINT size)
{
	if (!size) return 0;

	ASSERT(mReading);
	
	int bytesread=0;

	if( mLastBlock && mPtr + size > mData + mDataSize )
	{
		mEOF = TRUE;
		size = ( mData + mDataSize - mPtr );
	}

	if( size > 0 )
	{
		while( mPtr + size > mData + mDataSize )
		{
			UINT lBytesToRead = mData + mDataSize - mPtr;
			bytesread+=lBytesToRead;
			size -= lBytesToRead;

			LoadNextBlock();

			if( mLastBlock && mPtr + size > mData + mDataSize )
			{
				mEOF = TRUE;
				size = ( mData + mDataSize - mPtr );
			}
		}
		bytesread+=size;
		mPtr += size;
	}

	ASSERT(mPtr <= mData + mDataSize);

	mPos+=bytesread;

	return(bytesread);
}

//******************************************************************************************
SINT	CDXMemBuffer::Read(void *data, SINT size)
{
	ASSERT(mReading);
	
	int bytesread=0;

	char * lpData = (char *)data;
	if( mLastBlock && mPtr + size > mData + mDataSize )
	{
		mEOF = TRUE;
		size = ( mData + mDataSize - mPtr );
	}

	if( size > 0 )
	{
		while( mPtr + size > mData + mDataSize )
		{
			// We've haven't loaded enough. copy what we have...
			UINT lBytesToRead = mData + mDataSize - mPtr;
			bytesread+=lBytesToRead;
			memcpy( lpData, mPtr, lBytesToRead );
			lpData += lBytesToRead;
			size -= lBytesToRead;

			// And now, what's the point of doing block stuff and a memcpy if it's huge?
			// Now we've got non-buffered reads we have to be careful about reading the right size blocks to the right place
			// (because it's DMA)
			if (0)//!THREADED && size >= mNextReadBufferSize && !(int(lpData) & 3))
			{
				DWORD amount_loaded;

				// do a nice number of blocks (actually should be a multiple of sector size but what the hell
				DWORD amount_to_load = size / mNextReadBufferSize * mNextReadBufferSize;

				if (!ReadFile(mFile, lpData, amount_to_load, &amount_loaded, NULL))
				{
					// failed.
					how_bad_is_that();
					amount_loaded = 0;
				}

				bytesread += amount_loaded;
				lpData += amount_loaded;
				size -= amount_loaded;
				mPtr = mData;
				mDataSize = 0;

				if (amount_loaded != amount_to_load)
					mLastBlock = true;
			}

			// we want a small amount now, just do normal stuff.
			LoadNextBlock();

			if( mLastBlock && mPtr + size > mData + mDataSize )
			{
				mEOF = TRUE;
				size = ( mData + mDataSize - mPtr );
			}
		}
		bytesread+=size;
		memcpy( lpData, mPtr, size );
		mPtr += size;
	}

	ASSERT(mPtr <= mData + mDataSize);

	mPos+=bytesread;

	return(bytesread);
}

//******************************************************************************************
void	CDXMemBuffer::ReadString(char *data, SINT maxchars)
{
	ASSERT(mReading);

	// calculate number of bytes to read:
	UINT lBytesRead = 0;
	for( SINT i=0; i<maxchars-1; ++i )
	{
		if( mPtr >= mData + mDataSize )
		{
			if( mLastBlock )
			{
				mEOF = TRUE;
				break;
			}
			else
			{
				LoadNextBlock();
			}
		}

		data[lBytesRead++] = *(mPtr++);

		if (data[lBytesRead-1] == 10)
		{
			break;
		}
	}

	if( lBytesRead > 1 )

	{
		if( data[lBytesRead-2] == 0x0d )
		{
			data[lBytesRead-2] = '\n';
			data[lBytesRead-1] = '\0';
		}
	}
	data[lBytesRead] = '\0';

	mPos+=lBytesRead;

	ASSERT(mPtr <= mData + mDataSize);
}

//******************************************************************************************
static void write_crc_data(FILE *file, const char *data, SINT size, SINT buffersize)
{
	if (file)
	{
		for (; size > 0; size -= buffersize, data += buffersize)
		{
			char crc;

			if (size >= buffersize) crc = eval_crc(data, buffersize);
			else					crc = eval_crc(data, size      );

			if (fwrite(&crc, 1, 1, file) != 1)
			{
				SASSERT(0, "Can't write CRC!");
			}
		}
	}
}

//******************************************************************************************
void	CDXMemBuffer::Write(void *data, SINT size)
{
	ASSERT(!mReading);
	DWORD bytes_written;
	
	while(size > 0 && mPtr+size>mData+mSize)
	{
		// Would run off end of buffer - write out buffer

		// But because of the CRC we have to write out precise buffer-size blocks.
		SINT bytes_to_write = mSize - (mPtr - mData);
		memcpy(mPtr, data, bytes_to_write);
		
		// We save out crc data.
		write_crc_data(mCRCFile, mData, mSize, mNextReadBufferSize);

		// and then the actual data.
		if (!WriteFile(mFile, mData, mSize, &bytes_written, NULL))
		{
			TRACE("Write failed\n");
			DWORD i = GetLastError();
		}
		
		mDataSize=0;
		mPtr=mData;
		size -= bytes_to_write;
		mPos += bytes_to_write;
		data = (void *)(((char *)data) + bytes_to_write);
	}
	
	if (size > 0)
	{
		// Buffer the last bit.
		memcpy(mPtr, data, size);
		mPtr += size;
		mDataSize += size;
		mPos+=size;		
	}
}

//******************************************************************************************
BOOL	CDXMemBuffer::Close()
{
	if( mData )
	{
		if (mReading)
		{
			// just free up - easy
			CloseHandle(mFile);
//			fclose( mFile );
			delete [] mData;
			mData = NULL;
			delete [] mCRCData;
			mCRCData = NULL;
			return TRUE;
		}

		// otherwise write out the stream.

//		fwrite(mData, mDataSize, 1, mFile);
		DWORD bytes_written;
		if (!WriteFile(mFile, mData, mDataSize, &bytes_written, NULL))
		{
			TRACE("Write failed");
		}

		write_crc_data(mCRCFile, mData, mDataSize, mNextReadBufferSize);

		CloseHandle(mFile);
//		fclose(mFile);

		fclose(mCRCFile);
		mCRCFile = 0;

		// free up
		delete [] mData;
		mData = NULL;

		return TRUE;
	}

	return FALSE;
}

//******************************************************************************************
void	CDXMemBuffer::DeclareInvalidData(CThing *t)
{
	char	text[300];

	sprintf(text, "Invalid data in Thing");  // can't link _GetClassName() without editor...
//	CONSOLE.AddString(text);
}

//******************************************************************************************

BOOL	CDXMemBuffer::EndOfFile()
{
	return( mEOF );
}

//******************************************************************************************

#endif
