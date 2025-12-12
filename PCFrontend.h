#ifndef PCFRONTEND_H
#define PCFRONTEND_H

// Note - don't include this header file directly - use "Frontend.h" instead

//*********************************************************************************
#include	"FrontEnd.h"


//*********************************************************************************
class	CPCFrontEnd : public CFrontEnd
{
public:
	// rendering
	void	RenderParticles();

	void	RenderStaticScreen(CTEXTURE *screen,int alpha, DWORD col = 0xffffffff);

	virtual BOOL	RenderStart();
	virtual void	RenderEnd(BOOL started);

	// sound
	void	PlaySound(EFrontEndSound sound);

protected:
	char	*GetSoundName(EFrontEndSound sound);

};

//*********************************************************************************
extern CPCFrontEnd FRONTEND;

#endif