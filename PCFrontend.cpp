#include	"Common.h"

#if TARGET == PC

//#include	"DX.h"

#include	"Frontend.h"
#include	"Platform.h"
#include	"SoundManager.h"

#include	"State.h"
#include	"Engine.h"
#include	"D3DUtils.h"
#include	"SpriteRenderer.h"
#include	"MeshRenderer.h"
#include	"RenderInfo.h"
#include	"ParticleManager.h"

//*********************************************************************************
CPCFrontEnd	FRONTEND;

//*********************************************************************************
void	CPCFrontEnd::RenderParticles()
{
	RENDERINFO.SetWorld(ZERO_FVECTOR, ID_FMATRIX);

	LT.D3D_SetMaterial(STATE.GetDefaultMaterial());
//	LT.SRS(D3DRS_LIGHTING,FALSE);
	RENDERINFO.SetLightingEnabled(FALSE);
	LT.SRS( D3DRS_ALPHATESTENABLE, FALSE ); 
	PARTICLE_MANAGER.RenderOther();
	LT.SRS( D3DRS_ALPHATESTENABLE, TRUE ); 
}

//*********************************************************************************
void	CPCFrontEnd::RenderStaticScreen(CTEXTURE *screen,int alpha, DWORD col)
{
	if (!screen)
	{
		PLATFORM.Font( FONT_DEBUG )->DrawText(16,16,0xFFFFFFFF,ToWCHAR("Frontend screen not found..."));
		return;
	}
	
#if TARGET == XBOX
	LT.STS(0,D3DTSS_MINFILTER,D3DTEXF_POINT);	// D3DTEXF_NONE is an invalid state on the xbox
	LT.STS(0,D3DTSS_MAGFILTER,D3DTEXF_POINT);
#else
	LT.STS(0,D3DTSS_MINFILTER,D3DTEXF_NONE);
	LT.STS(0,D3DTSS_MAGFILTER,D3DTEXF_NONE);
#endif
	
	LT.STS(0,D3DTSS_ADDRESSU,D3DTADDRESS_CLAMP);
	LT.STS(0,D3DTSS_ADDRESSV,D3DTADDRESS_CLAMP);	
	
	LT.SRS(D3DRS_ALPHATESTENABLE,FALSE);
	
	CSPRITERENDERER::DrawColouredSprite(0,0,0.1f,screen,(alpha<<24) | (col & 0x00ffffff),1.0f,1.0f);

	LT.STS(0,D3DTSS_MINFILTER,D3DTEXF_LINEAR);
	LT.STS(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR);

	LT.STS(0,D3DTSS_ADDRESSU,D3DTADDRESS_WRAP);
	LT.STS(0,D3DTSS_ADDRESSV,D3DTADDRESS_WRAP);
}

//*********************************************************************************
void	CPCFrontEnd::PlaySound(EFrontEndSound sound)
{
	char	*sample = GetSoundName(sound);

	SOUND.PlayNamedSample(sample, NULL);
}

//*********************************************************************************
char	*CPCFrontEnd::GetSoundName(EFrontEndSound sound)
{
	switch (sound)
	{
	case FES_MOVE:
		return "frontend\\move";

	case FES_SELECT:
		return "frontend\\select";

	default:
		return "";
	};
}

//*********************************************************************************

BOOL	CPCFrontEnd::RenderStart()
{
	STATE.UseDefault();
	RENDERINFO.Reset();

	// Setup Renderstates		
	STATE.UseDefault();
	PLATFORM.ClearScreen(0x000f0f2f);

	LT.SRS(D3DRS_ALPHABLENDENABLE,TRUE);	

	return(CFrontEnd::RenderStart());
}

//*********************************************************************************
void	CPCFrontEnd::RenderEnd(BOOL started)
{
	if (started)
	{
		// render common stuff
		RenderParticles();		
	}

	CFrontEnd::RenderEnd(started);
}



#endif