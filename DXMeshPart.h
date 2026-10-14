// DXMeshPart.h: interface for the DXMeshPart class.
//
//////////////////////////////////////////////////////////////////////

#ifndef DXMESHPART_H
#define DXMESHPART_H

#include "MeshPart.h"

#ifndef SIMPLE_MESHES
#include "MeshVB.h"
#endif

#include "MemBuffer.h"

class CDXMesh;
class CMeshTexture;
class CMeshTriangle;
class CMeshDVertex;
class CPolyBucket;
class IRenderableThing;

class	CDXMeshPart : public CMeshPart// : public IMeshPart
{
public:

//	struct			EBREdge*		mEBR;				// Edge based version for shadow stuff
//	int				mNEBREdges;
//	void			MakeEBR();
//	void			AddEdge(UWORD va, UWORD vb, UWORD faceindex);


//	CCollisionVolume	*mCollisionVolume;
};

#endif // DXMESHPART_H
