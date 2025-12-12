#ifndef PCGAME_H
#define PCGAME_H

// don't include this file directly - include "Game.h" instead

#include	"Platform.h"
#include	"Game.h"

class	CPCGame : public CGame
{
public:
	void		DrawGameStuff();

	void		DumpTimeRecords();

	void		AddTimeStatus(char *s);
	void		SetBaseTime(float t) { mBaseTime=t; };
	void		SetFrameTime(float t) { mFrameTime=t; };
	
protected:
};

#endif