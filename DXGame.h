#ifndef DXGAME_H
#define DXGAME_H

// don't include this file directly - include "Game.h" instead

#include	"Platform.h"
#include	"Game.h"

class	CDXGame : public CGame
{
public:
	void		DumpTimeRecords();

	void		AddTimeStatus(char *s);
	void		SetBaseTime(float t) { mBaseTime=t; };
	void		SetFrameTime(float t) { mFrameTime=t; };
	
protected:
};

#endif