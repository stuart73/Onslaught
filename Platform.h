#ifndef PLATFORM_H
#define PLATFORM_H

#include "KeyEventType.h"

enum EQuitType
{
	QT_NONE	= 0,
	QT_QUIT_TO_FRONTEND,
	QT_QUIT_TO_SYSTEM,
	QT_LOAD_ERROR,
	QT_RESTART_LEVEL,
	QT_QUIT_TIMEOUT,
	QT_USER_QUIT_TO_FRONTEND,
	QT_USER_QUIT_TO_TITLE_SCREEN
};

enum EFontType
{
	FONT_NORMAL,
	FONT_SMALL,
	FONT_DEBUG,
	FONT_TITLE,
};

typedef void (*pKeyTrapper)(BYTE key,KeyEventType event);

class	CPlatform
{
public:

	// debug
 //   void		DrawDebugTextShadowed(float x, float y, DWORD col, char *s, DWORD flags = 0L);
 //   void		DrawDebugTextDynamic(float x, float y, DWORD col, char *s, float t, BOOL fade = FALSE, DWORD flags = 0L);	

	void Flip(BOOL in_game = FALSE);

};

#if TARGET == PC

#include	"PCPlatform.h"
extern	CPCPlatform PLATFORM;

#elif TARGET == PS2

#include	"PS2Platform.h"
extern	CPS2Platform PLATFORM;

#elif TARGET == XBOX

#include "XBOXplatform.h"
extern CXBOXPlatform PLATFORM;

#endif

#endif