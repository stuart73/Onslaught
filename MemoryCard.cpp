// Generic memory card access

#include "common.h"
#include "MemoryCard.h"

CMEMORYCARD MEMORYCARD;

BOOL CMemoryCard::CardBeenUnpluggedSinceLastTimeIAsked(int card)
{
	return FALSE;
}
