// PCEngine.cpp: implementation of the CPCEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "common.h"

#if TARGET == PC

#include	"PCEngine.h"
#include	"DX.h"
#include	"State.h"
#include	"Map.h"
#include	"D3DUtils.h"
#include	"Maptex.h"
#include	"Game.h"
#include	"RTMesh.h"
#include	"RenderThing.h"
#include	"Imposter.h"
#include	"Thing.h"
#include	"KempyCube.h"
#include	"Sky.h"
#include	"Water.h"
#include	"Shadows.h"
#include	"GCGamut.h"
#include	"World.h"
#include	"Camera.h"
#include	"Profile.h"
#include	"ParticleManager.h"
#include	"ParticleSet.h"
#include	"Line.h"
#include	"UMTexture.h"
#include	"PCHud.h"
#include	"Lights.h"
#include	"VBufTexture.h"
#include	"Capture.h"
#include	"Fog.h"
#include	"Smoke.h"
#include	"Atmospherics.h"
#include	"Console.h"
#include	"BattleLine.h"
#include	"EventManager.h"
#include	"landscape.h"
#include	"cockpit.h"
#include	"rendertarget.h"
#include	"spriterenderer.h"
#include	"waterreflection.h"
#include	"cutscene.h"
#include	"debris.h"
#include	"renderqueue.h"
#include	"screenfx.h"
#include	"renderinfo.h"
#include	"membuffer.h"
#include	"gameinterface.h"
#include	"Player.h"
#include	"debugtext.h"
#include	"debuglog.h"
#include	"rain.h"
#include	"cutsceneeditor.h"
#include	"pcpatchmanager.h"
#include	"debugmarker.h"

//=============================================-===--==---=---- ---  --   -
CPCEngine		ENGINE;
    //=============================================-===--==---=---- ---  --   -
   //
  // Engine
 //
//=============================================-===--==---=---- ---  --   -
CPCEngine::CPCEngine()
{	
	mDefaultShader	= NULL;
	mScreenTexture	= NULL;
	
	mCurrentViewpoint=-1;
	mRenderReflections=false;
	mTextureReflections=true;
	mHudAditive		= TRUE;
//#ifdef EDITORBUILD2
	mNavDisplay = FALSE;
//#endif
}
//=============================================-===--==---=---- ---  --   -
CPCEngine::~CPCEngine()
{
//	ShutDown();
}
//=============================================-===--==---=---- ---  --   -
void CPCEngine::ShutDown()
{
	CEngine::Shutdown();

	SAFE_RELEASE(mScreenTexture);

	SAFE_DELETE(mWaterReflection);

	SAFE_RELEASE(mDefaultTexture);
	SAFE_RELEASE(mTexOutline);
	SAFE_RELEASE(mTexOpaque);
	SAFE_RELEASE(mTexEdArrow);

	SAFE_RELEASE(mDefaultMesh);

	PATCHMANAGER.Shutdown();
}

//=============================================-===--==---=---- ---  --   -
// Sets the gamma ramp bias (0=normal, 1=silly contrast!)
//=============================================-===--==---=---- ---  --   -

void con_setgammabias(char *cmd)
{
	float value;

	if (sscanf(cmd,"%*s %f",&value)!=1)
	{
		CONSOLE.Print("Syntax : SetGammaBias <value> (range 0-1)\n");
		return;
	}

	ENGINE.SetGammaBias(value);
}

void CPCEngine::SetGammaBias(float value)
{
	D3DGAMMARAMP ramp;

	for (int i=0;i<256;i++)
	{
		float scale=(256-i)*value;
		if (scale>1)
			scale=1;
		if (scale<0)
			scale=0;
		scale=1-scale;
		ramp.red[i]=(UWORD) (mDefaultGammaRamp.red[i]*scale);
		ramp.green[i]=(UWORD) (mDefaultGammaRamp.green[i]*scale);
		ramp.blue[i]=(UWORD) (mDefaultGammaRamp.blue[i]*scale);
		ramp.red[i]=(ramp.red[i]<<8)+ramp.red[i];
		ramp.green[i]=(ramp.green[i]<<8)+ramp.green[i];
		ramp.blue[i]=(ramp.blue[i]<<8)+ramp.blue[i];
	}

	LT.D3D_SetGammaRamp(D3DSGR_NO_CALIBRATION,&ramp);
}

//=============================================-===--==---=---- ---  --   -
BOOL CPCEngine::Init()
{
	if (!CEngine::Init())
		return(FALSE);
	
	mDefaultShader	= NULL;
	mScreenTexture	= NULL;
	
	mDrawDebugStuff = DRAW_COCKPIT ;

//	HRESULT hr;	
/*
	if(FAILED(hr = mKempyCube->Init(MAP.GetGenerator()->SV.CubeNo)))	return hr;
	if(FAILED(hr = mSky->Init(MAP.GetGenerator()->SV.CloudNo)))			return hr;
	if(FAILED(hr = mWater->Init(MAP.GetGenerator()->SV.WaterTexture)))	return hr;
*/
	
	mNumGlobalMeshes = 0;

	// Set up the capture texture
	mScreenTexture = new( MEMTYPE_TEXTURE ) CTEXTURE();

	mScreenTexture->InitCustom(1024,512,TEXFMT_X8R8G8B8,1,true);
	strcpy(mScreenTexture->Name,"Screen contents texture");
	mCaptureScreen=true;
	mCaptureAreaTop=0;
	mCaptureAreaBottom=1000;
	mBlurAlpha=0;
	mNewBlurAlpha=-1;
	mFirstFrame=TRUE;

	// Set up the reflections

	mWaterReflection=new( MEMTYPE_WATER ) CWaterReflection();

	// Grab the current gamma ramp

	LT.D3D_GetGammaRamp(&mDefaultGammaRamp);

	// Set up our console commands

	CONSOLE.RegisterCommand("SetGammaBias","Alter the gamma ramp bias (0=normal, 1=black), 0-0.005 is a good range to try",&con_setgammabias);

	CONSOLE.RegisterVariable("cg_renderreflections","Should water reflections be rendered",CVar_bool,&mRenderReflections);
	CONSOLE.RegisterVariable("cg_texturereflections","Should render to texture be used for reflections",CVar_bool,&mTextureReflections);

	PATCHMANAGER.Init(33*33,17*17,9*9,5*5);
	
	return(TRUE);
}
//=============================================-===--==---=---- ---  --   -
void CPCEngine::InitResources()
{
	CEngine::InitResources();

	mDefaultTexture     = CTEXTURE::GetTextureByName("meshtex\\default.tga");
	mTexOutline			= CTEXTURE::GetTextureByName("meshtex\\outline.tga");
	mTexOpaque			= CTEXTURE::GetTextureByName("meshtex\\basicpanel.tga");
	mTexEdArrow			= CTEXTURE::GetTextureByName("meshtex\\EdArrow.tga");

	mDefaultMesh = CMESH::GetMesh("default.msh");
}

//******************************************************************************************
void	CPCEngine::DrawDebugCuboid(DWORD col, FVector &axes, FVector &pos, FMatrix ori, CTEXTURE *for_texture)
{
	// Draw polys representing cuboid.
	// ** ASSUMES local orientation already set up...

	RENDERINFO.SRS( RS_ALPHABLENDENABLE,	TRUE );
	RENDERINFO.SRS( RS_SRCBLEND,			BLEND_ONE );
	RENDERINFO.SRS( RS_DESTBLEND,			BLEND_ONE );
	RENDERINFO.Apply() ;

	FVector ax   = ori * FVector(axes.X, 0, 0);
	FVector ay   = ori * FVector(0, axes.Y, 0);
	FVector az   = ori * FVector(0, 0, axes.Z);
	
	SINT		c0;
	CFVF_GTEX	vert;

	CVBufTexture *vb=CVBufTexture::GetTextureByTex(for_texture);

	vb->SetupV(FVF_GTEX_FLAGS,D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,sizeof(CFVF_GTEX),D3DPT_TRIANGLELIST,D3DPOOL_DEFAULT);
	vb->SetupI(D3DFMT_INDEX16,D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,2,D3DPOOL_DEFAULT);

	for (c0 = 0; c0 < 6; c0++)
	{
		SINT		i[4];

		for (int c1 = 0;c1 < 4; c1 ++)
		{
			switch (c0)
			{
			case 0:	vert.mPos	=   ax                  + ((c1 & 1) ? ay : -ay) + ((c1 & 2) ? az : -az) ; break;
			case 1:	vert.mPos	= - ax                  + ((c1 & 1) ? ay : -ay) + ((c1 & 2) ? az : -az) ; break;
			case 2:	vert.mPos	= ((c1 & 1) ? ax : -ax) + ay                    + ((c1 & 2) ? az : -az) ; break;
			case 3:	vert.mPos	= ((c1 & 1) ? ax : -ax) - ay                    + ((c1 & 2) ? az : -az) ; break;
			case 4:	vert.mPos	= ((c1 & 1) ? ax : -ax) + ((c1 & 2) ? ay : -ay) + az		   			; break;
			case 5:	vert.mPos	= ((c1 & 1) ? ax : -ax) + ((c1 & 2) ? ay : -ay) - az		   			; break;
			};

			vert.mPos.X  += pos.X;
			vert.mPos.Y  += pos.Y;
			vert.mPos.Z  += pos.Z;

			vert.mU	= (c1&1)?1.f:0.f;
			vert.mV	= (c1&2)?0.f:1.f;

			// Make the front of objects a different colour

			if (c0==2)
				vert.mDiffuse = 0xFFFFFFFF;
			else
				vert.mDiffuse = col;

			i[c1]=vb->AddVertices(&vert,1);
		}

		// double-sided...

		UWORD indices[12];
		
		indices[0]=i[0]; indices[1]=i[3]; indices[2]=i[2];
		indices[3]=i[0]; indices[4]=i[1]; indices[5]=i[3];
		indices[6]=i[0]; indices[7]=i[2]; indices[8]=i[3];
		indices[9]=i[0]; indices[10]=i[3]; indices[11]=i[1];
		
		vb->AddIndices(&indices,12);
	}

	vb->Draw(true);

	SAFE_RELEASE(vb);
	RENDERINFO.SRS( RS_SRCBLEND,			BLEND_SRCALPHA );
	RENDERINFO.SRS( RS_DESTBLEND,			BLEND_INVSRCALPHA );
	RENDERINFO.Apply() ;
}

//******************************************************************************************

void	CPCEngine::DrawDebugLine(DWORD col, FVector &start, FVector &end)
{
	FVector axes=FVector(0.01f,0.01f,(end-start).Magnitude()/2.0f);

	FMatrix ori;

	ori.BuildLookMatrix(end-start,FVector(0,0,1));

	DrawDebugCuboid(col,axes,(start+end)/2.0f,ori,GetOpaqueTexture());
}

//******************************************************************************************
void	CPCEngine::RenderArrow(FVector v1, FVector v2, DWORD col)
{
	// draw arrow between 2 points

	FMatrix ori = ID_FMATRIX;
	FVector pos = FVector(0, 0, 0);
	RENDERINFO.SetWorld(pos, ori);

	FVector l = v2 - v1;
	FVector s = l ^ FVector(0, 0, -1.f);
	s.Normalise();
	s *= 0.5f;

//	LOG.AddMessage("arrow dif = %08x", col) ;
	SINT		c1;
	CFVF_GTEX	vert;

	CVBufTexture *vb=CVBufTexture::GetTextureByTex(mTexEdArrow);
	
	vb->SetupV(FVF_GTEX_FLAGS,D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,sizeof(CFVF_GTEX),D3DPT_TRIANGLELIST,D3DPOOL_DEFAULT);
	vb->SetupI(D3DFMT_INDEX16,D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,2,D3DPOOL_DEFAULT);	

	SINT		i[4];

	for (c1 = 0;c1 < 4; c1 ++)
	{
		vert.mPos = ((c1 & 1) ? ZERO_FVECTOR : l) + ((c1 & 2) ? s : -s);

		FVector vr = v1 + FVector(0, 0, -0.5f); 

		vert.mPos.X += vr.X ;
		vert.mPos.Y += vr.Y ;
		vert.mPos.Z += vr.Z ;

		vert.mU	= (c1&2)?1.f:0.f;
		vert.mV	= (c1&1)?0.f:1.f;
		vert.mDiffuse = col;
// broken by me :-)		vert.mDiffuse = col;

		i[c1]=vb->AddVertices(&vert,1);
	}

	UWORD indices[6];
	
	indices[0]=i[0]; indices[1]=i[3]; indices[2]=i[2];
	indices[3]=i[0]; indices[1]=i[1]; indices[5]=i[3];
	
	vb->AddIndices(&indices,6);

	vb->Draw(true);
	
	SAFE_RELEASE(vb);
}

//=============================================-===--==---=---- ---  --   -
HRESULT CPCEngine::PreRender(CViewport *viewport)
{

	// Do offscreen stuff

	STATE.UseDefault();
	
	SwitchAllLights(FALSE);					// don't want lights showing on imps

	CIMPOSTER::MakeReady(mFirstFrame);		// Build imposters if needed

	// Clear whole screen

	PLATFORM.BeginScene();

	mCurrentViewpoint=0;
	memcpy(&mViewport[mCurrentViewpoint],viewport,sizeof(mViewport[mCurrentViewpoint]));

	PLATFORM.SetViewport(viewport);

	memcpy(&mCurrentViewport,viewport,sizeof(mCurrentViewport));
	
	// Clear entire viewport (the whole screen)

	LT.D3D_Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0x00607080, 1.0f, 0L );	

	PLATFORM.EndScene();

	// Update the ripple effect

	mRipplePhase+=1;

	return(S_OK);
}

//=============================================-===--==---=---- ---  --   -

HRESULT CPCEngine::Render(int viewpoint)
{
	if (!mCamera[viewpoint])
		return S_OK;  //! JCL - er..!!
	
	SelectViewpoint(viewpoint);

	STATE.UseDefault();

	RENDERQUEUE.Reset();	

	// Render shadows

	SwitchAllLights(FALSE);					// don't want lights showing on shadows 
	
	{
		PROFILE_FN(RenderShadowTextures);			
		SHADOWS.RenderTextures();			// render shadows picked last frame
		// this only scribles on the z-buffer
		CHECK_D3D_STATE("Shadow creation",0,0);
	}	
	
	PROFILE_START(TotalRender);

	{
		PROFILE_FN(RenderBeginScene);

		BOOL	bs_ok = PLATFORM.BeginScene();
		if (!bs_ok)
			return S_OK;		
	}

	PLATFORM.SetViewport(&mViewport[mCurrentViewpoint]);

    SwitchAllLights(TRUE);
	
	ASSERT(LT.D3DActive());

	float zoom = mCamera[mCurrentViewpoint]->GetZoom();	

	// Move debris into position around the camera

	CDebris::ShuffleDebris(mCamera[mCurrentViewpoint]->GetPos());

	/*

		Pass 0 : Reflection pass
		Pass 1 : Actual rendering

	*/

	int startpass=0;
	int endpass=1;

	if (!mRenderReflections) 
		startpass=1;

	float wlevel;
	wlevel = MAP.GetWaterLevel();

	for (int pass=startpass;pass<=endpass;pass++)
	{
		//********************************************************
		//** Setup

		// For first pass, reflect camera in water
		if (pass==0)
		{
			((CInterpolatedCamera *) mCamera[viewpoint])->Mirror(wlevel);
			LT.ForceRS(D3DRS_CULLMODE,D3DCULL_CW);
			if (mTextureReflections)
			{
				mWaterReflection->SetupRenderTarget();
			}
		}

		// Recalculate view in case it's moved

		
		FMatrix	matview;
		FVector	campos = mCamera[mCurrentViewpoint]->GetPos();
		GetViewMatrixFromCamera(mCamera[mCurrentViewpoint], &matview);
		if (pass==0)
			mGamut->MakeIt(&matview,256,zoom*1.0f,zoom*0.75f,&campos,true);
		else
			mGamut->MakeIt(&matview,256,zoom*1.0f,zoom*0.75f,&campos,false);
		RENDERINFO.SetView(&matview, &campos);



// Horible hack stuff here


		static float zoooom=96;
/*
		if(LT.JoyState(0)->rgbButtons[15]) zoooom -= 1.0f;
		if(LT.JoyState(0)->rgbButtons[13]) zoooom += 1.0f;
		if(zoooom<8) zoooom=8;
		if(zoooom>128) zoooom=128;
*/
		HUD.AddMapWhoMarkers(campos, zoooom);

		FMatrix bbori = mCamera[mCurrentViewpoint]->GetOrientation();
		
		float angle = atan2f(bbori.Row[1].X,bbori.Row[1].Y);
		CIMPOSTER::SetBillBoardAngle(angle);
	//		bbori.MakeRotationYawF(angle);
		
	//		D3DUtils.SetBillBoardMat(&bbori);

		SetupLights();
		
		mLights->Process();
		
		// finish setting up the fog
		RENDERINFO.SetFogColour(MAP.GetFog());
		float density = MAP.GetFogDensity();
		RENDERINFO.SetFogDensity(density);

		// Reflections

		if (mRenderReflections && mTextureReflections)
		{
			mWaterReflection->Draw();
		}

		// Water

		CHECK_D3D_STATE("pre-water",0,0);

		if (mRenderReflections)
		{
			if (pass==1)
			{
				LT.D3D_Clear(0,NULL,D3DCLEAR_ZBUFFER,0x00607080,1.0f,0L);
				if (wlevel<0)
				{
					LT.SRS(D3DRS_ZFUNC,D3DCMP_ALWAYS);
					mWater->Render(mSky, wlevel);
					LT.SRS(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
					CHECK_D3D_STATE("Water",0,0);
				}
			}
		}

		//********************************************************
		//** SKY
		
		// Draw Sky after munging up a large view matrix
		{
			PROFILE_FN(RenderCube);
			
			float vfar = 1*(float)sqrt((SKY_HEIGHT*SKY_HEIGHT)+((SKY_CELL_SIZE*SKY_RAD)*(SKY_CELL_SIZE*SKY_RAD)));
			RENDERINFO.SetProjection(1.0f,vfar, zoom ,zoom*0.75f);

			mKempyCube->Render();
			CHECK_D3D_STATE("Kempycube",0,0);
		}
		{
			PROFILE_FN(RenderSky);
			mSky->Render();
			CHECK_D3D_STATE("Sky",0,0);
		}


		//********************************************************
		//** LANDSCAPE

		// set up the real view
		float howfar = 700;
		RENDERINFO.SetProjection(mNearZ,howfar,mNearZ*zoom,mNearZ*zoom*0.75f);

		// used for landscape, objects, shadows, particles, we can finally back the bastard up

		if (mRenderLandscape)
		{
			mLandscape[mCurrentViewpoint]->Render(mGamut);

			CHECK_D3D_STATE("Landscape",0,0);
			SHADOWS.RenderLandscapeShadows();
			CHECK_D3D_STATE("Shadows",0,0);
		}
		
		// Water (for non-reflective mode)
		if (!mRenderReflections)
		{
			if (wlevel<0)
			{
				mWater->Render(mSky, wlevel);
				CHECK_D3D_STATE("Water",0,0);
			}
		}

	//	CIMPOSTER::TestDrawAll();

		SHADOWS.TestDraw();

		CHECK_D3D_STATE("Shadow test drawing",0,0);
		
		// Capture needs to be done here for cloaked stuff
	/*	if (mCaptureScreen)
		{
			int top=(LT.GetWindowHeight()*mCaptureAreaTop)/1000;
			int bottom=(LT.GetWindowHeight()*mCaptureAreaBottom)/1000;
			LPDIRECT3DSURFACE8 backbuffer;
			LT.D3D_GetRenderTarget( &backbuffer );
			mScreenTexture->GrabFromSurface(backbuffer,0,top,0,top,LT.GetWindowWidth(),bottom-top,false);
			backbuffer->Release();
			mCaptureScreen=false;
		}*/

		//********************************************************
		//** WORLD

		SHADOWS.ResetShads();				// clear shadow list

		LT.SRS(D3DRS_ALPHATESTENABLE,			TRUE );

		{
			PROFILE_FN(RenderImposters);
			DWORD rsx[]={D3DRS_ALPHATESTENABLE,0};
			WORLD.RenderImposters(mGamut) ;		// render imposters

			CHECK_D3D_STATE("World Imposters",rsx,0);
				
			CIMPOSTER::RenderAllImposters();	// Render cache of all imposter stuff
			CHECK_D3D_STATE("World Imposters",rsx,0);
		}
		
		{
			PROFILE_FN(RenderThings);
			WORLD.Render(mGamut,false);			// render objects		
		}
		
		DEBUGMARKERS.Render();				// Render debug markers

		{
			PROFILE_FN(RenderShadows);
			CHECK_D3D_STATE("World",rsx,0);
			SHADOWS.RenderObjectShadows();		// Render object shadows
			CHECK_D3D_STATE("Shadows",rsx,0);
		}

		if (SHADOWS.mBlobShadowsEnabled)
		{
			PROFILE_FN(RenderBlobShadows);
			WORLD.RenderBlobShadows(mGamut);	// Draw blob shadows into VB
			SHADOWS.DrawAllBlobShadows();		// Render VB contents out
			CHECK_D3D_STATE("Blob shadows",rsx,0);
		}
		
//		SHADOWS.DrawBlobShadow(FVector(256.0f,256.0f,0),5.0f);
		
		LT.SRS(D3DRS_ALPHATESTENABLE,			FALSE );

		PARTICLE_MANAGER.DoInterpolation(); // Needs to be after things are drawn and before the particles.
		PARTICLE_MANAGER.RenderMeshes();	// needs to be here to share the lighting model..
		CHECK_D3D_STATE("Particles",0,0);

		if (pass==0)
		{
			LT.ForceRS(D3DRS_CULLMODE,D3DCULL_CCW);
			((CInterpolatedCamera *) mCamera[viewpoint])->Mirror(wlevel);
			if (mTextureReflections)
			{
				mWaterReflection->ShutdownRenderTarget(mRipplePhase);
			}
		}

		RENDERQUEUE.Render();				// Render all our batched stuff
	}

// Render effects

	CAtmospherics::RenderAll();
	CHECK_D3D_STATE("Atmospherics",0,0);
	
// Fog
	
//	CFog::Render();

// Smoke
	
//	CSmoke::Render();

// Rain

//	jcl_water_test();

	// Gamut debug

///	mGamut->RenderGamut();

	CHECK_D3D_STATE("Gamut debug",0,0);

	//********************************************************
	//** MAPWHO DEBUG
	if (mDrawDebugStuff & DRAW_MAP_WHO)
	{
		EnableAlpha() ;
		MAP_WHO.DrawMapWhoDebug() ;
	
		CHECK_D3D_STATE("Mapwhodbug",0,0);
		
	}



	//***********************************************************************
	//** SUN

	if (!(GAME.IsPaused()))
	{
		SINT	cno = MAP.GetHFProperties().CubeNo;

		if (cno != 6)
		{
			// can we see the sun?
			#define SUN_SCALE	0.6f
			FVector	sun;
			
			// JCL - hmmm!!!
			switch(MAP.GetHFProperties().CubeNo)
			{
			case 2:
			case 3:
			case 4:
				sun	= FVector(0.5f, 0.0055f, -0.0505f);
				break;

			case 7:
			case 8:
			case 9:
				sun = FVector(0.5f, 0.117f, -0.0469f);
				break;

			default:
				sun = FVector(0.5f, -0.00f, -0.62f);
				break;
			}

			sun *= SUN_SCALE;
			FVector	sunch = sun;
			FVector temp;

			sunch.Normalise();
			sunch *= 100.f;
			CLine line_of_sight(mCamera[mCurrentViewpoint]->GetPos(), mCamera[mCurrentViewpoint]->GetPos() + sunch) ;
			CWorldLineColReport wlcr ;
			CThing* to_ignore = NULL ;

			if (mPlayer[mCurrentViewpoint] && mPlayer[mCurrentViewpoint]->GetBattleEngine())
			{
				to_ignore = mPlayer[mCurrentViewpoint]->GetBattleEngine() ;
			}

			if(WORLD.FindFirstThingToHitLine(line_of_sight,to_ignore, &wlcr, TRUE)==kCollideNothing)
			{
				// note - could now do this with a persistent frame of reference, and animate!!
				CParticleDescriptor	*pd;

				pd = PARTICLE_SET.GetPD("Sun Sprite");
				PARTICLE_MANAGER.AddParticle(pd, NULL, mCamera[mCurrentViewpoint]->GetPos() + sun);
			}
		}
	}

	//***********************************************************************
	//** PARTICLE SYSTEM
	{
		RENDERINFO.SetWorld(ZERO_FVECTOR, ID_FMATRIX);

		LT.D3D_SetMaterial(STATE.GetDefaultMaterial());
		
		PROFILE_FN(RenderParticles);

		PARTICLE_MANAGER.RenderOther();

		CHECK_D3D_STATE("Particles (other)",0,0);
	}

	//***********************************************************************
	//** MISC STUFF


//	lpDev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
//	MAP_WHO.DrawMapWhoDebug() ;

/*	lpDev->SetRenderState(D3DRS_LIGHTING,TRUE);

	GAME.RenderHudStuff() ;

	lpDev->SetRenderState(D3DRS_LIGHTING,FALSE);

	lpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );

	FVector smallpos = *campos/512;
	CRotoMap::Render(matview, &smallpos,&FVector(0.16f,0.112f,-19.8f));
*/	

	//*****************************************
	//** Render Cockpit

	CPlayer *player=mPlayer[mCurrentViewpoint];
	
	if (player)
	{
		CBattleEngine *be = player->GetBattleEngine();
		
		if (be && (!be->IsDying()) && (player->ShouldRenderInternalCockpit() == TRUE))
		{
			ENGINE.EnableAlpha();
//			LT.SRS(D3DRS_LIGHTING,TRUE);
			RENDERINFO.SetLightingEnabled(TRUE);
			
			be->GetCockpit()->Render() ;
			RENDERQUEUE.Render();
			RENDERINFO.SetLightingEnabled(FALSE);
//			LT.SRS(D3DRS_LIGHTING,FALSE);
		}
	}

	// Render screen FX

	SCREENFX.Render();

	// Debug - draw reflection texture

	//mWaterReflection->DrawDebug();

	// *************************************************************************

	{
		PROFILE_FN(RenderEndScene);
		PLATFORM.EndScene();
	}

	return S_OK;
}

//=============================================-===--==---=---- ---  --   -

HRESULT	CPCEngine::PostRender(CViewport *viewport)
{
	{
		PROFILE_FN(RenderBeginScene);
		PLATFORM.BeginScene();	
	}

	PLATFORM.SetViewport(viewport);
	memcpy(&mCurrentViewport,viewport,sizeof(mCurrentViewpoint));

	// Render HUD

	HUD.Render();

	RENDERINFO.SetFogEnabled(false);
	RENDERINFO.SetLightingEnabled(false);
	RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_CLAMP);
	RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_CLAMP);
	RENDERINFO.STS(0, TSS_MINFILTER, TEXF_POINT);
	RENDERINFO.STS(0, TSS_MAGFILTER, TEXF_POINT);
	RENDERINFO.STS(0, TSS_MIPFILTER, TEXF_NONE);
	RENDERINFO.Apply() ;
	
	GAME.GetMessageLog()->Render() ;
	GAME.GetLevelBriefingLog()->Render() ;

 
	RENDERINFO.SetFogEnabled(false);
	RENDERINFO.SetLightingEnabled(false);
	RENDERINFO.STS(0, TSS_ADDRESSU, TADDRESS_CLAMP);
	RENDERINFO.STS(0, TSS_ADDRESSV, TADDRESS_CLAMP);
	RENDERINFO.STS(0, TSS_MINFILTER, TEXF_POINT);
	RENDERINFO.STS(0, TSS_MAGFILTER, TEXF_POINT);
	RENDERINFO.STS(0, TSS_MIPFILTER, TEXF_NONE);
	RENDERINFO.STS(0, TSS_MINFILTER, TEXF_LINEAR);
	RENDERINFO.STS(0, TSS_MAGFILTER, TEXF_LINEAR);
	RENDERINFO.SRS(RS_ZFUNC, ZFUNC_LEQUAL);
	RENDERINFO.Apply() ;

//	CHECK_D3D_STATE("HUD",0,0);			*/\/\/\/\/\/\/ We'll do this after Stu has stopped fucking around... \/\/\/\/\/*

	if (RENDERQUEUE.mUseQueue)
		PLATFORM.Font( FONT_DEBUG )->DrawText(0,40,0xFFFFFFFF,ToWCHAR("Render queue active"));

	// Stencil shadows
/*	
	RENDERINFO.SetFogEnable(false);
	LT.SRS(D3DRS_STENCILENABLE,true);
	LT.SRS(D3DRS_STENCILPASS,D3DSTENCILOP_ZERO);
	LT.SRS(D3DRS_STENCILFUNC,D3DCMP_LESS);
	LT.SRS(D3DRS_STENCILREF,0);
	
	CTEXTURE *tex=ENGINE.GetDefaultTexture();
	CSPRITERENDERER::DrawColouredSprite(0,0,ORDER(1),tex,0xFFFFFFFF,((float) LT.GetWindowWidth())/((float) tex->GetWidth()),((float) LT.GetWindowHeight())/((float) tex->GetHeight()));	
	
	LT.SRS(D3DRS_STENCILPASS,D3DSTENCILOP_KEEP);
	LT.SRS(D3DRS_STENCILFUNC,D3DCMP_NEVER);	
	LT.SRS(D3DRS_STENCILENABLE,false);
	RENDERINFO.SetFogEnable(true);
*/
	// Render cutscreen widescreen bars

//	CCutscene::RenderWidescreenBars();
	
	// Render screen overlays/blur
	
	HUD.RenderOverlay();
	//	DWORD hudrsx[]={D3DRS_ZWRITEENABLE, D3DRS_FOGENABLE, 0 };

	
//	CHECK_D3D_STATE("HUD Overlay",0,0);			*/\/\/\/\/\/\/ Removed too \/\/\/\/\/*

	PLATFORM.SetViewport(viewport);
	memcpy(&mCurrentViewport,viewport,sizeof(mCurrentViewpoint));	
	
	if (mBlurAlpha>0)
	{
		HUD.RenderBlur(mBlurAlpha);
		if (mBlurAlpha>0)
			mBlurAlpha-=15;
	}


	if (mCaptureScreen)
	{
		LPDIRECT3DSURFACE8 backbuffer;
		LT.D3D_GetRenderTarget( &backbuffer );
		int top=(LT.GetWindowHeight()*mCaptureAreaTop)/1000;
		int bottom=(LT.GetWindowHeight()*mCaptureAreaBottom)/1000;
		mScreenTexture->GrabFromSurface(backbuffer,0,top,0,top,LT.GetWindowWidth(),bottom-top,false);
		mCaptureScreen=false;
		backbuffer->Release();
	}
	
	// Apply any delayed changes to blur/overlays
	
	if (mNewBlurAlpha!=-1)
	{
		mBlurAlpha=mNewBlurAlpha;
		mNewBlurAlpha=-1;
	}
	
	HUD.SwitchInOverlay();
	
	// Write our movie file if necessary
	
	CCAPTURE::Capture();

	// Draw the interface (help, pause menu, etc)

	GAMEINTERFACE.Render();
	// Finally, reset all the VBuffers
	
	CVBufTexture::ResetAll();
	
	PROFILE_END(TotalRender);	
	
//	LT.SRS(D3DRS_LIGHTING,TRUE);
	RENDERINFO.SetLightingEnabled(TRUE);
	
#if ENABLE_PROFILER == 1
	if (mDrawDebugStuff & DRAW_PROFILE)
	{
		CProfiler::ShowProfile();
	}
#endif

	{
		PROFILE_FN(RenderDebug);

		// draw memory manager
		if (mDrawDebugStuff & DRAW_MEM_MANAGER)
		{
			MEM_MANAGER.PrintStats() ;	
		}

		GAME.DrawDebugStuff() ;

		// Draw Misc Game Stuff

		GAME.DrawGameStuff();
		// Draw console
		
		RENDERINFO.SetFogEnabled(false);
		CONSOLE.Render();
		RENDERINFO.SetFogEnabled(true);

		CHECK_D3D_STATE("After 2D",0,0);
	}
	
	{
		PROFILE_FN(RenderEndScene);
		PLATFORM.EndScene();
	}

	mFirstFrame=FALSE;

	return(S_OK);
}

//=============================================-===--==---=---- ---  --   -
void	CPCEngine::SetBlurAlpha(int a,bool thisframe)
{
	if (thisframe)
		mBlurAlpha=a;
	else
		mNewBlurAlpha=a;
}
//=============================================-===--==---=---- ---  --   -
CDetailLevel* CPCEngine::GetDetailLevel(int lev) 
{
	ASSERT(mCurrentViewpoint!=-1);
	return mLandscape[mCurrentViewpoint]->GetDetailLevel(lev);
}
//=============================================-===--==---=---- ---  --   -
void CPCEngine::SetDefaultMaterial()
{
	LT.D3D_SetMaterial(STATE.GetDefaultMaterial());
}

//=============================================-===--==---=---- ---  --   -
void CPCEngine::SetModulate2X( const bool aSet )
{
	if( aSet )
	{
		LT.ForceTS(0, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
	}
	else
	{
		LT.ForceTS(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	}
}
//=============================================-===--==---=---- ---  --   -
void CPCEngine::AccumulateResources( CResourceAccumulator * accumulator )
{
	accumulator->AddTexture( mDetailTexture );
	accumulator->AddTexture( mDefaultTexture );
	accumulator->AddTexture( mTexOutline );
	accumulator->AddTexture( mTexOpaque );
	accumulator->AddTexture( mTexEdArrow );

	accumulator->AddMesh( mDefaultMesh );

	mKempyCube->AccumulateResources( accumulator );
	SCREENFX.AccumulateResources( accumulator );
	GAMEINTERFACE.AccumulateResources( accumulator );
	PARTICLE_SET.AccumulateResources( accumulator );
}
//=============================================-===--==---=---- ---  --   -

#endif