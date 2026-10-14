// Mesh rendering code

#include "common.h"
#include "texture.h"

#include "MeshRenderer.h"
#include "renderablething.h"
#include "mesh.h"
#include "motioncontroller.h"
#include "engine.h"
#include "renderinfo.h"
#include "renderthing.h"
#include "particlemanager.h"
#include "polybucket.h"
#include "cliparams.h"
#include "meshpose.h"
#include "thing.h"
#include "rtmesh.h"

int		CMeshRenderer::mRenderMode=R_Normal;
int		CMeshRenderer::mRenderAlpha=255;
int		CMeshRenderer::mCloakAlpha=0;
float	CMeshRenderer::mExtrusionDistance=0.0f;
FVector CMeshRenderer::mExtrusionVector=ZERO_FVECTOR;
CTEXTURE *CMeshRenderer::mSingleTexture=NULL;
float	CMeshRenderer::mFrameOverride=0.0f;
int		CMeshRenderer::mAnimIndexOverride=0;
BOOL	CMeshRenderer::mDisableSkeletalShaders=FALSE;
UINT	CMeshRenderer::mSurfaceLOD=100;
int		CMeshRenderer::mHitEffectStrength=0;
float	CMeshRenderer::mMeshDistance=0.0f;
int		CMeshRenderer::mCustomAlphaRef=-1;

// --------------------------------------------------------------------------------

CMeshRenderer::CMeshRenderer()
{
	// Don't try to instantiate this!
	// Everything is static here...
	ASSERT(0);
}

// --------------------------------------------------------------------------------
//  Utility functions
// --------------------------------------------------------------------------------

float CMeshRenderer::clamp(float x)
{
	while (x<0)
		x-=1.0f;
	while (x>1.0f)
		x+=1.0f;
	return(x);
}

// --------------------------------------------------------------------------------

float CMeshRenderer::absf(float x)
{
	if (x<0)
		x=-x;
	return(x);
}

// --------------------------------------------------------------------------------

DWORD CMeshRenderer::VectorToRGBA(FVector* v, float fHeight)
{
    DWORD r = (DWORD)( 127.0f * v->X + 128.0f );
    DWORD g = (DWORD)( 127.0f * v->Y + 128.0f );
    DWORD b = (DWORD)( 127.0f * v->Z + 128.0f );
    DWORD a = (DWORD)( 255.0f * fHeight );
    
    return( (a<<24L) + (r<<16L) + (g<<8L) + (b<<0L) );
}

// --------------------------------------------------------------------------------

void	CMeshRenderer::CalculateMeshHierarchy(IRenderableThing *t, FVector *p, FMatrix *ori, CMESHPART *mp, CMESHPART **rendermp, SINT *flags)
{
	CMotionController *controller=NULL;
	if (t)
		controller=t->GetMotionController();

	// Figure out which animation frame we're on

	int vframe;
	float vframeoffset;

	int animind=-1;
	float fvframe=0.0f;

	if (t)
		animind=t->GetRealAnimIndex();
	if (((*flags) & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
		animind=mAnimIndexOverride;

	if ((mp->mNumVFrames >1) &&
	    (mp->mBaseMesh->mNumAnimModes)&&
	    (animind>=0))
	{
		CAnimMode*	am = &mp->mBaseMesh->mAnimModes[animind];
	
		if (am)
		{
			if (t)
				fvframe = t->GetRenderFrame();
			if (((*flags) & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
				fvframe=mFrameOverride;
			fvframe*=am->mFrameScaler; 
			fvframe+=am->mStart;
			if (controller)
				controller->UpdateRenderFrame(mp,&fvframe);
			vframe = (SINT(floorf(fvframe)));
			vframeoffset=(float) (fvframe-floorf(fvframe));
		}
		else
		{
			// Pretend there is no animation

			// THIS SHOULD NOT HAPPEN!

			printf("Warning - got null animmode in mesh %s in RenderMeshHierarchy\n",mp->mBaseMesh->mName);

			if (controller)
				controller->UpdateRenderFrame(mp,&fvframe);
			vframe = (SINT(floorf(fvframe)));
			vframeoffset=(float) (fvframe-floorf(fvframe));
		}
	}
	else
	{
		// No animation
		if (controller)
			controller->UpdateRenderFrame(mp,&fvframe);
		vframe = (SINT(floorf(fvframe)));
		vframeoffset=(float) (fvframe-floorf(fvframe));
	}

	if (vframe>=mp->mNumVFrames)
		vframe=mp->mNumVFrames-1;

	FVector lpos;
	FMatrix lori;

	UBYTE hframe1,hframe2;

	if (controller)
	{
		controller->UpdateRenderPositionPre(mp,p,ori,TRUE);
	}

	BOOL looping=TRUE;

	if (controller)
		looping=controller->IsAnimationLooping();

	// Get HFrame numbers

	hframe1 = mp->mVHFrameMap[vframe];
	int vframe2 = vframe + 1;
	if( vframe2 >= mp->mNumVFrames )
	{
		if (looping)
			vframe2=0;
		else
			vframe2 = mp->mNumVFrames-1;
	}
	hframe2 = mp->mVHFrameMap[vframe2];

	#if TARGET == PS2
		asm __volatile__
		(
		"
			lqc2		vf01,0x0(%1)		\n
			lqc2		vf02,0x0(%2)		\n
			qmtc2		%3,vf03				\n
			vnop							\n
			vnop							\n
			vsub		vf04,vf02,vf01		\n
			vnop							\n
			vnop							\n
			vnop							\n
			vmulax.xyz	ACC,vf04,vf03x		\n
			vmaddw.xyz	vf01,vf01,vf00w		\n
			vnop							\n
			vnop							\n
			vnop							\n
			sqc2		vf01,0x0(%0)		\n
		"
		: : "r" (&lpos), "r" (&mp->mHFramePositions[hframe1]), "r" (&mp->mHFramePositions[hframe2]), "r" (vframeoffset)
		: "memory"
		);

		FMatrix hfo1,hfo2;

		hfo1=mp->GetHFrameOrientation(hframe1);
		hfo2=mp->GetHFrameOrientation(hframe2);
		
		asm __volatile__
		(
		"
			lqc2		vf01,0x0(%1)		\n
			lqc2		vf02,0x10(%1)		\n
			lqc2		vf03,0x20(%1)		\n
			lqc2		vf04,0x0(%2)		\n
			lqc2		vf05,0x10(%2)		\n
			lqc2		vf06,0x20(%2)		\n
			qmtc2		%3,vf07				\n
			vnop							\n
			vnop							\n
			vsub.xyz	vf08,vf04,vf01		\n
			vsub.xyz	vf09,vf05,vf02		\n
			vsub.xyz	vf10,vf06,vf03		\n
			vnop							\n
			vmulax		ACC,vf08,vf07x		\n
			vmaddw		vf01,vf01,vf00w		\n
			vmulax		ACC,vf09,vf07x		\n
			vmaddw		vf02,vf02,vf00w		\n
			vmulax		ACC,vf010,vf07x		\n
			vmaddw		vf03,vf03,vf00w		\n
			sqc2		vf01,0x0(%0)		\n
			sqc2		vf02,0x10(%0)		\n
			vnop							\n
			sqc2		vf03,0x20(%0)		\n
		"
		: : "r" (&lori), "r" (&hfo1), "r" (&hfo2), "r" (vframeoffset)
		: "memory"
		);
	#else
		lpos=mp->mHFramePositions[hframe1]+((mp->mHFramePositions[hframe2]-mp->mHFramePositions[hframe1])*vframeoffset);
		lori=mp->GetHFrameOrientation(hframe1)+((mp->GetHFrameOrientation(hframe2)-mp->GetHFrameOrientation(hframe1))*vframeoffset);
	#endif

	// Technically we should normalise this, but that screws up because some of the 
	// matrices are flipped. In fact the normalisation doesn't really matter much, as
	// over the steps involved the error should be negligable.

	// hframeori.Normalise();
	
	// evaluate transforms
	if ((!t) || (t->RenderUseHierarchy(mp->mPartNumber)))
	{
#if TARGET == PS2
		asm __volatile__
		(
		"
			vaddw.xyz	vf31,vf00,vf00w		\n		# vector of 1s
			lqc2		vf01,0x0(%2)		\n		# load lpos
			lqc2		vf02,0x0(%1)		\n		# load ori 1
			lqc2		vf03,0x10(%1)		\n		# load ori 2
			lqc2		vf04,0x20(%1)		\n		# load ori 3
			lqc2		vf05,0x0(%0)		\n		# load p
			vmul.xyz	vf06,vf02,vf01		\n		# do matrix multiply
			vmul.xyz	vf07,vf03,vf01		\n	
			vmul.xyz	vf08,vf04,vf01		\n
			lqc2		vf09,0x0(%3)		\n		# load lori 1
			vadday.x	ACC,vf06,vf06y		\n
			vmaddz.x	vf06,vf31,vf06z		\n
			vaddax.y	ACC,vf07,vf07x		\n
			vmaddz.y	vf06,vf31,vf07z		\n
			vaddax.z	ACC,vf08,vf08x		\n
			vmaddy.z	vf06,vf31,vf08y		\n
			lqc2		vf10,0x10(%3)		\n		# load lori 2
			lqc2		vf11,0x20(%3)		\n		# load lori 3
			vmulax.xyz	ACC,vf09,vf02x		\n		# do matrix multiply
			vadd		vf05,vf05,vf06		\n		# add to p
			vmadday.xyz	ACC,vf10,vf02y		\n
			vmaddz.xyz	vf12,vf11,vf02z		\n
			vmulax.xyz	ACC,vf09,vf03x		\n
			sqc2		vf05,0x0(%0)		\n		# store p
			vmadday.xyz	ACC,vf10,vf03y		\n
			vmaddz.xyz	vf13,vf11,vf03z		\n
			vmulax.xyz	ACC,vf09,vf04x		\n
			vmadday.xyz	ACC,vf10,vf04y		\n
			vmaddz.xyz	vf14,vf11,vf04z		\n
			sqc2		vf12,0x0(%1)		\n		# store ori 1
			sqc2		vf13,0x10(%1)		\n		# store ori 2
			vnop							\n
			sqc2		vf14,0x20(%1)		\n		# store ori 3
		"
		: : "r" (&(p->X)), "r" (&(ori->Row[0].X)), "r" (&lpos.X), "r" (&lori.Row[0].X)
		);
#else
		(*p)  += (*ori) * lpos; 
		(*ori) = (*ori) * lori;
#endif
	}
	else
	{
		(*p)  += lpos;
		(*ori) = lori;
	}

	if (controller)
	{
		controller->UpdateRenderPositionPost(mp,p,ori,TRUE);
		controller->UpdateRenderFlags(mp,flags);
	}

	// Get substitute part if necessary

	*rendermp=mp;

	lpos=*p;
	lori=*ori;
	
	if (controller)
	{
		int substituenum=controller->GetSubstituteNumber(mp);
		
		if (substituenum!=0)
		{
			for (int i=0;i<substituenum;i++)
			{
				*rendermp=(*rendermp)->mNextMeshPartInChain;
				ASSERT(*rendermp);
			}			
			
			// Find the offset between where we are now and the substitute's base
			
			int vf2=vframe+1;
			if (vf2>=(*rendermp)->mNumVFrames)
				vf2=(*rendermp)->mNumVFrames-1;
			
			(*rendermp)->GetWorldTransforms(vframe,vf2,vframeoffset,t,&lori,&lpos,TRUE,TRUE);
			
		}
	}

	// Alternative (slower) method of getting position and orientation
	// Only really useful for debugging the GetWorldTransforms() function
/*
	FMatrix nori;
	FVector np;

	if (t)
		mp->GetWorldTransforms(vframe,vframe+1,vframeoffset,t,&nori,&np,false,true);
	else
	{
		np = p;
		nori = ori;
	}

	RenderMeshPart(np, nori, rendermp, t, rt, skin, flags);*/

	// Write back results

	*p=lpos;
	*ori=lori;
}

// --------------------------------------------------------------------------------
// The unposed version of the renderer
// --------------------------------------------------------------------------------

void	CMeshRenderer::RenderMeshHierarchy(FVector p, FMatrix ori, CMESHPART *mp, IRenderableThing *t, CRenderThing *rt, SINT skin, SINT flags)
{
	CMESHPART *rendermp;

	SINT localflags=flags;

	if ((!mp->mParent) && (t))
	{
		p=(t->GetRenderOrientation() * p) + t->GetRenderPos();
		ori=t->GetRenderOrientation() * ori;		
	}

	CalculateMeshHierarchy(t,&p,&ori,mp,&rendermp,&localflags);

	RenderMeshPart(p, ori, rendermp, t, rt, skin,flags);
	
	// and the children
	SINT	c0;
	
	for (c0 = 0; c0 < mp->mNumChildren; c0 ++)
	{
		RenderMeshHierarchy(p, ori, mp->mChildren[c0], t, rt, skin,flags);
	}
}

// --------------------------------------------------------------------------------

void	CMeshRenderer::RenderPose(FVector lpos,FMatrix lori,CMESH *mesh,IRenderableThing *t, CRenderThing *rt,SINT flags)
{
	CMeshPose *pose=rt->GetPose();
	if (!pose)
		return;

	lpos=(t->GetRenderOrientation() * lpos) + t->GetRenderPos();
	lori=t->GetRenderOrientation() * lori;

	for (int i=0;i<mesh->GetNumParts();i++)
	{
		if (pose->mPartRenderPart[i]!=-1)
		{
			FVector pos=pose->mPartPos[i];
			FMatrix ori=pose->mPartOri[i];

			pos=(lori * pos) + lpos;
			ori=lori * ori;

			RenderMeshPart(pos,ori,mesh->GetPart(pose->mPartRenderPart[i]),t,rt,0,pose->mPartFlags[i] | flags);
		}			
	}
}

// --------------------------------------------------------------------------------

void	CMeshRenderer::RenderMesh(FVector &p, FMatrix &ori, CMESH *mesh, IRenderableThing *t, CRenderThing *rt, SINT skin, bool renderchain, SINT flags)
{
//	return;

	if (mesh)
	{
		// Uncomment this to draw mesh (rather than mesh part) bounding boxes

/*		if (ENGINE.GetDrawDebugStuff() & DRAW_OBJECTS_AS_CUBOIDS)
		{
			FVector pos = p ;
			FVector v = mesh->mBoundingBox->mOrigin ;
			FMatrix m = ID_FMATRIX ;

			if (t)
			{
				p  += t->GetRenderPos();
				ori = ori * t->GetRenderOrientation();
			}

			RENDERINFO.SetWorld(p, ori);

			DWORD	col = 0xff7f7f7f;
			if (t)
				col = t->GetRenderColour();

			ENGINE.DrawDebugCuboid(col, mesh->mBoundingBox->mAxes, v, m, ENGINE.GetOpaqueTexture());
			return ; 
		}*/

		if (mesh->GetNumParts()>0)
		{
			ASSERT(mesh->GetPart( 0 )->mParent == NULL); // first part should be the parent

			CMeshPose *pose=NULL;
			
			if (rt)
				pose=rt->GetPose();

			if (pose)
			{
				// We can draw through the pose cache
				pose->UpdatePose(mesh,t,rt,TRUE);
				RenderPose(p,ori,mesh,t,rt,flags);
			}
			else
			{
				// No pose - draw normally
				RenderMeshHierarchy(p,ori, mesh->GetPart( 0 ), t, rt, skin,flags);
			}			

		}
		if ((renderchain) && (mesh->mNextInChain))
		{
			// Render chained mesh
			RenderMesh(p,ori,mesh->mNextInChain,t,rt,skin,renderchain,flags);
		}
	}
}
 
// --------------------------------------------------------------------------------

void	CMeshRenderer::RenderMeshPart(FVector &p, FMatrix &ori, CMESHPART *mesh_part, IRenderableThing *t, CRenderThing *rt,SINT skin, SINT flags)
{
	if (((mesh_part->mPartType==MESHPART_TYPE_NULL) || (mesh_part->mPartType==MESHPART_TYPE_CAMERA)) &&
        ((flags & RF_BOUNDINGBOX)==0))
	{
		// Don't render NULL parts or cameras (unless RF_BOUNDINGBOX is set)
		return;
	}

	if ((flags & RF_DONTRENDER)==RF_DONTRENDER)
	{
		// Don't render if the flag is set
		return;
	}

	if ((flags & RF_SNOWLAYER)==RF_SNOWLAYER)
	{
		int animind=-1;
		
		if (t)
			animind=t->GetRealAnimIndex();

		// This should work, but some things appear to be all animated when
		// only parts should be....

		if ((animind!=-1) && ((mesh_part->mNumHFrames>1) || (mesh_part->mNumAFrames>1)))
		{
			// It's animated, so no snow
			return;
		}
	}

	if (mesh_part->mPartType==MESHPART_TYPE_EMITTER)
	{	
		// Emitter

		CRTMesh *rtm=(CRTMesh *) rt;

		if (rt)
		{
			int effectno=rt->GetEffectNumberForPart(mesh_part);
			if (effectno!=-1)
			{				
				if (rtm->mEffectPD[effectno])
				{
					if (!rtm->mEffectFOR[effectno])
					{
						rtm->mEffectFOR[effectno]=new (MEMTYPE_PARTICLE) CValidatedFoR();
						rtm->mEffectFOR[effectno]->Reset();
						PARTICLE_MANAGER.AddParticle(rtm->mEffectPD[effectno],rtm->mEffectFOR[effectno],FVector(0,0,0), TRUE); // Last TRUE means the FoR *HAS* to be updated each frame, otherwise the particles won't be drawn
						
						rtm->mEffectFOR[effectno]->SetPos(FVector(0.0f, 0.0f, 100.0f));
						rtm->mEffectFOR[effectno]->SetPos(FVector(0.0f, 0.0f, 100.0f));
						rtm->mEffectInitialised[effectno]=FALSE;
					}
					
					if (!rt->GetEffectInitialised(effectno))
					{
						int flags=rt->GetEffectFlags(effectno);
						
						rt->SetEffectInitialised(effectno,true);
						
						CValidatedFoR *f=rt->GetEffectFOR(effectno);
						
						if (f)
						{
							// twice to prevent interpolation (we're already doing it here...)
							f->SetPos(p);
							f->SetPos(p);
							
							if (((flags | EFFECT_FLAG_USE_ORIENTATION)==EFFECT_FLAG_USE_ORIENTATION) ||
								((flags | EFFECT_FLAG_SET_ORIENTATION)==EFFECT_FLAG_SET_ORIENTATION))
							{
								// twice to prevent interpolation (we're already doing it here...)
								f->SetOrientation(ori);				
								f->SetOrientation(ori);				
							}						
						}
					}
					else
					{
						CValidatedFoR *f=rt->GetEffectFOR(effectno);
						
						if (f)
						{
							int flags=rt->GetEffectFlags(effectno);
							
							// twice to prevent interpolation (we're already doing it here...)
							f->SetPos(p);
							f->SetPos(p);
							if ((flags | EFFECT_FLAG_USE_ORIENTATION)==EFFECT_FLAG_USE_ORIENTATION)
							{
								// twice to prevent interpolation (we're already doing it here...)
								f->SetOrientation(ori);
								f->SetOrientation(ori);
							}
						}
					}
				}
			}
		}

		if ((flags & RF_BOUNDINGBOX)==0)	// Don't render anything
			return; 
	}
	
	bool wasreference=false;

	if (mesh_part->mPartType==MESHPART_TYPE_REFERENCE)
	{
		// Reference
		mesh_part=mesh_part->mReferenceTo;
		wasreference=true;
	}

	RENDERINFO.SetWorld(p, ori /* *scale */);

	int should_do_hit_effect = 0 ;
	if (t)
	{
		should_do_hit_effect = t->GetShouldDoHitEffect(mesh_part->mPartNumber) ;
	}

	if (should_do_hit_effect>0)
	{
		flags=flags | RF_HITEFFECT;
		SetHitEffectStrength(should_do_hit_effect);
	}
	else
		flags=flags & ~RF_HITEFFECT;

	if ((ENGINE.GetDrawDebugStuff() & DRAW_OBJECTS_AS_CUBOIDS) ||
		((flags & RF_BOUNDINGBOX)==RF_BOUNDINGBOX))
	{
		FVector pos = p ;
		FVector v = mesh_part->mBoundingBox->mOrigin ;
		FMatrix m = ID_FMATRIX ;

		DWORD	col = 0xff7f7f7f;
		if (t)
			col = t->GetRenderColour();
		if (wasreference)
			col=0xff008080;

		FVector axes;

		if (mesh_part->mBoundingBox->mRadius==0.0f)
			axes.Set(0.1f,0.1f,0.1f);
		else
			axes=mesh_part->mBoundingBox->mAxes;
		
		ENGINE.DrawDebugCuboid(col, axes, v, m, ENGINE.GetOpaqueTexture());
		return;
	}

	if (ENGINE.mDrawPolyBuckets)
	{
		if (!mesh_part->mPolyBucket)
			return;

		mesh_part->mPolyBucket->Render();
		
		return;		
	}
	
	CMotionController *controller=NULL;
	if (t)
		controller=t->GetMotionController();

#if TARGET==PC
#ifdef DEV_VERSION	
	if (((flags & RF_HILIGHT)!=0) && (CMESHRENDERER::GetRenderMode()!=R_Shadow) && (CLIPARAMS.mModelViewer))
	{
		// Render with hilight outline
		
		RENDERINFO.SetShiftEnabled(true);
		
		float size=fabsf(sinf(ENGINE.GetRenderTime()*2.0f)*0.015f);
		
		RENDERINFO.SetDiffuseBlendColour(0xFF00FF00);
		RENDERINFO.SetDiffuseBlendFactor(255);
		SetSingleTexture(ENGINE.GetHilightTexture());
		
		RENDERINFO.SRS(RS_ZWRITEENABLE,FALSE);
		RENDERINFO.SRS(RS_SRCBLEND,BLEND_ONE);
		RENDERINFO.SRS(RS_DESTBLEND,BLEND_ZERO);
		SetRenderMode(R_NoTexture);
		ENGINE.GetHilightTexture()->SelectTexture();
		LT.SRS(D3DRS_CULLMODE,D3DCULL_CW);
		
		RENDERINFO.SetShiftFactor(FVector(size,0,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		RENDERINFO.SetShiftFactor(FVector(-size,0,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		RENDERINFO.SetShiftFactor(FVector(0,size,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		RENDERINFO.SetShiftFactor(FVector(0,-size,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		SetRenderMode(R_Normal);
		ENGINE.GetHilightTexture()->ShutdownRenderMode();
		RENDERINFO.SRS(RS_ZWRITEENABLE,TRUE);
		RENDERINFO.SRS(RS_SRCBLEND,BLEND_SRCALPHA);
		RENDERINFO.SRS(RS_DESTBLEND,BLEND_INVSRCALPHA);
		LT.SRS(D3DRS_CULLMODE,D3DCULL_CCW);
		RENDERINFO.SetDiffuseBlendFactor(0);
		RENDERINFO.SetShiftEnabled(false);
					
	//	CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags & ~RF_HILIGHT,controller,wasreference);			

		// Render into Z-Buffer
		
		LT.SRS(D3DRS_COLORWRITEENABLE,0);
		
		SetRenderMode(R_NoTexture);
		RENDERINFO.SRS(RS_SRCBLEND,BLEND_ONE);
		RENDERINFO.SRS(RS_DESTBLEND,BLEND_ZERO);
		ENGINE.GetHilightTexture()->SelectTexture();
		LT.SRS(D3DRS_CULLMODE,D3DCULL_CW);
		RENDERINFO.SetShiftEnabled(true);
		
		RENDERINFO.SetShiftFactor(FVector(size,0,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		RENDERINFO.SetShiftFactor(FVector(-size,0,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		RENDERINFO.SetShiftFactor(FVector(0,size,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		RENDERINFO.SetShiftFactor(FVector(0,-size,0));
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
		
		SetRenderMode(R_Normal);
		RENDERINFO.SRS(RS_SRCBLEND,BLEND_SRCALPHA);
		RENDERINFO.SRS(RS_DESTBLEND,BLEND_INVSRCALPHA);
		RENDERINFO.SetShiftEnabled(false);
		
		LT.SRS(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);			
		
		LT.SRS(D3DRS_CULLMODE,D3DCULL_CCW);
	}
	else
#endif
#endif
	{
		// Normal
		
		CMESHRENDERER::DrawMeshPart(p,ori,mesh_part,t,rt,skin,flags,controller,wasreference);
	}
}
