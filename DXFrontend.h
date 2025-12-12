#ifndef DXFRONTEND_H
#define DXFRONTEND_H

// Note - don't include this header file directly - use "Frontend.h" instead

//*********************************************************************************

//*********************************************************************************
class	CDXFrontEnd : public CFrontEnd
{
public:
	// rendering
	void	RenderParticles();

	void	RenderStaticScreen(CTEXTURE *screen,int alpha, DWORD col = 0xffffffff);

	virtual BOOL	RenderStart();
	virtual void	RenderEnd(BOOL started);
};

//*********************************************************************************
extern CDXFrontEnd FRONTEND;

#endif