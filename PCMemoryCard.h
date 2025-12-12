// PC Memory Card access

#ifndef PCMEMORYCARD_H
#define PCMEMORYCARD_H

#include "MemoryCard.h"

class CPCMemoryCard : public CMemoryCard
{
public:
	void		InitialiseOnce() {};
	void		Initialise() {};
	void		Shutdown() {};
	
	BOOL		IsHDDAvailable() { return(FALSE); };
	
	int			GetNumCards(int *num) { *num=0; return(MCE_SUCCESS); };
	int			GetCardInfo(int card,BOOL *present,BOOL *formatted,int *bytesfree,int *bytestotal)
	{
		if (present)
			*present=FALSE;
		if (formatted)
			*formatted=FALSE;
		if (bytesfree)
			*bytesfree=0;
		if (bytestotal)
			*bytestotal=0;
		return(MCE_SUCCESS);
	}
	int			GetCardOrPreviousCardName2(int card,WCHAR *buffer)
	{
		wcscpy(buffer,ToWCHAR(""));
		return(MCE_SUCCESS);
	}
	int			GetCardName(int card,WCHAR *buffer)
	{
		wcscpy(buffer,ToWCHAR(""));
		return(MCE_SUCCESS);
	}
	int			GetCardType(int card,int *cardtype)
	{
		*cardtype=MCT_CARD;
		return(MCE_SUCCESS);
	}
	
	int			Format(int card)
	{
		return(MCE_SUCCESS);
	}
	int			Unformat(int card)
	{
		return(MCE_SUCCESS);
	}
	
	int			GetNumSaves(int card,int *num)
	{
		*num=0;
		return(MCE_SUCCESS);
	}
	int			GetSaveName(int card,int save,WCHAR *name)
	{
		wcscpy(name,ToWCHAR(""));
		return(MCE_SUCCESS);
	}
	int			CreateSave(int card,WCHAR *name,int *save,BOOL allowed_overwrite)
	{
		*save=0;
		return(MCE_SUCCESS);
	}
	int			DeleteSave(int card,int save,WCHAR *name)
	{
		return(MCE_SUCCESS);
	}
	
	int			WriteSave(int card,int save,WCHAR *name,void *data,int datasize)
	{
		return(MCE_SUCCESS);
	}
	int			ReadSave(int card,int save,WCHAR *name,void *data,int datasize,int *bytesread)
	{
		return(MCE_SUCCESS);
	}
	
	int			GetSaveSize(int datasize,int *savesize)
	{
		*savesize=0;
		return(MCE_SUCCESS);
	}
	int			MakeHumanReadableSize(int bytes,WCHAR *buffer)
	{
		wcscpy(buffer,ToWCHAR(""));
		return(MCE_SUCCESS);
	}
	
	void		AccumulateResources(class CResourceAccumulator *ra, DWORD flags=0);	

	bool		Update() {return false;}
};

#endif