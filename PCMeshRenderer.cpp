// PCMeshRenderer.cpp: implementation of the CPCMeshRenderer class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"

#if TARGET == PC

#include "DX.h"
#include "platform.h"
#include "mesh.h"
#include "thing.h"
#include "d3dutils.h"
#include "engine.h"
#include "vertexshader.h"
#include "PCMeshRenderer.h"
#include "profile.h"
#include "vbuftexture.h"
#include "motioncontroller.h"
#include "particlemanager.h"
#include "renderqueue.h"
#include "renderinfo.h"

// --------------------------------------------------------------------------------

void	CPCMeshRenderer::DrawMeshPart(FVector p, FMatrix &ori, CMESHPART *mesh_part, IRenderableThing *t, CRenderThing *rt,SINT skin, SINT flags, CMotionController *controller, BOOL wasreference)
{
	SINT		c0;	
	
	int		animind;
	
	LT.SRS(D3DRS_ALPHABLENDENABLE,FALSE);	// **HACK! Remove me!
	
	if (mesh_part->mPartType==MESHPART_TYPE_MESH)
	{
		// Normal mesh

		animind=-1;
	
		if (t)
			animind=t->GetRealAnimIndex();
		if ((flags & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
			animind=mAnimIndexOverride;

		// Following code is for vertex animation only
		if ((mesh_part->mNumVFrames >1) &&
			(mesh_part->mBaseMesh->mNumAnimModes) && 
			(mesh_part->mNumAFrames >1) &&
			(animind>=0))
		{
			// Vertex-animated mesh
			float	vframe=0.0f;
			if (t)
				vframe = t->GetRenderFrame();
			if ((flags & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
				vframe=mFrameOverride;
			CAnimMode*	am = &mesh_part->mBaseMesh->mAnimModes[animind];
			vframe*=am->mFrameScaler; // this fraction varies for looped and unlooped animations
									// see loader code
			vframe+=am->mStart;

			if (controller)
				controller->UpdateRenderFrame(mesh_part,&vframe);

			SINT	vf1 = (SINT(floor(vframe))) % mesh_part->mNumVFrames;		//### % shouldn't be needed
			SINT	vf2 = vf1 + 1;
			if(vf2 > am->mLast) vf2 = am->mStart;

			// now lookup into the vframes table
			float	af1 = mesh_part->mVFrames[vf1];
			float	af2 = mesh_part->mVFrames[vf2];

			if (af2 < af1) af2 = float(SINT(floor(af1)) + 1);

			float	vfwhole= floorf(vframe);
			float	vffrac = vframe-vfwhole;
			float	aframe = (af2 * vffrac) + (af1 * (1 - vffrac));

			SINT	f1 = SINT(floor(aframe));
			SINT	f2 = f1 + 1;
			if	(f2 > SINT(floor(mesh_part->mVFrames[am->mLast]))) f2 = SINT(floor(mesh_part->mVFrames[am->mStart]));

			FVector *pverts1 = mesh_part->mPVertices[f1];
			FVector *pverts2 = mesh_part->mPVertices[f2];

			float	frac2 = aframe - floorf(aframe);
			float	frac1 = (1.f - frac2);

			for (c0 = 0; c0 < mesh_part->mNumDVertices; c0++)
			{
				CMeshDVertex *v	= &mesh_part->mDVertices[c0];

				CFVF_GTEX		vert;
				
				vert.mPos		= (pverts1[v->mPVert] * frac1) + (pverts2[v->mPVert] * frac2);
				vert.mU			= v->mU;
				vert.mV			= v->mV;
				vert.mNormal	= v->mNormal;
				vert.mDiffuse	= v->mDiffuse; 

				if (mRenderAlpha!=255)
				{
					int a = vert.mDiffuse >> 24;
					a=(a*mRenderAlpha)/255;
					vert.mDiffuse=(a<<24) | (vert.mDiffuse & 0x00FFFFFF);
				}

				if ((flags & RF_HILIGHT)==RF_HILIGHT)
					vert.mDiffuse=0xFFFF0000;

				if (mRenderMode==R_BumpmapNormals)
				{
					vert.mDiffuse = VectorToRGBA(&v->mNormal,1.0f);
				}

				v->mIndex[0]=mesh_part->mBaseMesh->mTextures[v->mTexture[0]].mVBuffer->AddVertices(&vert,1);
			}
		
			for (c0 = 0; c0 < mesh_part->mNumTriangles; c0++)
			{
				CMeshTriangle *t = &mesh_part->mTriangles[c0];

				ASSERT(t->mA->mTexture[0]!=-1);
				ASSERT(t->mA->mTexture[0] == t->mB->mTexture[0]);
				ASSERT(t->mA->mTexture[0] == t->mC->mTexture[0]);

				UWORD index[3];

				index[0]=t->mA->mIndex[0];
				index[1]=t->mB->mIndex[0];
				index[2]=t->mC->mIndex[0];

				mesh_part->mBaseMesh->mTextures[t->mA->mTexture[0]].mVBuffer->AddIndices(&index,3);
			}

			MESH_TEXTURE::DrawAllInMesh(mesh_part->mBaseMesh);
		}
		else
		{
			if ((mRenderMode==R_Normal) || (mRenderMode==R_Shadow))
			{
				mesh_part->mMeshVB->Render(t,flags,&ori,&p);
			}
			else
			{
				D3DVIEWPORT8 vp;
				D3DMATRIX	pr, wo, vi;
				float swidth,sheight;
				float stwidth,stheight;
				float twidth,theight;

				int vf1=0;
				float	vframe=0;
				int animind=-1;
				if (t)
					animind= t->GetRealAnimIndex();
				if ((flags & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
					animind=mAnimIndexOverride;
				if (animind>=0)
				{
					if (t)
						vframe=t->GetRenderFrame();
					if ((flags & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
						vframe=mFrameOverride;
					CAnimMode*	am = &mesh_part->mBaseMesh->mAnimModes[animind];
					vframe*=am->mFrameScaler;
					vframe+=am->mStart;
					vf1=((int) floor(vframe)) % mesh_part->mNumVFrames;
				}

				FVector *pverts1 = mesh_part->mPVertices[0];
				MESH_TEXTURE *tex;

				if (mRenderMode==R_Cloaked)
				{
					tex=MESH_TEXTURE::GetTextureByTex(ENGINE.GetScreenTexture());

					// As it won't necessarily have been set up...

					tex->SetupV(FVF_GTEX_FLAGS,D3DUSAGE_WRITEONLY,sizeof(CFVF_GTEX),D3DPT_TRIANGLELIST,D3DPOOL_MANAGED);
					tex->SetupI(D3DFMT_INDEX16,D3DUSAGE_WRITEONLY,2,D3DPOOL_MANAGED);

					LT.D3D_GetViewport( &vp );

					RENDERINFO.GetProjection(&pr);
					RENDERINFO.GetWorld(&wo);
					RENDERINFO.GetView(&vi);

					swidth=(float) ENGINE.GetCurrentViewportWidth();
					sheight=(float) ENGINE.GetCurrentViewportWidth();
					twidth=(float) tex->mTexture->GetWidth();
					theight=(float) tex->mTexture->GetHeight();
					stwidth=swidth/twidth;
					stheight=sheight/theight;
				}

				if (mRenderMode==R_Extruded)
				{
					tex=MESH_TEXTURE::GetTextureByTex(mExtrusionTexture);
					
					// As it won't necessarily have been set up...
					
					tex->SetupV(FVF_GTEX_FLAGS,D3DUSAGE_WRITEONLY,sizeof(CFVF_GTEX),D3DPT_TRIANGLELIST,D3DPOOL_MANAGED);
					tex->SetupI(D3DFMT_INDEX16,D3DUSAGE_WRITEONLY,2,D3DPOOL_MANAGED);
				}					

				int passes=TEXTURE_TYPES;

				if ((mRenderMode==R_Cloaked) || (mRenderMode==R_Extruded))
					passes=1;

				FVector extrusionvector;
				if (mRenderMode==R_Extruded)
				{
					extrusionvector=mExtrusionVector;
					extrusionvector.Normalise();
					extrusionvector=ori.Inverse() * extrusionvector; // Rotate into part space
				}

				for (c0 = 0; c0 < mesh_part->mNumDVertices; c0++)
				{
					CMeshDVertex *v	= &mesh_part->mDVertices[c0];

					for (int i=0;i<passes;i++)
					{
						if ((v->mTexture[i]!=-1) && (mesh_part->mBaseMesh->mTextures[v->mTexture[i]].mMaxBlend>0.0f))
						{
							CFVF_GTEX		vert;

							if ((mRenderMode!=R_Cloaked) && (mRenderMode!=R_Extruded))
								tex=mesh_part->mBaseMesh->mTextures[v->mTexture[i]].mVBuffer;
					
							vert.mPos		= pverts1[v->mPVert];
							vert.mU			= v->mU;
							vert.mV			= v->mV;
							vert.mNormal	= v->mNormal;
							vert.mDiffuse	= v->mDiffuse;

							if (mRenderMode==R_Extruded)
							{									
								if ((v->mSmoothNormal * extrusionvector)<0)
								{
									FVector vt= extrusionvector*mExtrusionDistance;
									vert.mPos.X-= vt.X ;
									vert.mPos.Y-= vt.Y ;
									vert.mPos.Z-= vt.Z ;
									vert.mDiffuse=0x00000000;
								}
								else
									vert.mDiffuse=0xFF000000;
								vert.mU=vert.mPos.X/10.0f;
								vert.mV=vert.mPos.Y/10.0f;							

								//vert.mPos+=v->mSmoothNormal*0.05f;
							}
/*
							static float expandovision=0.0f;

							vert.mPos+=v->mNormal*expandovision;

							if (c0==0)
							{
								expandovision+=0.001f;
								if (expandovision>1.0f)
									expandovision=0.0f;
							}
*/
							// mTextureBlend[0] is acceptable here as we *know* at this point
							// that this isn't

							int alpha=(vert.mDiffuse >> 24) & 0xFF;
							alpha=(int) ((float) alpha*mesh_part->mBaseMesh->mTextures[v->mTexture[i]].mTextureBlend[(vf1 % mesh_part->mBaseMesh->mTextures[v->mTexture[i]].mFrames)]);
							if (mRenderAlpha!=255)
							{
								alpha=(alpha*mRenderAlpha)/255;
							}
							vert.mDiffuse	= (vert.mDiffuse & 0x00FFFFFF) + (alpha<<24);

							if ((mRenderMode==R_BumpmapNormals) || (mRenderMode==R_SphericalBumpmapNormals))
							{
								FVector n;
								n.X =vert.mNormal.X;
								n.Y =vert.mNormal.Y;
								n.Z =vert.mNormal.Z;
								FVector worldspacenormal=ori * n;

								if (mRenderMode==R_SphericalBumpmapNormals)
								{
									FVector vt1;
									vt1.X = vert.mPos.X ;
									vt1.Y = vert.mPos.Y ;
									vt1.Z = vert.mPos.Z ;

									worldspacenormal=ori * vt1;
								}

								worldspacenormal.Normalise();
								
								vert.mDiffuse = VectorToRGBA(&worldspacenormal,1.0f);
							}

							if (mRenderMode==R_Cloaked)
							{
								FVector	spos;
								FVector3 wpos = vert.mPos;
								D3DXVec3Project((D3DXVECTOR3 *)(&spos), (CONST D3DXVECTOR3 *)&wpos, &vp,
												(CONST D3DXMATRIX *)&pr, (CONST D3DXMATRIX *)&vi, (CONST D3DXMATRIX *)&wo);

								vert.mU=spos.X/twidth;
								vert.mV=spos.Y/theight;

								if (vert.mU<0.0f)
									vert.mU=0.0f;
								if (vert.mV<0.0f)
									vert.mV=0.0f;
								if (vert.mU>stwidth)
									vert.mU=stwidth;
								if (vert.mV>stheight)
									vert.mV=stheight;

								vert.mDiffuse=0x00FFFFFFF;
								vert.mDiffuse=(vert.mDiffuse & 0x00FFFFFF) | (mCloakAlpha<<24);
							}

							v->mIndex[i]=tex->AddVertices(&vert,1);
						}
					}
				}
				for (c0 = 0; c0 < mesh_part->mNumTriangles; c0++)
				{
					CMeshTriangle *t = &mesh_part->mTriangles[c0];

					UWORD index[3];

					if ((mRenderMode==R_Cloaked) || (mRenderMode==R_Extruded))
					{
						// Make sure pass 0 has some data!
						if ((t->mA->mTexture[0]!=-1) && (mesh_part->mBaseMesh->mTextures[t->mA->mTexture[0]].mMaxBlend>0.0f))
						{
							index[0]=t->mA->mIndex[0];
							index[1]=t->mB->mIndex[0];
							index[2]=t->mC->mIndex[0];
	
							tex->AddIndices(&index,3);
						}
					}
					else
					{
						for (int i=0;i<passes;i++)
						{
							if ((t->mA->mTexture[i]!=-1) && (mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mMaxBlend>0.0f))
							{
								ASSERT(t->mA->mTexture[i]!=-1);
								ASSERT(t->mA->mTexture[i] == t->mB->mTexture[i]);
								ASSERT(t->mA->mTexture[i] == t->mC->mTexture[i]);

								index[0]=t->mA->mIndex[i];
								index[1]=t->mB->mIndex[i];
								index[2]=t->mC->mIndex[i];

								mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mVBuffer->AddIndices(&index,3);

								mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mVBuffer->mTexture->mRenderMode=i;
								mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mVBuffer->mTexture->mU=mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mTextureU[(vf1 % mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mFrames)];
								mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mVBuffer->mTexture->mV=mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mTextureV[(vf1 % mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mFrames)];
								mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mVBuffer->mTexture->mUScale=mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mTextureUScale[(vf1 % mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mFrames)];
								mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mVBuffer->mTexture->mVScale=mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mTextureVScale[(vf1 % mesh_part->mBaseMesh->mTextures[t->mA->mTexture[i]].mFrames)];
							}
						}
					}
				}
				if (mRenderMode==R_Cloaked)
				{
					LT.SRS(D3DRS_LIGHTING,FALSE);
					LT.STS(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
					tex->Draw(true);
					LT.SRS(D3DRS_LIGHTING,TRUE);
					LT.STS(0, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
				}
				else if (mRenderMode==R_Extruded)
				{
					LT.SRS(D3DRS_LIGHTING,FALSE);
					tex->Draw(true);
					LT.SRS(D3DRS_LIGHTING,TRUE);
				}
				else
					MESH_TEXTURE::DrawAllInMesh(mesh_part->mBaseMesh);
			}
		}
	}
	else if (mesh_part->mPartType==MESHPART_TYPE_SKELETAL)
	{
		// Skeletal mesh

		PROFILE_FN(SkeletalEverything);

		PROFILE_START(SkeletalBoneSetup);

		SINT vf1,vf2,f1,f2;
		float vframe=0.0f;
		float aframe;
		float vffrac;

		animind=-1;

		if (t)
			animind=t->GetRealAnimIndex();
		if ((flags & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
			animind=mAnimIndexOverride;

		if ((mesh_part->mNumVFrames >1) &&
			(mesh_part->mBaseMesh->mNumAnimModes)&&
			(animind>=0))
		{
			if (t)
				vframe = t->GetRenderFrame();
			if ((flags & RF_FRAMEOVERRIDE)==RF_FRAMEOVERRIDE)
				vframe=mFrameOverride;
			CAnimMode*	am = &mesh_part->mBaseMesh->mAnimModes[animind];
			vframe*=am->mFrameScaler; // this fraction varies for looped and unlooped animations
									// see loader code
			vframe+=am->mStart;
			if (controller)
				controller->UpdateRenderFrame(mesh_part,&vframe);

			vf1 = (SINT(floor(vframe))) % mesh_part->mNumVFrames;		//### % shouldn't be needed
			vf2 = vf1 + 1;
			if(vf2 > am->mLast) vf2 = am->mStart;

			// Get AFrames for vertex animation (if present)
			float	af1 = mesh_part->mVFrames[vf1];
			float	af2 = mesh_part->mVFrames[vf2];

			if (af2 < af1) af2 = float(SINT(floor(af1)) + 1);

			float	vfwhole= floorf(vframe);
					vffrac = vframe-vfwhole;
					aframe = (af2 * vffrac) + (af1 * (1 - vffrac));

			f1 = SINT(floor(aframe));
			f2 = f1 + 1;
			if	(f2 > SINT(floor(mesh_part->mVFrames[am->mLast]))) f2 = SINT(floor(mesh_part->mVFrames[am->mStart]));
		}
		else
		{
			// Static - use first frame
			vf1=0;
			vf2=0;
			vframe=0;
			f1=SINT(floor(mesh_part->mVFrames[vf1]));
			f2=f1;
			aframe=1.0f;
		}

		// Get two sets of PVerts to interpolate between, and an interpolation factor

		FVector *pverts1 = mesh_part->mPVertices[f1];
		FVector *pverts2 = mesh_part->mPVertices[f2];

		float	frac2 = aframe - floorf(aframe);
		float	frac1 = (1.f - frac2);

		// Get the transforms of ths part
		FMatrix partori;
		FVector partpos;
		mesh_part->GetWorldTransforms(vf1,vf2,vffrac,t,&partori,&partpos,true,true);
		FMatrix partori_inv=partori.Inverse();

		// Extract the transformations of the bones needed

		D3DXMATRIX bonemat[MAX_SOFTWARE_BONES];

		ASSERT(mesh_part->mNumBones<=MAX_SOFTWARE_BONES);

		// Calculate bone matrices

		for (int i=0;i<mesh_part->mNumBones;i++)
		{
			FMatrix boneori,bonebaseori;
			FVector bonepos,bonebasepos;

			// Get base transforms 
			//mesh_part->mBones[i]->GetWorldTransforms(0,t,&bonebaseori,&bonebasepos,true);
			bonebaseori=mesh_part->mBones[i]->mBaseOrientation;
			bonebasepos=mesh_part->mBones[i]->mBasePosition;
			// Get transforms as they are now
			mesh_part->mBones[i]->GetWorldTransforms(vf1,vf2,vffrac,t,&boneori,&bonepos,true,true);
			// Get transforms in the local space of this part
			bonebaseori=partori_inv * bonebaseori;
			bonebasepos=partori_inv * (bonebasepos - partpos);
			boneori=partori_inv * boneori;
			bonepos=partori_inv * (bonepos - partpos);

			D3DXMATRIX rmat,rimat,pmat,pimat,mat;
			FMatrix ori;
			FVector pos;

			// Bone orientation

			ori=boneori;
			rmat._11 = ori.Row[0].X; rmat._12 = ori.Row[1].X; rmat._13 = ori.Row[2].X;
			rmat._21 = ori.Row[0].Y; rmat._22 = ori.Row[1].Y; rmat._23 = ori.Row[2].Y;
			rmat._31 = ori.Row[0].Z; rmat._32 = ori.Row[1].Z; rmat._33 = ori.Row[2].Z;
			rmat._14 = rmat._24 = rmat._34 = 0.0f;
			rmat._41 = rmat._42 = rmat._43 = 0.0f;
			rmat._44 = 1.0f;

			// Bone base orientation

			ori=bonebaseori;
			rimat._11 = ori.Row[0].X; rimat._12 = ori.Row[1].X; rimat._13 = ori.Row[2].X;
			rimat._21 = ori.Row[0].Y; rimat._22 = ori.Row[1].Y; rimat._23 = ori.Row[2].Y;
			rimat._31 = ori.Row[0].Z; rimat._32 = ori.Row[1].Z; rimat._33 = ori.Row[2].Z;
			rimat._14 = rimat._24 = rimat._34 = 0.0f;
			rimat._41 = rimat._42 = rimat._43 = 0.0f;
			rimat._44 = 1.0f;

			D3DXMatrixInverse(&rimat,NULL,&rimat);

			// Bone position

			pos=bonepos;

			D3DXMatrixTranslation(&pmat,pos.X,pos.Y,pos.Z);
			
			// Bone base position

			pos=bonebasepos;

			D3DXMatrixTranslation(&pimat,pos.X,pos.Y,pos.Z);

			D3DXMatrixInverse(&pimat,NULL,&pimat);

			// Build composite bone transformation matrix

			bonemat[i] = pimat * rimat * rmat * pmat;
		}

		PROFILE_END(SkeletalBoneSetup);

		// Note : The accurate skeletal shader can't be turned on/off at runtime

		if ((CVertexShader::VertexShadersEnabled) && 
			(mesh_part->mNumBones<=MAX_SHADER_BONES) &&
			(!mDisableSkeletalShaders) &&
			(mRenderMode==R_Normal) &&
			(!(ENGINE.GetDrawDebugStuff() & DRAW_SKELETAL_ACCURATELY)))
		{
			// Vertex-shader based skeletal renderer
			// Only used if model has less than MAX_SHADER_BONES bones!

			PROFILE_START(SkeletalShader);

			// Set up the additional shader constants
			// The default constants will be set up by the
			// SetupShader() call in the vbuffer code

			// Bone data

			RENDERINFO.SetNumBones(mesh_part->mNumBones);
			
			for (int i=0;i<mesh_part->mNumBones;i++)
			{
				D3DXMATRIX mat=bonemat[i];

				// Pre-divide matrix to save having to do it after composition

				mat=mat/((float) BONE_SLOTS);

				RENDERINFO.SetBoneMatrix(i,&mat);
					
				// Transpose matrix prior to passing it to the shader

//				D3DXMatrixTranspose(&mat,&mat);

//				LT.D3D_SetVertexShaderConstant((i*4)+4,&mat,4);
			}

			DWORD declaration[]=
			{	D3DVSD_STREAM(0),
				D3DVSD_REG(0, D3DVSDT_FLOAT3),		// Position
				D3DVSD_REG(5, D3DVSDT_D3DCOLOR),	// Diffuse
				D3DVSD_REG(7, D3DVSDT_FLOAT2),		// Texture co-ords
				D3DVSD_REG(3, D3DVSDT_FLOAT3),		// Normal
				D3DVSD_REG(11, D3DVSDT_FLOAT1),		// Index 1
				D3DVSD_REG(12, D3DVSDT_FLOAT1),		// Index 2
				D3DVSD_REG(13, D3DVSDT_FLOAT1),		// Index 3
				D3DVSD_END()
			};

			RENDERINFO.SetDeclaration(declaration,sizeof(declaration));
			RENDERINFO.SetSkeletal(true);

			// Now render the mesh through the shader
			
			mesh_part->mMeshVB->Render(t,flags,&ori,&p);

			RENDERINFO.SetSkeletal(false);
			RENDERINFO.SetDeclaration(NULL,0);

			PROFILE_END(SkeletalShader);
		}
		else
		{
			// Normal skeletal renderer

			PROFILE_START(SkeletalVertexCalc);

			for (c0 = 0; c0 < mesh_part->mNumDVertices; c0++)
			{
				CMeshDVertex *v	= &mesh_part->mDVertices[c0];

				CFVF_GTEX		vert;
				
				vert.mPos		= pverts1[v->mPVert];
				// Should be :
				// (pverts1[v->mPVert] * frac1) + (pverts2[v->mPVert] * frac2);
				// Vertex animation on skeletal meshes is disabled until we
				// find (a) a good reason for it and (b) a way to do it.
				vert.mU			= v->mU;
				vert.mV			= v->mV;
				vert.mNormal	= v->mNormal;
				vert.mDiffuse	= v->mDiffuse; 

				if (mRenderAlpha!=255)
				{
					int a = vert.mDiffuse >> 24;
					a=(a*mRenderAlpha)/255;
					vert.mDiffuse=(a<<24) | (vert.mDiffuse & 0x00FFFFFF);
				}

				if ((flags & RF_HILIGHT)==RF_HILIGHT)
					vert.mDiffuse=0xFFFF0000;

				if (mRenderMode==R_BumpmapNormals)
				{
					vert.mDiffuse = VectorToRGBA(&v->mNormal,1.0f);
				}

				// Apply skeletal deformation

				D3DXVECTOR3 basepos,basenormal;
				basepos.x=vert.mPos.X;
				basepos.y=vert.mPos.Y;
				basepos.z=vert.mPos.Z;
				basenormal.x=vert.mNormal.X;
				basenormal.y=vert.mNormal.Y;
				basenormal.z=vert.mNormal.Z;
				
				vert.mPos.X = 0.0f;
				vert.mPos.Y = 0.0f;
				vert.mPos.Z = 0.0f;

				if (!(ENGINE.GetDrawDebugStuff() & DRAW_SKELETAL_ACCURATELY))
				{ 
					 // Optimised (fixed-bone-slot) renderer
					if (mesh_part->mBoneSlots[v->mPVert][0]!=-1)
					{
						for (int i=0;i<BONE_SLOTS;i++)
						{
							// Apply bone matrix

							D3DXVECTOR3 newpos;
							D3DXVECTOR3 newnormal;

							D3DXVec3TransformCoord(&newpos,&basepos,&bonemat[mesh_part->mBoneSlots[v->mPVert][i]]);
							D3DXVec3TransformCoord(&newnormal,&basenormal,&bonemat[mesh_part->mBoneSlots[v->mPVert][i]]);

							// Blend with current position 

							vert.mPos.X+=newpos.x;
							vert.mPos.Y+=newpos.y;
							vert.mPos.Z+=newpos.z;

							vert.mNormal.X+=newnormal.x;
							vert.mNormal.Y+=newnormal.y;
							vert.mNormal.Z+=newnormal.z;
						}
						// Divide by number of slots
						vert.mPos=vert.mPos /BONE_SLOTS;
						vert.mNormal.Normalise();
					}
					else
					{
						vert.mPos.X=basepos.x;
						vert.mPos.Y=basepos.y;
						vert.mPos.Z=basepos.z;
						
						vert.mNormal.X=basenormal.x;
						vert.mNormal.Y=basenormal.y;
						vert.mNormal.Z=basenormal.z;
					}
				}
				else
				{			
					bool gotbone=false;

					// Unoptimised (any number of bones) renderer
					for (int i=0;i<mesh_part->mNumBones;i++)
					{
						float weight=mesh_part->mBoneWeights[v->mPVert][i];

						if (weight>0)
						{
							// Apply bone matrix

							D3DXVECTOR3 newpos,newnormal;

							D3DXVec3TransformCoord(&newpos,&basepos,&bonemat[i]);
							D3DXVec3TransformCoord(&newnormal,&basenormal,&bonemat[i]);

							// Blend with current position according to vertex weight

							vert.mPos.X+=(newpos.x*weight);
							vert.mPos.Y+=(newpos.y*weight);
							vert.mPos.Z+=(newpos.z*weight);

							vert.mNormal.X+=(newnormal.x*weight);
							vert.mNormal.Y+=(newnormal.x*weight);
							vert.mNormal.Z+=(newnormal.x*weight);

							gotbone=true;
						}
					}
					if (gotbone)
					{
						vert.mNormal.Normalise();
					}
					else
					{
						vert.mPos.X=basepos.x;
						vert.mPos.Y=basepos.y;
						vert.mPos.Z=basepos.z;
						
						vert.mNormal.X=basenormal.x;
						vert.mNormal.Y=basenormal.y;
						vert.mNormal.Z=basenormal.z;
					}
				}
				v->mIndex[0] = mesh_part->mBaseMesh->mTextures[v->mTexture[0]].mVBuffer->AddVertices(&vert,1);
			}

			PROFILE_END(SkeletalVertexCalc);
			PROFILE_START(SkeletalRender);
		
/*			LT.ForceRS(D3DRS_SOFTWAREVERTEXPROCESSING,true);
			mesh_part->mMeshVB.Render();
			LT.ForceRS(D3DRS_SOFTWAREVERTEXPROCESSING,false);*/

			for (c0 = 0; c0 < mesh_part->mNumTriangles; c0++)
			{
				CMeshTriangle *t = &mesh_part->mTriangles[c0];

				ASSERT(t->mA->mTexture[0]!=-1);
				ASSERT(t->mA->mTexture[0] == t->mB->mTexture[0]);
				ASSERT(t->mA->mTexture[0] == t->mC->mTexture[0]);

				UWORD index[3];

				index[0]=t->mA->mIndex[0];
				index[1]=t->mB->mIndex[0];
				index[2]=t->mC->mIndex[0];

				mesh_part->mBaseMesh->mTextures[t->mA->mTexture[0]].mVBuffer->AddIndices(&index,3);
			}
			MESH_TEXTURE::DrawAllInMesh(mesh_part->mBaseMesh);

			PROFILE_END(SkeletalRender);
		}
	}
	else if (mesh_part->mPartType==MESHPART_TYPE_CAMERA)
	{
	}
	else if (mesh_part->mPartType==MESHPART_TYPE_NULL)
	{
	}
	else if (mesh_part->mPartType==MESHPART_TYPE_EMITTER)
	{
	}
	else 
	{
		// Unknown type
		char buffer[256];
		sprintf(buffer,"Attempt to render unknown mesh part type %d!\n",mesh_part->mPartType);
		TRACE(buffer);
		ASSERT(false);
	}

	LT.SRS(D3DRS_ALPHABLENDENABLE,TRUE); // ***HACK! Remove me!
}

// --------------------------------------------------------------------------------

void	CPCMeshRenderer::RenderMeshPart2D(CMESHPART *mesh_part, IRenderableThing *t, SINT skin)
{
	SINT		c0;	

	int		animind;

//	LT.D3DDev()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE2X);

	CMotionController *controller=NULL;

	if (t)
		controller=t->GetMotionController();

	if ((mesh_part->mPartType==MESHPART_TYPE_NULL) || (mesh_part->mPartType==MESHPART_TYPE_CAMERA))
	{
		// Don't render NULL parts or cameras
		return;
	}

	if (mesh_part->mPartType==MESHPART_TYPE_MESH)
	{
		// Normal mesh

		SINT vf1,vf2,f1,f2;
		float vframe;
		float aframe;

		if (t &&
			(mesh_part->mNumVFrames >1) &&
			(mesh_part->mBaseMesh->mNumAnimModes)&&
			((animind = t->GetRealAnimIndex())>= 0))
		{
			vframe = t->GetRenderFrame();
			CAnimMode*	am = &mesh_part->mBaseMesh->mAnimModes[animind];
			vframe*=am->mFrameScaler; // this fraction varies for looped and unlooped animations
									// see loader code
			vframe+=am->mStart;
			if (controller)
				controller->UpdateRenderFrame(mesh_part,&vframe);

			vf1 = (SINT(floor(vframe))) % mesh_part->mNumVFrames;		//### % shouldn't be needed
			vf2 = vf1 + 1;
			if(vf2 > am->mLast) vf2 = am->mStart;

			// Get AFrames for vertex animation (if present)
			float	af1 = mesh_part->mVFrames[vf1];
			float	af2 = mesh_part->mVFrames[vf2];

			if (af2 < af1) af2 = float(SINT(floor(af1)) + 1);

			float	vfwhole= floorf(vframe);
			float	vffrac = vframe-vfwhole;
					aframe = (af2 * vffrac) + (af1 * (1 - vffrac));

			f1 = SINT(floor(aframe));
			f2 = f1 + 1;
			if	(f2 > SINT(floor(mesh_part->mVFrames[am->mLast]))) f2 = SINT(floor(mesh_part->mVFrames[am->mStart]));
		}
		else
		{
			// Static - use first frame
			vf1=0;
			vf2=0;
			vframe=0;
			f1=SINT(floor(mesh_part->mVFrames[vf1]));
			f2=f1;
			aframe=1.0f;
		}

		// Get two sets of PVerts to interpolate between, and an interpolation factor

		FVector *pverts1 = mesh_part->mPVertices[f1];
		FVector *pverts2 = mesh_part->mPVertices[f2];

		float	frac2 = aframe - floorf(aframe);
		float	frac1 = (1.f - frac2);

		float	xsize=mesh_part->mBoundingBox->mAxes.X;
		float	ysize=mesh_part->mBoundingBox->mAxes.Y;
		float	xmin=mesh_part->mBoundingBox->mOrigin.X-xsize;
		float	ymin=mesh_part->mBoundingBox->mOrigin.Y-ysize;

		float	screenwidth=(float) LT.GetWindowWidth();
		float	screenheight=(float) LT.GetWindowHeight();

		float	xscale=screenwidth/(xsize*2);
		float	yscale=screenheight/(ysize*2);

		float	texscalex=screenwidth/1024.0f;
		float	texscaley=screenheight/1024.0f;

		// Only apply scaling factors if we are working with the screen contents texture

		if (mesh_part->mBaseMesh->mTextures[mesh_part->mTriangles[0].mA->mTexture[0]].mVBuffer->mTexture!=ENGINE.GetScreenTexture())
		{
			texscalex=1.0f;
			texscaley=1.0f;
		}

		for (c0 = 0; c0 < mesh_part->mNumTriangles; c0++)
		{
			STLVert svert[3];

			CMeshTriangle *t = &mesh_part->mTriangles[c0];

			CMeshDVertex *va=t->mA;
			CMeshDVertex *vb=t->mB;
			CMeshDVertex *vc=t->mC;

			FVector posa = (pverts1[va->mPVert] * frac1) + (pverts2[va->mPVert] * frac2);
			FVector posb = (pverts1[vb->mPVert] * frac1) + (pverts2[vb->mPVert] * frac2);
			FVector posc = (pverts1[vc->mPVert] * frac1) + (pverts2[vc->mPVert] * frac2);

			ASSERT(t->mA->mTexture[0]!=-1);
			ASSERT(t->mA->mTexture[0] == t->mB->mTexture[0]);
			ASSERT(t->mA->mTexture[0] == t->mC->mTexture[0]);

			FVector a = posb-posa;
			FVector b = posc-posa;
			FVector normal = a ^ b;
			FVector light;
			light.Set(1,1,1);
			normal.Normalise();
			light.Normalise();

			float l=normal * light;	
			l= cosf(l*3.14f);
			int lfactor=int (128.0f*fabsf(l))+127;
	
			ULONG c=lfactor+(lfactor<<8)+(lfactor<<16)+(255<<24);
		
			svert[0].mPos.X=(posa.X-xmin)*xscale;
			svert[0].mPos.Y=screenheight-(posa.Y-ymin)*yscale;
			svert[0].mPos.Z=1.0f;
			svert[0].mPos.W=1.0f;
			svert[0].mDiffuse=c;
			svert[0].mU=clamp(va->mU*texscalex);
			svert[0].mV=clamp((1-absf(va->mV))*texscaley);

			svert[1].mPos.X=(posb.X-xmin)*xscale;
			svert[1].mPos.Y=screenheight-(posb.Y-ymin)*yscale;
			svert[1].mPos.Z=1.0f;
			svert[1].mPos.W=1.0f;
			svert[1].mDiffuse=c;
			svert[1].mU=clamp(vb->mU*texscalex);
			svert[1].mV=clamp((1-absf(vb->mV))*texscaley);

			svert[2].mPos.X=(posc.X-xmin)*xscale;
			svert[2].mPos.Y=screenheight-(posc.Y-ymin)*yscale;
			svert[2].mPos.Z=1.0f;
			svert[2].mPos.W=1.0f;
			svert[2].mDiffuse=c;
			svert[2].mU=clamp(vc->mU*texscalex);
			svert[2].mV=clamp((1-absf(vc->mV))*texscaley);

			mesh_part->mBaseMesh->mTextures[t->mA->mTexture[0]].mVBuffer->mTexture->SelectTexture();

			UWORD	indices[] = {0, 1, 2};

			CVBufTexture *vbuf=mesh_part->mBaseMesh->mTextures[t->mA->mTexture[0]].mVBuffer;
			
			vbuf->SetupV(FVF_TL_FLAGS,D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,sizeof(STLVert),D3DPT_TRIANGLELIST,D3DPOOL_DEFAULT);
			vbuf->SetupI(D3DFMT_INDEX16,D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,2,D3DPOOL_DEFAULT);

			vbuf->AddVertices(&svert,3);
			vbuf->AddIndices(&indices,3);

			vbuf->Draw(true);

/*			LT.D3D_SetVertexShader(FVF_TL_FLAGS);
			LT.D3D_DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 3, 1,
					 							indices, D3DFMT_INDEX16, svert, sizeof(STLVert));*/
		}
	}
	else if (mesh_part->mPartType==MESHPART_TYPE_SKELETAL)
	{
		// Skeletal mesh
		TRACE("Error : 2D renderer doesn't support skeletal meshes!\n");
		ASSERT(false);
	}
	else 
	{
		// Unknown type
		char buffer[256];
		sprintf(buffer,"Attempt to render unknown mesh part type %d!\n",mesh_part->mPartType);
		TRACE(buffer);
		ASSERT(false);
	}

//	LT.D3DDev()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
}

#endif
