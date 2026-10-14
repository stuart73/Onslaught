// PCMeshRenderer.h: interface for the CPCMeshRenderer class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PCMESHRENDERER_H
#define PCMESHRENDERER_H

#include "Mesh.h"
#include "Texture.h"
#include "MeshRenderer.h"

class CRenderThing;

// --------------------------------------------------------------------------------

#define FVF_TL_FLAGS (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1) 

struct	STLVert
{
	FVector mPos;
//	float	mRHW;
	DWORD	mDiffuse;
	float	mU, mV;
};

// --------------------------------------------------------------------------------

class CPCMeshRenderer : public CMeshRenderer
{
public:
	static	void	DrawMeshPart(FVector p, FMatrix &ori, CMESHPART *mesh_part, IRenderableThing *t, CRenderThing *rt=NULL, SINT skin = 0, SINT flags = 0,CMotionController *controller=NULL, BOOL wasreference=FALSE);
	static	void	RenderMeshPart2D(CMESHPART *mesh_part, IRenderableThing *t, SINT skin);
};

#endif // PCMESHRENDERER_H
