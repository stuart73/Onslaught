// Mesh pose computation code

#include "common.h"
#include "meshpose.h"
#include "meshrenderer.h"
#include "renderablething.h"
#include "renderthing.h"
#include "motioncontroller.h"
#include "game.h"

void	CMeshPose::UpdatePoseHierarchy(FVector p, FMatrix ori, CMESHPART *mp, IRenderableThing *t, CRenderThing *rt, SINT flags)
{
	CMESHPART *rendermp;
	
	SINT localflags=flags;
	
	CMeshRenderer::CalculateMeshHierarchy(t,&p,&ori,mp,&rendermp,&localflags);
	
	mPartOri[mp->mPartNumber]=ori;
	mPartPos[mp->mPartNumber]=p;
	mPartRenderPart[mp->mPartNumber]=rendermp->mPartNumber;
	mPartFlags[mp->mPartNumber]=localflags;
	
	// and the children
	SINT	c0;
	
	for (c0 = 0; c0 < mp->mNumChildren; c0 ++)
	{
		UpdatePoseHierarchy(p, ori, mp->mChildren[c0], t, rt, localflags);
	}
}

BOOL CMeshPose::UpdatePose(CMesh *mesh,IRenderableThing *t,CRenderThing *rt,BOOL important)
{
	// Check if we've actually moved, and if not then don't bother updating the pose

	int frame=(int) GAME.GetRenderFrameNumber();

	if (frame==mLastUpdateFrame) // Never update twice in one render frame
		return(FALSE);

//	if (t && (((CThing *)t)->IsA(THING_TYPE_MECH)))
//		important = TRUE;

	if (((frame)<(mLastUpdateFrame+30)) && (!important)) // Only allow non-critical updates every 30 frames
		return(FALSE);

	CMotionController *controller=t->GetMotionController();

	float fvframe=0.0f;
	int animind=t->GetRealAnimIndex();

	BOOL update=FALSE;

	if (mLastUpdateFrame=-9999)	// First update
		update=TRUE;
	
	if ((mesh->mNumAnimModes) && (animind>=0))
	{
		CAnimMode*	am = &mesh->mAnimModes[animind];
		
		if (am)
		{
			fvframe = t->GetRenderFrame();
			fvframe*=am->mFrameScaler; 
			fvframe+=am->mStart;
		}
	}
	
	if (mCurrentFVFrame!=fvframe)
		update=TRUE;
	
	if (controller)
		if (controller->IsUpdateNeeded())
			update=TRUE;
		
	if (!update)
		return(FALSE);

	UpdatePoseHierarchy(ZERO_FVECTOR,ID_FMATRIX,mesh->GetPart(0),t,rt,0);

	mLastUpdateFrame=frame;	

	mCurrentFVFrame=fvframe;

	return(TRUE);
}

void CMeshPose::GetPoseTransforms(CRenderThing *rt,IRenderableThing *t,CMeshPart *mp,FVector &outVector,FMatrix &outMatrix,BOOL inModelSpace,BOOL important)
{
	outVector=ZERO_FVECTOR;
	outMatrix=ID_FMATRIX;

	CMeshPose *pose=rt->GetPose();

	CMESH *mesh=rt->GetRTMesh();

	if (!mesh)
		return;
	
	if (!mp)
		return;
	
	if ((pose) && (GAME.mRenderFrameNumber>1)) // HACK - don't use pose controller before rendering starts because we might be in init (see Hive Boss)
	{
		// Use pose cache
		
		pose->UpdatePose(mesh,t,rt,important);
		
		outVector=pose->mPartPos[mp->mPartNumber];
		outMatrix=pose->mPartOri[mp->mPartNumber];
		
		if (!inModelSpace)
		{
			FVector lpos=t->GetRenderPos();
			FMatrix lori=t->GetRenderOrientation();
			
			outVector=(lori * outVector) + lpos;
			outMatrix=lori * outMatrix;
		}
	}
	else
	{
		// Figure it out properly :-(
		
		if (mp)
		{
			SINT f1;
			float offset;
			if (t->GetRealAnimIndex()>=0)
				f1=mp->ResolveFrameNumber(t->GetRenderFrame(),t->GetRealAnimIndex(),offset,t->GetMotionController());
			else
				f1=0;
			mp->GetWorldTransforms(f1,f1,0.0f,t,&outMatrix,&outVector,inModelSpace?true:false,true);
		}
	}
}
