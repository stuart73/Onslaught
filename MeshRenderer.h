// --------------------------------------------------------------------------------
// Mesh rendering code
// --------------------------------------------------------------------------------

#ifndef MESH_RENDERER_H
#define MESH_RENDERER_H

#include "meshpart.h"

// --------------------------------------------------------------------------------
//  All the possible rendering modes
// --------------------------------------------------------------------------------

enum
{
	R_Normal,
	R_BumpmapNormals,
	R_Cloaked,
	R_EnvMap,
	R_Shadow,
	R_ShadowReceiver,
	R_Extruded,
	R_SphericalBumpmapNormals,
	R_NoTexture
};

// --------------------------------------------------------------------------------
//  Part rendering flags
// --------------------------------------------------------------------------------

enum
{
	RF_DONTRENDER	=	1,
	RF_HILIGHT		=	2,
	RF_BOUNDINGBOX  =   4,
	RF_FRAMEOVERRIDE=   8,
	RF_SNOWLAYER	=	16,
	RF_HITEFFECT	=	32,
	RF_CLOAKED		=	64
};

class CRenderThing;
class IRenderableThing;

class CMeshRenderer
{
public:
					CMeshRenderer();
	static	void	SetRenderMode(int rm);
	static	int		GetRenderMode();
	static	void	CalculateMeshHierarchy(IRenderableThing *t, FVector *p, FMatrix *ori, CMESHPART *mp, CMESHPART **rendermp, SINT *flags);
	static	void	RenderMeshHierarchy(FVector p, FMatrix ori, CMESHPART *mp, IRenderableThing *t, CRenderThing *rt=NULL, SINT skin = 0, SINT flags = 0);
	static	void	RenderMesh(FVector &p, FMatrix &ori, CMESH *mesh, IRenderableThing *t = NULL, CRenderThing *rt=NULL, SINT skin = 0, bool renderchain = true, SINT flags = 0);
	static	void	RenderMeshPart(FVector &p, FMatrix &ori, CMESHPART *mesh_part, IRenderableThing *t, CRenderThing *rt=NULL, SINT skin = 0, SINT flags = 0);
	static	void	SetFrameOverride(float f)	  { mFrameOverride=f; };
	static	void	SetAnimIndexOverride(int ai)  { mAnimIndexOverride=ai; };
	static	void	SetExtrusionDistance(float d) { mExtrusionDistance=d; };
	static	void	SetExtrusionVector(FVector f) { mExtrusionVector=f; };
	static	void	SetSingleTexture(CTEXTURE *t) { mSingleTexture=t; };
	static  CTEXTURE *GetSingleTexture() { return(mSingleTexture); };
	static	void	SetRenderAlpha(int a) { mRenderAlpha=a; };
	static	int		GetRenderAlpha() { return mRenderAlpha; };
	static	int		mCloakAlpha;
	static	DWORD	VectorToRGBA(FVector* v, float fHeight);
	static	float	clamp(float x);
	static	float	absf(float x);
	static	void	SetDisableSkeletalShaders(BOOL s) { mDisableSkeletalShaders=s; };
	static	BOOL	GetDisableSkeletalShaders() { return(mDisableSkeletalShaders); };
	static	UINT	GetSurfaceLOD() { return(mSurfaceLOD); };
	static	void	SetSurfaceLOD(UINT lod) { mSurfaceLOD=lod; };
	static	int		GetHitEffectStrength() { return(mHitEffectStrength); };
	static	void	SetHitEffectStrength(int s) { mHitEffectStrength=s; };
	static	void	RenderPose(FVector lpos,FMatrix lori,CMESH *mesh,IRenderableThing *t, CRenderThing *rt,SINT flags);
	static	void	SetMeshDistance(float d) { mMeshDistance=d; };
	static	float	GetMeshDistance() { return(mMeshDistance); };
	static	int		GetCustomAlphaRef() { return(mCustomAlphaRef); };
	static	void	SetCustomAlphaRef(UINT arf) { mCustomAlphaRef=arf; };
	
protected:
	static	BOOL	mDisableSkeletalShaders;
	static	int		mRenderMode;
	static	int		mRenderAlpha;
	static	float	mExtrusionDistance;
	static	FVector	mExtrusionVector;
	static	CTEXTURE *mSingleTexture;
	static	float	mFrameOverride;
	static	int		mAnimIndexOverride;	
	static	UINT	mSurfaceLOD;
	static	int		mHitEffectStrength;
	static	float	mMeshDistance;
	static	int		mCustomAlphaRef;
};

// --------------------------------------------------------------------------------

inline void	CMeshRenderer::SetRenderMode(int rm)
{
	mRenderMode=rm;
}

// --------------------------------------------------------------------------------

inline int		CMeshRenderer::GetRenderMode()
{
	return(mRenderMode);
}

#ifdef _DIRECTX

#include "DXMeshRenderer.h"
#define CMESHRENDERER CDXMeshRenderer

#elif TARGET == PS2

#include "PS2MeshRenderer.h"
#define CMESHRENDERER CPS2MeshRenderer

#endif

#endif