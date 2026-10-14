// Mesh pose information

#ifndef MESHPOSE_H
#define MESHPOSE_H

#include "meshpart.h"

class CMeshPose
{
public:
	CMeshPose()
	{
		mNumParts=0;
		mPartPos=NULL;
		mPartOri=NULL;
		mPartFlags=NULL;
		mPartRenderPart=NULL;
		mCurrentFVFrame=-99999.0f;
		mLastUpdateFrame=-9999;
	}

	~CMeshPose()
	{
		SAFE_DELETE_ARRAY(mPartOri);
		SAFE_DELETE_ARRAY(mPartPos);
		SAFE_DELETE_ARRAY(mPartFlags);
		SAFE_DELETE_ARRAY(mPartRenderPart);
	}

	void Create(int numparts)
	{
		mNumParts=numparts;
		mPartOri=new (MEMTYPE_POSE) FMatrix[numparts];
		mPartPos=new (MEMTYPE_POSE) FVector[numparts];
		mPartFlags=new (MEMTYPE_POSE) SINT[numparts];
		mPartRenderPart=new (MEMTYPE_POSE) SINT[numparts];

		for (int i=0;i<mNumParts;i++)
			mPartRenderPart[i]=-1;
	}

	void UpdatePoseHierarchy(FVector p, FMatrix ori, CMESHPART *mp, class IRenderableThing *t, class CRenderThing *rt, SINT flags);
	BOOL UpdatePose(class CMesh *mesh,class IRenderableThing *t,class CRenderThing *rt,BOOL important);
	static void GetPoseTransforms(CRenderThing *rt,IRenderableThing *t,CMeshPart *mp,FVector &outVector,FMatrix &outMatrix,BOOL inModelSpace,BOOL important);

	FMatrix				*mPartOri;
	FVector				*mPartPos;
	SINT				*mPartFlags;
	int					*mPartRenderPart;
	int					mNumParts;
	int					mLastUpdateFrame;
	float				mCurrentFVFrame;
};

#endif