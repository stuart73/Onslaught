#include	"Common.h"

#include	"Platform.h"
#include	"console.h"

#if TARGET == PC
#include "DX.h"
#elif TARGET == PS2
#include "PS2DMAList.h"
#endif

#include	<string.h>

void CPlatform::Flip(BOOL in_game)
{
	// we may be forbidden and we should go into an infinite loop!
	while (CONSOLE.mStopEverything);

	// otherwise, normal flip.
	PLATFORM.DeviceFlip(in_game);
}


