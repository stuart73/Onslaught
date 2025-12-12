#include "common.h"

#if TARGET==PC

#include "memorycard.h"
#include "resourceaccumulator.h"


#ifdef RESBUILDER
void CPCMemoryCard::AccumulateResources(CResourceAccumulator *ra, DWORD flags)
{
	// this if-statement not possible: if (ra->GetTargetLevel()==-2) // Frontend only
	// because targetlevel isn't defined yet. But this only ever gets called when accumulating the
	// frontend resources so we don't need to do any checking here.

	CTEXTURE *saveimage=CTEXTURE::GetTextureByName("FrontEnd\\v2\\FE_XB_SaveGame.tga",TEXFMT_UNKNOWN,TEX_NORMAL,1);
	saveimage->AccumulateResources(ra,flags | RES_NOTONPS2);
	saveimage->Release();
}
#endif
#endif