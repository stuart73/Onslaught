// MeshVB.h: interface for the CMeshVB class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _MESHVB_H_
#define _MESHVB_H_

// This is the number of bone "slots" used by the optimised renderer
// Again, don't change unless you also edit the shader stuff

#define BONE_SLOTS 3

#define	MAX_MESH_MAT_PARTS	64 // MAX materials per mesh

class CMeshVB  
{
};

#ifdef _DIRECTX

#include	"DXMeshVB.h"
#define CMESHVB CDXMeshVB

#elif TARGET == PS2

#include	"PS2MeshVB.h"
#define CMESHVB CPS2MeshVB

#endif

#endif // MESHVB_H
