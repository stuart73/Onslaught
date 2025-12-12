#ifndef STORAGE_H
#define STORAGE_H

class	CStorage
{
public:

};

#if TARGET == PC

#include	"PCStorage.h"
extern	CPCStorage STORAGE;

#elif TARGET == PS2

#include	"PS2Storage.h"
extern	CPS2Storage STORAGE;

#elif TARGET == XBOX

#include	 "XBOXStorage.h"
extern	CXBOXStorage STORAGE;

#endif

#endif