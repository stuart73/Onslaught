#ifndef DXMESHVB_H
#define DXMESHVB_H

#include	"dx.h"
#include	"deviceobject.h"
#include	"MeshVB.h"
#include	"MeshTypes.h"

//#include	"VertexShader.h"

// Maximum number of bones allowed by skeletal animation system
// Note that you need SKELETAL_SHADER_MATRIX_ROWS*(MAX_SHADER_BONES+1)
// free constant slots

// Also note that MAX_SOFTWARE_BONES must be greater than 
// MAX_SHADER_BONES

#define MAX_SHADER_BONES 25
#define MAX_SOFTWARE_BONES 100

//**********************************************************************
class	CMeshTexture;
class	IRenderableThing;
class	CDXMeshPart;

//**********************************************************************
struct	SDXMeshMatPart
{
    LPDIRECT3DVERTEXBUFFER8	mVBuff;
    CBEIndexBuffer *mIBuff;
	
	ULONG			mVBSize;
	ULONG			mIBSize;

	SINT			mNumIndices;
	SINT			mNumVertices;
	SINT			mNumPrimitives;

	BOOL			mActive;

	CMeshTexture	*mTexture[TEXTURE_TYPES];
	void			*mpVBData;
};

//**********************************************************************
// Vertex structure for skeletal models

class CSKELETAL_VB
{
public:
	FVector3 mPos;
	ULONG	mDiffuse;
	float	mU, mV;
	FVector3	mNormal;
//	float	mWeight[MAX_SHADER_BONES];
	float	mBoneSlot[MAX_SHADER_BONES];
};

//**********************************************************************
// Vertex buffer structure for indexed blending

class CIBLEND_FVF
{
public:
	FVector3	mPos;	
    float   mBlend;
	float	mBlend2;
//	DWORD	mMatrixIndices;
	FVector3	mNormal;
//	ULONG	mDiffuse;
//	float	mU, mV;
};

#define IBLEND_FVF_FLAGS (D3DFVF_XYZB2 | D3DFVF_NORMAL)
// | D3DFVF_DIFFUSE | D3DFVF_TEX1)

//**********************************************************************
// Device independent primitive type (because enums are different between PC and xbox

enum EPrimitiveType
{
	PRIMTYPE_POINTLIST,
	PRIMTYPE_LINELIST,
	PRIMTYPE_LINESTRIP,
	PRIMTYPE_TRIANGLELIST,
	PRIMTYPE_TRIANGLESTRIP,
	PRIMTYPE_TRIANGLEFAN,
};

//**********************************************************************
class	CDXMeshVB : public CMeshVB, public DeviceObject
{
public:
	CDXMeshVB();
	~CDXMeshVB();
	
    virtual	HRESULT		InitDeviceObjects();
    virtual	HRESULT		DeleteDeviceObjects();

	void	Render(IRenderableThing *t,SINT flags,FMatrix *ori,FVector *pos);
	void	CoreRender(int part, int texture);

	void	SetMeshPart(CDXMeshPart *mp) {mMeshPart = mp;};
	HRESULT LoadAndCreateShader(char *strFilePath, DWORD* pDeclaration, DWORD Usage, DWORD* pHandle);
	
	void				Serialize(class CChunker *c,class CResourceAccumulator *ra);
	void				Deserialize(class CChunkReader *c);
	static				D3DPRIMITIVETYPE ToD3DPrimType(EPrimitiveType primtype);
	static				EPrimitiveType FromD3DPrimType(D3DPRIMITIVETYPE primtype);
	
	int		GetPolys();
	int		GetParts();

protected:

	void				SerializeForDX(class CChunker *c,class CResourceAccumulator *ra);	
	void				SerializeForPS2(class CChunker *c,class CResourceAccumulator *ra);	
	
	SDXMeshMatPart	*mVBPart[MAX_MESH_MAT_PARTS];
	SINT			mNumTextures;
	CDXMeshPart		*mMeshPart;
	class CVertexShader	*mCurrentShader;		// Current vertex shader (if any)
	int				mVertexBufferChunkSize; // Size of each vertex entry in the VBuffer
	DWORD			mFVFFlags;				// FVF flags
	EPrimitiveType	mPrimitiveType;		// Primitive type

	HRESULT			BuildSkeletalVB();
	HRESULT			BuildNormalVB();

	static	int		mTotalTriangles;
	static	int		mTotalStrips;
	static	int		mMaxStripLength;
};


#endif