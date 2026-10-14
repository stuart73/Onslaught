#ifndef PCMESHVB_H
#define PCMESHVB_H

#include	"deviceobject.h"
#include	"MeshVB.h"

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
class	CPCMeshPart;

//**********************************************************************
struct	SPCMeshMatPart
{
    LPDIRECT3DVERTEXBUFFER8	mVBuff;
    LPDIRECT3DINDEXBUFFER8	mIBuff;
	
	ULONG			mVBSize;
	ULONG			mIBSize;

	SINT			mNumIndices;
	SINT			mNumVertices;
	SINT			mNumPrimitives;

	BOOL			mActive;

	int				mTextureRenderMode;
	CMeshTexture	*mTexture;
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
class	CPCMeshVB : public CMeshVB, public DeviceObject
{
public:
	CPCMeshVB();
	~CPCMeshVB();
	
    virtual	HRESULT		InitDeviceObjects();
    virtual	HRESULT		DeleteDeviceObjects();

	void	Render(IRenderableThing *t,SINT flags,FMatrix *ori,FVector *pos);

	void	SetMeshPart(CPCMeshPart *mp) {mMeshPart = mp;};
	HRESULT LoadAndCreateShader(char *strFilePath, DWORD* pDeclaration, DWORD Usage, DWORD* pHandle);
	
	void				Serialize(class CChunker *c,class CResourceAccumulator *ra);
	void				Deserialize(class CChunkReader *c);

protected:

	void				SerializeForPC(class CChunker *c,class CResourceAccumulator *ra);	
	void				SerializeForPS2(class CChunker *c,class CResourceAccumulator *ra);	
	
	SPCMeshMatPart	mVBPart[MAX_MESH_MAT_PARTS];
	SINT			mNumTextures;
	CPCMeshPart		*mMeshPart;
	class CVertexShader	*mCurrentShader;		// Current vertex shader (if any)
	int				mVertexBufferChunkSize; // Size of each vertex entry in the VBuffer
	DWORD			mFVFFlags;				// FVF flags
	D3DPRIMITIVETYPE	mPrimitiveType;		// Primitive type

	HRESULT			BuildSkeletalVB();
	HRESULT			BuildNormalVB();

	static	int		mTotalTriangles;
	static	int		mTotalStrips;
	static	int		mMaxStripLength;
};


#endif