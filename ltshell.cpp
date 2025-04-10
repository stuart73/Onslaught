#include "common.h"

#if TARGET == PC

#include	"resource.h"
#include	"DX.h"
#include	"System.h"
#include	"CLIParams.h"

#include	"ParticleTexture.h"
#include	"tgaloader.h"

#include	"imposter.h"

#include	"VertexShader.h"
#include	"deviceobject.h"
#include	"console.h"
#include	"DebugText.h"

#include	"renderinfo.h"

#include	"imagehlp.h" 
#include	"process.h"

#include	"game.h"
#include	"modelviewer.h"
#include	"cutsceneeditor.h"
#include	"frontend.h"

#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "d3dx8.lib")
#pragma comment(lib, "d3d8.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dxof.lib")

PCLTShell		LT;
DWORD					PCLTShell::mRenderStates[N_RENDERSTATES];
DWORD					PCLTShell::mTextureStageStates[8][N_TEXTURESTAGESTATES];
#ifdef LT_DEBUG
DWORD					PCLTShell::mRenderStates2[N_RENDERSTATES];
DWORD					PCLTShell::mTextureStageStates2[8][N_TEXTURESTAGESTATES];
#endif
IDirect3DBaseTexture8*	PCLTShell::mCurrentTextures[8];
DWORD					PCLTShell::mLastVertexShader;

#ifndef EDITORBUILD2
#define PCLTShellSUPERTYPE	CD3DApplication
#else
#define PCLTShellSUPERTYPE	CEditorD3DApp
#endif 

//-----------------------------------------------------------------------------
DeviceObject*	PCLTShell::mDeviceObjects=0;

//*********************************************************************************
// Critical exception error handler
//*********************************************************************************

#define EWRITE0(x)								\
{												\
	sprintf(buffer,x);							\
	TRACE(buffer);								\
	fwrite(&buffer,strlen(buffer),1,fout);		\
}

#define EWRITE1(x,a)							\
{												\
	sprintf(buffer,x,a);						\
	TRACE(buffer);								\
	fwrite(&buffer,strlen(buffer),1,fout);		\
}

#define EWRITE2(x,a,b)							\
{												\
	sprintf(buffer,x,a,b);						\
	TRACE(buffer);								\
	fwrite(&buffer,strlen(buffer),1,fout);		\
}

#define EWRITE3(x,a,b,c)						\
{												\
	sprintf(buffer,x,a,b,c);					\
	TRACE(buffer);								\
	fwrite(&buffer,strlen(buffer),1,fout);		\
}

#define EWRITE4(x,a,b,c,d)						\
{												\
	sprintf(buffer,x,a,b,c,d);					\
	TRACE(buffer);								\
	fwrite(&buffer,strlen(buffer),1,fout);		\
}

void resolvesymbolmodule(DWORD addr,char *name)
{
	IMAGEHLP_MODULE modinfo;
	memset(&modinfo,0,sizeof(IMAGEHLP_MODULE));
	modinfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE);
		
	if (SymGetModuleInfo(GetCurrentProcess(),addr,&modinfo))
	{
		strcpy(name,modinfo.ModuleName);
	}
	else
	{
		strcpy(name,"N/A");
	}		
}

void resolvesymbolname(DWORD addr,char *name)
{
	DWORD displacement;
	byte staticspace[sizeof(IMAGEHLP_SYMBOL)+256];
	IMAGEHLP_SYMBOL *symbol=(IMAGEHLP_SYMBOL *) &staticspace;
	memset(symbol,0,sizeof(IMAGEHLP_SYMBOL)+256);
	symbol->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL)+256;
	symbol->MaxNameLength=256;
	
	if (SymGetSymFromAddr(GetCurrentProcess(),addr,&displacement,symbol))
	{
		if (displacement==0)
			sprintf(name,"%s",symbol->Name);
		else
			sprintf(name,"%s (+0x%08x)",symbol->Name,displacement);
	}
	else
	{
		strcpy(name,"N/A");
	}
}

BOOL resolvesymbolline(DWORD addr,char *filename,DWORD *lineno)
{
	IMAGEHLP_LINE line;
	memset(&line,0,sizeof(IMAGEHLP_LINE));
	line.SizeOfStruct=sizeof(IMAGEHLP_LINE);
	
	// Walk backward until we hit a valid line
	
	BOOL done=FALSE;
	DWORD offset=0;
	DWORD displacement;
	
	while ((!done) && (offset<128))
	{
		done=SymGetLineFromAddr(GetCurrentProcess(),addr-offset,&displacement,&line);
		offset++;
	}

	if (done)
	{
		strcpy(filename,line.FileName);
		*lineno=line.LineNumber;
		return(TRUE);
	}

	return(FALSE);
}

void resolvesymbolinfo(DWORD addr,char *name)
{
	char mod[4096];
	resolvesymbolmodule(addr,mod);
	resolvesymbolname(addr,name);
	strcat(name," (");
	strcat(name,mod);
	strcat(name,")");
}

LONG __stdcall ExceptionHandler(EXCEPTION_POINTERS *info)
{
	SetUnhandledExceptionFilter(NULL); // We don't want re-entrant exception handling!

	FILE *fout=fopen("OnslaughtException.txt","wt");

	char buffer[4096];
	char name[4096];

	EWRITE0("-- Onslaught exception handler --\n")

	// Setup symbol services

	char cli[4096];
	strcpy(cli,FromTCHAR(GetCommandLine()));

	char *start=cli;
	char *p=start;
	if (*p=='"')
	{
		// Find end quote and terminate string there
		p++;
		start++;
		while (*p!=0)
		{
			if (*p=='"')
			{
				*p=0;
			}
			p++;
		}
	}
	else
	{
		// Find first space and terminate there
		while (*p!=0)
		{
			if (*p==32)
				*p=0;
			p++;
		}
	}

	// Find last slash and terminate there

	char *lastslash=0;
	p=start;

	while (*p!=0)
	{
		if (*p=='\\')
			lastslash=p;
		p++;
	}

	if (lastslash)
		*lastslash=0;
	
	EWRITE1("\nExecutable path : %s\n",start);
		
	SymSetOptions(SYMOPT_LOAD_LINES);	
	
	if (!SymInitialize(GetCurrentProcess(),start,TRUE))
	{
		EWRITE0("\nFailed to initialise symbol services :-(\n\n");
	}
	
	// Write out exception details 

	EXCEPTION_RECORD *current=info->ExceptionRecord;

	int exceptionnum=0;

	while (current)
	{
		EWRITE1("\n--Exception info (%d)--\n\n",exceptionnum)
		EWRITE0("Type :				")
		if (current->ExceptionCode==EXCEPTION_ACCESS_VIOLATION)
		{
			EWRITE0("Access violation\n");
			if (current->NumberParameters>=2)
			{
				if (current->ExceptionInformation[0]==0)
					EWRITE0("Access : 			Read access attempted\n")
				else
					EWRITE0("Access : 			Write access attempted\n")
				EWRITE1("Address :			0x%08x\n",current->ExceptionInformation[1])
			}
			else
			{
				EWRITE0("Exception parameter frame invalid!?\n")
			}
		}
		else if (current->ExceptionCode==EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
			EWRITE0("Array bounds exceeded\n")
		else if (current->ExceptionCode==EXCEPTION_BREAKPOINT)
			EWRITE0("Breakpoint hit\n")
		else if (current->ExceptionCode==EXCEPTION_DATATYPE_MISALIGNMENT)
			EWRITE0("Data misalignment\n")
		else if (current->ExceptionCode==EXCEPTION_FLT_DENORMAL_OPERAND)
			EWRITE0("FPU denormal operand\n")
		else if (current->ExceptionCode==EXCEPTION_FLT_DIVIDE_BY_ZERO)
			EWRITE0("FPU divide by zero\n")
		else if (current->ExceptionCode==EXCEPTION_FLT_INEXACT_RESULT)
			EWRITE0("FPU inexact result(!)\n")
		else if (current->ExceptionCode==EXCEPTION_FLT_INVALID_OPERATION)
			EWRITE0("FPU invalid operation\n")
		else if (current->ExceptionCode==EXCEPTION_FLT_OVERFLOW)
			EWRITE0("FPU overflow\n")
		else if (current->ExceptionCode==EXCEPTION_FLT_STACK_CHECK)
			EWRITE0("FPU stack over/underflow\n")
		else if (current->ExceptionCode==EXCEPTION_FLT_UNDERFLOW)
			EWRITE0("FPU result underflow\n")
		else if (current->ExceptionCode==EXCEPTION_ILLEGAL_INSTRUCTION)
			EWRITE0("Illegal instruction\n")
		else if (current->ExceptionCode==EXCEPTION_IN_PAGE_ERROR)
			EWRITE0("Page in failed\n")
		else if (current->ExceptionCode==EXCEPTION_INT_DIVIDE_BY_ZERO)
			EWRITE0("Integer divide by zero\n")
		else if (current->ExceptionCode==EXCEPTION_INT_OVERFLOW)
			EWRITE0("Integer overflow\n")
		else if (current->ExceptionCode==EXCEPTION_INVALID_DISPOSITION)
			EWRITE0("Invalid exception handler disposition\n")
		else if (current->ExceptionCode==EXCEPTION_NONCONTINUABLE_EXCEPTION)
			EWRITE0("Noncontinuable exception was continued\n")
		else if (current->ExceptionCode==EXCEPTION_PRIV_INSTRUCTION)
			EWRITE0("Unauthorised privileged instruction execution\n")
		else if (current->ExceptionCode==EXCEPTION_SINGLE_STEP)
			EWRITE0("Single step trap\n")
		else if (current->ExceptionCode==EXCEPTION_STACK_OVERFLOW)
			EWRITE0("Stack overflow\n")
		else
			EWRITE1("*Unknown exception code (%d)*\n",current->ExceptionCode)
					
		EWRITE1("PC :				0x%08x\n",current->ExceptionAddress)

		if (current->ExceptionFlags==0)
			EWRITE0("Continuable :			Yes\n")
		else if (current->ExceptionFlags==EXCEPTION_NONCONTINUABLE)
			EWRITE0("Continuable :			No\n")
		else
			EWRITE1("*Unknown exception flags (%d)*\n",current->ExceptionFlags)
			                                    
		// Do symbol lookups

		DWORD pc=(DWORD) current->ExceptionAddress;
		HANDLE pid=GetCurrentProcess();

		IMAGEHLP_MODULE modinfo;
		memset(&modinfo,0,sizeof(IMAGEHLP_MODULE));
		modinfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE);

		resolvesymbolinfo(pc,name);

		EWRITE1("PC in :				%s\n",name)

		DWORD lineno;

		if (resolvesymbolline(pc,name,&lineno))
		{
			EWRITE1("File :				%s\n",name);
			EWRITE1("Line :				%d\n",lineno);
		}
		else
		{
			DWORD err=GetLastError();
			EWRITE1("Unable to resolve line (error 0x%08x)\n",err)
		}
		
		EWRITE0("\n--Stack trace--\n\n")
			
		STACKFRAME sf;
		memset(&sf,0,sizeof(sf));

		sf.AddrPC.Offset=pc;
		sf.AddrStack.Offset=info->ContextRecord->Esp;
		sf.AddrFrame.Offset=info->ContextRecord->Ebp;
		sf.AddrPC.Mode=AddrModeFlat;
		sf.AddrStack.Mode=AddrModeFlat;
		sf.AddrFrame.Mode=AddrModeFlat;
				
		int maxdump=128;
			
		while ((StackWalk(IMAGE_FILE_MACHINE_I386,GetCurrentProcess(),GetCurrentThread(),&sf,NULL,NULL,SymFunctionTableAccess,SymGetModuleBase,NULL)) && (maxdump>0))
		{		
			DWORD line;
			char filename[512];
			resolvesymbolname((DWORD) sf.AddrPC.Offset,name);
			resolvesymbolline((DWORD) sf.AddrPC.Offset,filename,&line);
			EWRITE2("0x%08x			%s\n",sf.AddrPC.Offset,name)
			EWRITE2("				%s line %d\n",filename,line)
			EWRITE4("				Parameters may be 0x%08x,0x%08x,0x%08x,0x%08x\n",sf.Params[0],sf.Params[1],sf.Params[2],sf.Params[3]);
			EWRITE1("				Stack frame at 0x%08x\n",sf.AddrFrame.Offset);

			if (sf.Virtual)
			{
				EWRITE0("				[Virtual frame]\n");
			}

			if (sf.Far)
			{
				EWRITE0("				[Far call]\n");
			}

			EWRITE0("\n");
			
			maxdump--;
		}

		EWRITE0("-Module info-\n\n");			
		
		if (SymGetModuleInfo(pid,pc,&modinfo))
		{
			EWRITE1("Module name :			%s\n",modinfo.ModuleName)
				EWRITE1("Base address :			0x%08x\n",modinfo.BaseOfImage)
				EWRITE1("Loaded symbols :		%d\n",modinfo.NumSyms)
				EWRITE1("Checksum :			0x%08x\n",modinfo.CheckSum)
		}
		else
		{
			EWRITE0("Could not resolve module information!\n")
		}		

		exceptionnum++;
		current=current->ExceptionRecord;				
	}

	EWRITE0("\n--End of exception info--\n\n")
	
	EWRITE0("--Context information--\n\n")

	CONTEXT *c=info->ContextRecord;

	EWRITE1("Dr0			=	0x%08x\n",c->Dr0)
	EWRITE1("Dr1			=	0x%08x\n",c->Dr1)
	EWRITE1("Dr2			=	0x%08x\n",c->Dr2)
	EWRITE1("Dr3			=	0x%08x\n",c->Dr3)
	EWRITE1("Dr6			=	0x%08x\n",c->Dr6)
	EWRITE1("Dr7			=	0x%08x\n",c->Dr7)

	EWRITE0("\n")
	
	EWRITE1("SegGs			=	0x%08x\n",c->SegGs)
	EWRITE1("SegFs			=	0x%08x\n",c->SegFs)
	EWRITE1("SegEs			=	0x%08x\n",c->SegEs)
	EWRITE1("SegDs			=	0x%08x\n",c->SegDs)

	EWRITE0("\n")

	resolvesymbolinfo(c->Edi,name);
	EWRITE2("EDI			=	0x%08x (%s)\n",c->Edi,name)
	resolvesymbolinfo(c->Esi,name);
	EWRITE2("ESI			=	0x%08x (%s)\n",c->Esi,name)
	resolvesymbolinfo(c->Eax,name);
	EWRITE2("EAX			=	0x%08x (%s)\n",c->Eax,name)
	resolvesymbolinfo(c->Ebx,name);
	EWRITE2("EBX			=	0x%08x (%s)\n",c->Ebx,name)
	resolvesymbolinfo(c->Ecx,name);
	EWRITE2("ECX			=	0x%08x (%s)\n",c->Ecx,name)
	resolvesymbolinfo(c->Edx,name);
	EWRITE2("EDX			=	0x%08x (%s)\n",c->Edx,name)

	EWRITE0("\n")

	resolvesymbolinfo(c->Ebp,name);
	EWRITE2("EBP			=	0x%08x (%s)\n",c->Ebp,name)
	resolvesymbolinfo(c->Eip,name);
	EWRITE2("EIP			=	0x%08x (%s)\n",c->Eip,name)
	resolvesymbolinfo(c->Esp,name);
	EWRITE2("ESP			=	0x%08x (%s)\n",c->Esp,name)
	resolvesymbolinfo(c->SegCs,name);
	EWRITE2("SegCs			=	0x%08x (%s)\n",c->SegCs,name)
	resolvesymbolinfo(c->SegSs,name);
	EWRITE2("SegSs			=	0x%08x (%s)\n",c->SegSs,name)
	EWRITE1("EFlags			=	0x%08x\n",c->EFlags)

	EWRITE0("\n--End of exception dump--\n")

	fclose(fout);

	// Trigger userdump

	char arga[256];
	char argb[256];
	sprintf(arga,"userdump.exe");
	sprintf(argb,"%d",getpid());

//	DeleteFile("onslaught.dmp");
	
//	execl("userdump.exe",arga,argb,NULL);

	return(EXCEPTION_EXECUTE_HANDLER);
}

#include "debuglog.h"

//-----------------------------------------------------------------------------
#ifndef EDITORBUILD
//-----------------------------------------------------------------------------
void ExitCode()
{
	TRACE("Cleaning up memory manager...\n");
	MEM_MANAGER.SetMerge( TRUE );
	MEM_MANAGER.Cleanup();

	// We can't call this because we can't be sure all the global destructors have been called...

	TRACE("Shutting down memory manager...\n");
	MEM_MANAGER.Shutdown();

	TRACE("Memory Manager Shutdown...\n");
}
//inline int	 Qf(float x) { 
//	float t=((x-0.5f/256.0f))+(float)(1.0f*(1<<15));
//	return *(int*)&t; 
//}
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// --- Setup debugging services ---	

	//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);	
	
	// Setup our exception handler
	
	SetUnhandledExceptionFilter(&ExceptionHandler);
		
#ifdef _DEBUG
	// Turn on FPU exceptions (except FPU_INEXACT and FPU_UNDERFLOW)
	// Note that d3dapp.cpp needs to turn on the FPU_PRESERVE flag
	// for the device in order for this to work!
	
	_controlfp(_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID, _MCW_EM);
#endif

	// --- End of debugging services ---

	// Set up our exit handler

	atexit(ExitCode);

	PLATFORM.SetMemorySize(96*1024*1024); // Default memory size

	// JCL - process command line
	CLIPARAMS.GetParams(lpCmdLine);

	LOG.AddMessage("Init game with %d bytes", PLATFORM.GetMemorySize()) ;
	
#ifndef VANILLA_MEMORYMANAGER
	MEM_MANAGER.Init(PLATFORM.GetMemorySize());
#endif	

	LT.mhInstance=hInstance;
	
	if( FAILED( LT.Create( hInstance/*, GetCommandLine()*/ ) ) )
        return 0;

	if(!LT.SetupSystemMessageHandler()) return LT.MsgInfo();

	LT.SetRunning(true);
	return LT.MainLoop();
}
#endif
//-----------------------------------------------------------------------------
void con_whatami(char *cmd)
{
	LT.ShowGFXCardInfo();
}
//-----------------------------------------------------------------------------
int PCLTShell::MainLoop()
{
	// JCL - new main loop - bit simpler!

	if (SYSTEM.Init())
	{
		CONSOLE.RegisterCommand("cg_whatami","Shows the current graphics card details",&con_whatami);
		CONSOLE.Print("Memory heap at %dMb\n",(PLATFORM.GetMemorySize()/(1024*1024)));
		//if(mJoypads!=0) // We can't do this because it causes the modelviewer to exit!
			SYSTEM.Run();

		SYSTEM.Shutdown();
		Cleanup3DEnvironment();
	}

/*	if( FAILED( hr = GAME.Init())) return 0;
	
	while(!LT.WantsToQuit())
	{
		if(!LT.HandlingSystemStuff())
		{
			if( FAILED( hr = GAME.Update() ) ) LT.IWantToQuit();
			if( FAILED( hr = GAME.Process() ) ) LT.IWantToQuit();
		}
	}
	GAME.Shutdown();						// this doesn't work after shutdowns below :-(*/

    DestroyMenu( GetMenu(LT.GetHWnd()) );
    DestroyWindow( LT.GetHWnd() );

//	if ( _CrtDumpMemoryLeaks() )
//		::MessageBox( NULL , "Memory leaks detected on exit!\nCheck debugging traces for details." ,
//						  "Onslaught" , MB_ICONEXCLAMATION|MB_OK );

	return LT.MsgInfo();
}

//-----------------------------------------------------------------------------
BOOL PCLTShell::SetupSystemMessageHandler()
{
    // Load keyboard accelerators
    mhAccel = LoadAccelerators( NULL, MAKEINTRESOURCE(IDR_MAIN_ACCEL) );

    // Now we're ready to recieve and process Windows messages.
    PeekMessage( &mMsg, NULL, 0U, 0U, PM_NOREMOVE );
	if( WM_QUIT != mMsg.message) return TRUE;
	else
	{
		ASSERT(0);	// does this ever happen?
		return FALSE;
	}
}
//-----------------------------------------------------------------------------
BOOL PCLTShell::HandlingSystemStuff()
{
    BOOL bGotMsg;
	BOOL cant_proc=TRUE;
    // Use PeekMessage() if the app is active, so we can use idle time to
    // render the scene. Else, use GetMessage() to avoid eating CPU time.
    if( m_bActive )
        bGotMsg = PeekMessage( &mMsg, NULL, 0U, 0U, PM_REMOVE );
    else
        bGotMsg = GetMessage( &mMsg, NULL, 0U, 0U );

    if( bGotMsg )
    {
        // Translate and dispatch the message
        if( 0 == TranslateAccelerator( m_hWnd, mhAccel, &mMsg ) )
        {
            TranslateMessage( &mMsg );
            DispatchMessage( &mMsg );
			cant_proc =	TRUE;
        }
    }
    else
    {
        // Render a frame during idle time (no messages are waiting)
        if( m_bActive && m_bReady )
        {
			cant_proc =	FALSE;
        }
    }

	if(!cant_proc) // stuff from top of Render3DEnvironment()
	{

		HRESULT hr;
		// Test the cooperative level to see if it's okay to render
		if( FAILED( hr = m_pd3dDevice->TestCooperativeLevel() ) )
		{
			// If the device was lost, do not render until we get it back
			if( D3DERR_DEVICELOST == hr )
				cant_proc = TRUE;

			// Check if the device needs to be resized.
			if( D3DERR_DEVICENOTRESET == hr )
			{
				// If we are windowed, read the desktop mode and use the same format for
				// the back buffer
				if( m_bWindowed )
				{
					D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
					m_pD3D->GetAdapterDisplayMode( m_dwAdapter, &pAdapterInfo->d3ddmDesktop );
					m_d3dpp.BackBufferFormat = pAdapterInfo->d3ddmDesktop.Format;
				}

				if( FAILED( hr = Resize3DEnvironment() ) )
				{
		            SendMessage( LT.GetHWnd(), WM_CLOSE, 0, 0 );
					cant_proc = TRUE;
				}
			}
			cant_proc = TRUE;
		}

		// Get the app's time, in seconds. Skip rendering if no time elapsed
		FLOAT fAppTime        = DXUtil_Timer( TIMER_GETAPPTIME );
		FLOAT fElapsedAppTime = DXUtil_Timer( TIMER_GETELAPSEDTIME );
		if( ( 0.0f == fElapsedAppTime ) && m_bFrameMoving )
			return S_OK;


// Not sure about this shite

		// FrameMove (animate) the scene
		if( m_bFrameMoving || m_bSingleStep )
		{
			// Store the time for the app
			m_fTime        = fAppTime;
			m_fElapsedTime = fElapsedAppTime;

			// Frame move the scene
//			if( FAILED( hr = FrameMove() ) )
//				return hr;

			m_bSingleStep = FALSE;
		}

// end of shite
	}

// process joystick

	for (int i=0;i<MAX_JOYPADS;i++)
	{
		UpdateJoystick(i);
	}
	
/*	if(mJoyState[0]->rgbButtons[11])				// hack to turn full screen on
	{
		if( m_bActive && m_bReady )
		{
			if( m_bWindowed )
				GetWindowRect( m_hWnd, &m_rcWindowBounds );

			if( FAILED( ToggleFullscreen() ) )
			{
				DisplayErrorMsg( D3DAPPERR_RESIZEFAILED, MSGERR_APPMUSTEXIT );
				return 0;
			}
		}
	}*/
	
	return	cant_proc;
}
//-----------------------------------------------------------------------------
void PCLTShell::DisplayScreen()
{
    // Keep track of the frame count
  /*  {
        static FLOAT fLastTime = 0.0f;
        static DWORD dwFrames  = 0L;
        FLOAT fTime = DXUtil_Timer( TIMER_GETABSOLUTETIME );
        ++dwFrames;

        // Update the scene stats once per second
        if( fTime - fLastTime > 1.0f )
        {
            m_fFPS    = dwFrames / (fTime - fLastTime);
            fLastTime = fTime;
            dwFrames  = 0L;

            // Get adapter's current mode so we can report
            // bit depth (back buffer depth may be unknown)
            D3DDISPLAYMODE mode;
            m_pD3D->GetAdapterDisplayMode(m_dwAdapter, &mode);

			char buffer[256];
			sprintf(buffer,"%.02f fps (%dx%dx%d)", m_fFPS,
					m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height,
					mode.Format==D3DFMT_X8R8G8B8?32:16 );

			_tcscpy(m_strFrameStats,ToTCHAR(buffer));

            if( m_bUseDepthBuffer )
            {
                D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
                D3DDeviceInfo*  pDeviceInfo  = &pAdapterInfo->devices[pAdapterInfo->dwCurrentDevice];
                D3DModeInfo*    pModeInfo    = &pDeviceInfo->modes[pDeviceInfo->dwCurrentMode];

                switch( pModeInfo->DepthStencilFormat )
                {
                case D3DFMT_D16:
                    lstrcat( m_strFrameStats, ToTCHAR(" (D16)") );
                    break;
                case D3DFMT_D15S1:
                    lstrcat( m_strFrameStats, ToTCHAR(" (D15S1)") );
                    break;
                case D3DFMT_D24X8:
                    lstrcat( m_strFrameStats, ToTCHAR(" (D24X8)") );
                    break;
                case D3DFMT_D24S8:
                    lstrcat( m_strFrameStats, ToTCHAR(" (D24S8)") );
                    break;
                case D3DFMT_D24X4S4:
                    lstrcat( m_strFrameStats, ToTCHAR(" (D24X4S4)") );
                    break;
                case D3DFMT_D32:
                    lstrcat( m_strFrameStats, ToTCHAR(" (D32)") );
                    break;
                }
            }
        }
    }
	*/
    // Show the frame on the primary surface.
    m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
PCLTShell::PCLTShell()
                  :PCLTShellSUPERTYPE()
{
	pDI					= NULL;  
	for (int i=0;i<MAX_JOYPADS;i++)
	{
		pJoystick[i]	= NULL;
		mRumbleInitialised[i] = FALSE;		
		mJoyState[i]=NULL;
		mOldJoyState[i]=NULL;
	}
	mRumbleEnabled		= TRUE;
    m_strWindowTitle    = ToTCHAR("Battle Engine Aquila");
    m_bUseDepthBuffer   = TRUE;
//    m_DepthBufferFormat = D3DFMT_UNKNOWN_D16;

	m_bWindowed		= FALSE;

	mJoypads=0;
	
	m_bTextureCompressionDisabled = FALSE;
}

PCLTShell::~PCLTShell()
{

}

//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: This creates all device-dependant managed objects, such as managed
//       textures and managed vertex buffers.
//-----------------------------------------------------------------------------
HRESULT PCLTShell::InitDeviceObjects()
{
	HRESULT res;

	mD3DActive = TRUE;


	//! JCL
//	if (res != S_OK)
//		return res;

	for (int i=0;i<MAX_JOYPADS;i++)
	{
		mJoyState[i]	= new( MEMTYPE_UNKNOWN ) DIJOYSTATE2;
		mOldJoyState[i]	= new( MEMTYPE_UNKNOWN ) DIJOYSTATE2;
	}

    // Initialize device objects
	DeviceObject* devob = mDeviceObjects;
	while(devob)
	{
		devob->InitDeviceObjects();
		devob=devob->mNext;
	}

	//##GC		This is a bit naughty really...
	CIMPOSTER::InvalidateImposters();
	//!PD
	CPARTICLETEXTURE::RecreateAllSurfaces();

	res = InitDirectInput( m_hWnd );
	
	if(res==S_FALSE) return S_FALSE;
	
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Restore device-memory objects and state after a device is created or
//       resized.
//-----------------------------------------------------------------------------
HRESULT PCLTShell::RestoreDeviceObjects()
{
	DeviceObject* devob = mDeviceObjects;
	while(devob)
	{
		HRESULT	result=devob->RestoreDeviceObjects();
		
		if (result!=S_OK)
			devob=devob;

		devob=devob->mNext;
	}
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Called when the device-dependant objects are about to be lost.
//-----------------------------------------------------------------------------
HRESULT PCLTShell::InvalidateDeviceObjects()
{
	DeviceObject* devob = mDeviceObjects;
	while(devob)
	{
		devob->InvalidateDeviceObjects();
		devob=devob->mNext;
	}
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//       this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
HRESULT PCLTShell::DeleteDeviceObjects()
{	
	for (int i=0;i<MAX_JOYPADS;i++)
	{
		SAFE_DELETE(mJoyState[i]);
		SAFE_DELETE(mOldJoyState[i]);
	}

	DeviceObject* devob = mDeviceObjects;
	while(devob)
	{
		HRESULT hr = devob->DeleteDeviceObjects();
		ASSERT(hr==S_OK);
		devob=devob->mNext;
	}
	FreeDirectInput();

	CPARTICLETEXTURE::FreeAllSurfaces();

	mD3DActive = FALSE;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT PCLTShell::FinalCleanup()
{
//	GAME.Shutdown();

	mRunning=false;
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device intialization, this code checks the device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT PCLTShell::ConfirmDevice( D3DCAPS8* pCaps, DWORD dwBehavior,
                                          D3DFORMAT Format )
{
	if (CVertexShader::VertexShadersEnabled)
	{
		// Ensure we have vertex shader support

/*		int majorv=((pCaps->VertexShaderVersion >> 8) & 0xFF);
		int minorv=(pCaps->VertexShaderVersion & 0xFF);

		char buffer[256];
		sprintf(buffer,"VShader v%d.%d\n",majorv,minorv);
		OutputDebugString(buffer);*/

		if ((dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING) ||
			(dwBehavior & D3DCREATE_MIXED_VERTEXPROCESSING))
		{
			// Assume we'll be OK with emulation - this is *bad*, but there 
			// doesn't seem to be another way
		}
		else
		{
			// Hardware VP
			if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,0) )
	            return E_FAIL;
		}
	}

	// This is necessary to make the reference rasteriser work as it will
	// default to using a pure device which disallows stuff we're doing

	if (CLIPARAMS.mPureDevice)
	{
		if (!(dwBehavior & D3DCREATE_PUREDEVICE))
			return E_FAIL;
	}
	else
	{
		if ((dwBehavior & D3DCREATE_PUREDEVICE))
			return E_FAIL;
	}

    if ((!(pCaps->TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE)) &&
        (!(pCaps->TextureCaps & D3DPTEXTURECAPS_ALPHA)))
   		return E_FAIL;

    return S_OK;
}

    //======================================================-==--=-- --  -
   //
  // stuff added for LT. shell
 //
//======================================================-==--=-- --  -
BOOL	PCLTShell::RemoveDeviceObject(DeviceObject* devob)
{
	DeviceObject*	tdos = mDeviceObjects;
	DeviceObject*	prevdo = 0;
	while(tdos)
	{
		if(tdos==devob)
		{
			if(prevdo)		prevdo->mNext = devob->mNext;
			else			mDeviceObjects = devob->mNext;
			return TRUE;
		}
		prevdo = tdos;	
		tdos=tdos->mNext;
	}
	return FALSE;
}

    //======================================================-==--=-- --  -
   //
  // stuff added for LT. shell
 //
//======================================================-==--=-- --  -
LRESULT PCLTShell::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    // Trap the about box command
    if( WM_COMMAND == uMsg && LOWORD(wParam) == IDM_ABOUT )
        return 0;

    // Record key presses
    if( WM_KEYDOWN == uMsg )
    {
		if (mCurrentKeytrap)
		{
			mCurrentKeytrap(wParam,KEY_EVENT_DOWN);
		}
		else
		{
			CONSOLE.HandleBind(wParam,KEY_EVENT_DOWN);
			KeyDown[wParam] = TRUE;
			KeyWasDown[wParam] = TRUE;	
		}
    }

    // Perform commands when keys are rleased
    if( WM_KEYUP == uMsg )
    {
		if (mCurrentKeytrap)
		{
			mCurrentKeytrap(wParam,KEY_EVENT_UP);
		}
		else
		{
			CONSOLE.HandleBind(wParam,KEY_EVENT_UP);
		}

		// Propogate keyup events even when trapping them
		// to avoid stuck keys

        KeyDown[wParam] = FALSE;
    }

    // Check for character messages

    if( WM_CHAR == uMsg )
    {
		if (mCurrentKeytrap)
		{
			mCurrentKeytrap(wParam,KEY_EVENT_CHAR);
		}
    }

    // Record mouse position

    if ((uMsg==WM_MOUSEMOVE) || 
		(uMsg==WM_LBUTTONDOWN) || (uMsg==WM_LBUTTONUP) ||
		(uMsg==WM_MBUTTONDOWN) || (uMsg==WM_MBUTTONUP) ||
		(uMsg==WM_RBUTTONDOWN) || (uMsg==WM_RBUTTONUP))
    {
		mMouseButtons = wParam; 
		mMouseX = LOWORD(lParam);
		mMouseY = HIWORD(lParam);		
    }
	
    return PCLTShellSUPERTYPE::MsgProc( hWnd, uMsg, wParam, lParam );
}

//***********************************************************************************8
void	PCLTShell::FlushInputBuffers()
{
	SINT	c0;
	for (c0 = 0; c0 < 256; c0 ++)
	{
		KeyDown[c0] = FALSE;
		KeyWasDown[c0] = FALSE;	
	}
}

//-----------------------------------------------------------------------------
// Name: EnumJoysticksCallback()
// Desc: Called once for each enumerated joystick. If we find one, create a
//       device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK PCLTShell::EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext )
{
    HRESULT		hr;
	PCLTShell*	sneaky_this = (PCLTShell*) pContext;
	ASSERT(sneaky_this);
	LPDIRECTINPUTDEVICE8 pad;
    // Obtain an interface to the enumerated joystick.
    hr = sneaky_this->pDI->CreateDevice( pdidInstance->guidInstance, &pad, NULL );

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if( FAILED(hr) ) 
        return DIENUM_CONTINUE;

	LT.pJoystick[LT.mJoypads]=pad;
	LT.mJoypads++;

    return DIENUM_CONTINUE;
}
//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick
//-----------------------------------------------------------------------------
BOOL CALLBACK PCLTShell::EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext )
{
    int num = (int) pContext;

    DIPROPRANGE diprg; 
    diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    diprg.diph.dwHow        = DIPH_BYOFFSET; 
    diprg.diph.dwObj        = pdidoi->dwOfs; // Specify the enumerated axis
    diprg.lMin              = -1000; 
    diprg.lMax              = +1000; 
    
    // Set the range for the axis
    if( FAILED( LT.pJoystick[num]->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
        return DIENUM_STOP;

    // Set some flags
    switch( pdidoi->dwOfs )
    {
        case DIJOFS_X:
			LT.DodgyJoyX[num] = FALSE;
            break;
        case DIJOFS_Y:
            break;
        case DIJOFS_Z:
			LT.mJoypadIsNewtype[num]	= TRUE;
            break;
        case DIJOFS_RX:
            break;
        case DIJOFS_RY:
            break;
        case DIJOFS_RZ:
            break;
        case DIJOFS_SLIDER(0):
            break;
        case DIJOFS_SLIDER(1):
            break;
    }

    return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Callback for DirectInput force feedback enumeration
// This creates the desired effect objects and uploads them to
// the controller
//-----------------------------------------------------------------------------

BOOL CALLBACK PCLTShell::EnumEffectsProc(LPCDIEFFECTINFO pei, LPVOID pv)
{
	int num=(int) pv;

	if (strcmp(FromTCHAR(pei->tszName),"Sine")==0)
	{
		DWORD      dwAxes[2] = {DIJOFS_X, DIJOFS_Y};
		LONG       lDirection[2] = {0, 0};
		
		DIPERIODIC diPeriodic;
		DIENVELOPE diEnvelope;
		DIEFFECT   diEffect;  
		
		diPeriodic.dwMagnitude = DI_FFNOMINALMAX; 
		diPeriodic.lOffset = 0; 
		diPeriodic.dwPhase = 0; 
		diPeriodic.dwPeriod = (DWORD)(0.05 * DI_SECONDS); 
		diEnvelope.dwSize = sizeof(DIENVELOPE);
		diEnvelope.dwAttackLevel = 0; 
		diEnvelope.dwAttackTime = (DWORD)(0.5 * DI_SECONDS); 
		diEnvelope.dwFadeLevel = 0; 
		diEnvelope.dwFadeTime = (DWORD)(1.0 * DI_SECONDS); 
		diEffect.dwSize = sizeof(DIEFFECT); 
		diEffect.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS; 
		diEffect.dwDuration = (DWORD)(0.25 * DI_SECONDS);
		
		diEffect.dwSamplePeriod = 0;
		diEffect.dwGain = DI_FFNOMINALMAX;
		diEffect.dwTriggerButton = DIEB_NOTRIGGER;
		diEffect.dwTriggerRepeatInterval = 0;      
		diEffect.cAxes = 2; 
		diEffect.rgdwAxes = dwAxes; 
		diEffect.rglDirection = &lDirection[0]; 
		diEffect.lpEnvelope = &diEnvelope; 
		diEffect.cbTypeSpecificParams = sizeof(diPeriodic);
		diEffect.lpvTypeSpecificParams = &diPeriodic;  
		
		HRESULT hr;
		hr=LT.pJoystick[num]->CreateEffect(pei->guid,&diEffect,&(LT.mRumbleEffect[num]),NULL);
		if FAILED(hr)
		{
			TRACE("Force-feedback effect creation failed!\n");
			LT.mRumbleEffect[num]=NULL;
		}
	}

    return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT PCLTShell::InitDirectInput( HWND hDlg )
{
    HRESULT hr;

	for (int i=0;i<MAX_JOYPADS;i++)
	{
		mJoypadIsNewtype[i] = FALSE;
		DodgyJoyX[i] = TRUE;
	}
	mDeadZone = 150;
    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    // Create a DInput object
    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
                                         IID_IDirectInput8, (VOID**)&pDI, NULL ) ) )
        return hr;


	mJoypads=0;

	// Enumerate available pads

    if( FAILED( hr = pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, 
                                         EnumJoysticksCallback,
                                         this, DIEDFL_ATTACHEDONLY ) ) )
        return hr;

    // Make sure we got at least one joypad

	BOOL modelviewerorcutsceneeditor = FALSE;
#ifdef DEV_VERSION
	modelviewerorcutsceneeditor = CLIPARAMS.mModelViewer || CLIPARAMS.mCutsceneEditor;
#endif
    if ((mJoypads==0) && (!modelviewerorcutsceneeditor) && (!CLIPARAMS.mBuildResources))
    {
        int r=MessageBox( NULL, ToTCHAR("This game requires at least one joypad!"), 
                    ToTCHAR("Battle Engine Aquila"), 
                    MB_ICONERROR | MB_OKCANCEL );
        EndDialog( hDlg, 0 );

		if (r==IDCANCEL)
			exit(-1);

        return S_FALSE;
    }

	if (mJoypads>MAX_JOYPADS)
		mJoypads=MAX_JOYPADS;

	// Now run through the pads and set them up

	for (i=0;i<mJoypads;i++)
	{
	    if( FAILED( hr = pJoystick[i]->SetDataFormat( &c_dfDIJoystick2 ) ) )
		    return hr;
	    if( FAILED( hr = pJoystick[i]->SetCooperativeLevel(hDlg, DISCL_EXCLUSIVE | DISCL_FOREGROUND)))
		    return hr;
	
		diDevCaps.dwSize = sizeof(DIDEVCAPS);
		if ( FAILED( hr = pJoystick[i]->GetCapabilities(&diDevCaps) ) )
			return hr;

		// Enumerate axes

	    if ( FAILED( hr = pJoystick[i]->EnumObjects( EnumAxesCallback,(VOID*) i, DIDFT_AXIS ) ) )
			return hr;
	}

	// Rearrange pads to put newtype pads at the top of the list

	for (i=0;i<mJoypads;i++)
	{
		for (int j=i+1;j<mJoypads;j++)
		{
			if ((mJoypadIsNewtype[j]) && (!mJoypadIsNewtype[i]))
			{
				bool tempb;
				LPDIRECTINPUTDEVICE8 tempd;

				tempb=mJoypadIsNewtype[i];
				mJoypadIsNewtype[i]=mJoypadIsNewtype[j];
				mJoypadIsNewtype[j]=tempb;

				tempb=DodgyJoyX[i];
				DodgyJoyX[i]=DodgyJoyX[j];
				DodgyJoyX[j]=tempb;

				tempd=pJoystick[i];
				pJoystick[i]=pJoystick[j];
				pJoystick[j]=tempd;
			}
		}
	}

	CONSOLE.Print("Found %d joypads\n",mJoypads);

    return S_OK;
}

//-----------------------------------------------------------------------------
// Initialise the rumble effects
//-----------------------------------------------------------------------------

void	PCLTShell::InitialiseRumble(int joypad)
{
	if (joypad >= mJoypads) return;


	// This is done on-demand as downloading the effect triggers it
	// for some reason...
	
	// Enumerate force-feedback effects and find one
	// we can use for rumble
	
	mRumbleEffect[joypad]=NULL;
	
	if (mRumbleEnabled)
	{
		if (pJoystick[joypad])
		{
			if (FAILED(pJoystick[joypad]->EnumEffects(EnumEffectsProc,(void *) joypad,DIEFT_PERIODIC)))
				mRumbleEffect[joypad]=NULL;
		}
	}

	mRumbleInitialised[joypad]=true;
}

//-----------------------------------------------------------------------------
// Trigger a rumble effect
//-----------------------------------------------------------------------------

void	PCLTShell::TriggerRumble(int pad)
{
	if (pad >= mJoypads) return;

	if (!mRumbleEnabled)
		return;
	if (!mRumbleInitialised[pad])
		InitialiseRumble(pad);
	
	if (mRumbleEffect[pad])
		mRumbleEffect[pad]->Start(1,0);
}

//-----------------------------------------------------------------------------
// Name: UpdateInputState()
// Desc: Get the input device's state and display it.
//-----------------------------------------------------------------------------
HRESULT PCLTShell::UpdateJoystick(int joypad)
{
	if (joypad >= mJoypads) return S_OK;

    HRESULT     hr;

	if ((!mJoyState[joypad]) || (!mOldJoyState[joypad]))
		return(S_OK);

	if (joypad>=mJoypads)
	{
		// Pad is not connected, so no buttons should be pressed
		memset(mOldJoyState[joypad],0,sizeof(DIJOYSTATE2));
		memset(mJoyState[joypad],0,sizeof(DIJOYSTATE2));
		return(S_OK);
	}

	DIJOYSTATE2* t	= mJoyState[joypad];
	mJoyState[joypad]= mOldJoyState[joypad];
	mOldJoyState[joypad]= t;
	memset(mJoyState[joypad],0,sizeof(DIJOYSTATE2));

    if( NULL == pJoystick[joypad] ) 
        return S_OK;

    // Poll the device to read the current state
    hr = pJoystick[joypad]->Poll(); 
    if( FAILED(hr) )  
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = pJoystick[joypad]->Acquire();
        while( hr == DIERR_INPUTLOST ) 
            hr = pJoystick[joypad]->Acquire();

        // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
        // may occur when the app is minimized or in the process of 
        // switching, so just try again later 
        return S_OK; 
    }

    // Get the input's device state
    if( FAILED( hr = pJoystick[joypad]->GetDeviceState( sizeof(DIJOYSTATE2), mJoyState[joypad] ) ) )
        return hr; // The device should have been acquired during the Poll()

	
// Hack for Joybox problem under DX8	
/*	
	if(DodgyJoyX[joypad])
	{
		mJoyState[joypad]->lX = ((mJoyState[joypad]->lX-32768)*1000)>>15;
		if(mJoyState[joypad]->lX>0) mJoyState[joypad]->lX++;
		mJoypadIsNewtype[joypad]=FALSE;
	}
*/

	if(mJoypadIsNewtype[joypad])		// hack the buttons into 'joybox' format....
	{			
		UBYTE* bp=mJoyState[joypad]->rgbButtons;
		UBYTE t=bp[0];
		bp[0]=bp[1];
		bp[1]=bp[2];
		bp[2]=bp[3];
		bp[3]=t;
	}

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
VOID PCLTShell::FreeDirectInput()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
	for (int i=0;i<mJoypads;i++)
	{
	    if( pJoystick[i] ) 
		    pJoystick[i]->Unacquire();
    
		SAFE_RELEASE( pJoystick[i] );
	}
    SAFE_RELEASE( pDI );
}
//-----------------------------------------------------------------------------
int	PCLTShell::DeadZone(const int axis)
{
	if(axis>0)
	{
		int dz =axis-mDeadZone;
		if(dz<0) return 0;
		else return dz;
	}
	else
	{
		int dz =axis+mDeadZone;
		if(dz>0) return 0;
		else return dz;
	}
}
//-----------------------------------------------------------------------------
void PCLTShell::ForceFile(char* dst, char* src, char* path)
{
	int d=strlen(src);
	int slashat=0;
	for(int i=0; i<d; i++) if(src[i] == '\\') slashat = i;
	ASSERT(slashat);
	sprintf(dst,"%s%s", path, &src[slashat]);
}
//-----------------------------------------------------------------------------
void PCLTShell::DumpScreen(int number)
{
	LPDIRECT3DSURFACE8 backbuffer;
	D3DSURFACE_DESC desc;

	LT.D3D_GetRenderTarget( &backbuffer );
	backbuffer->GetDesc(&desc);
	if((desc.Format==D3DFMT_A8R8G8B8)||(desc.Format == D3DFMT_X8R8G8B8))
	{
		D3DLOCKED_RECT rect;
		if(!FAILED(backbuffer->LockRect(&rect,NULL,D3DLOCK_READONLY)))
		{
			char name[32];
			sprintf(name,"grabs\\scr%.4d.tga",number);
			CTGALoader::Save24BitWithPitch( name, (ULONG*)rect.pBits, desc.Width,
											desc.Height, rect.Pitch);
			backbuffer->UnlockRect();
		}
	}
	backbuffer->Release();
}
//-----------------------------------------------------------------------------
void PCLTShell::SetKeytrap(pKeyTrapper trap)
{
	mCurrentKeytrap=trap;
}
//-----------------------------------------------------------------------------
void PCLTShell::InvalidateAllStates()
{
	int i, ii;
	for(i=0; i<N_RENDERSTATES; i++) mRenderStates[i]=INVALID_STATE;
	for(ii=0; ii<8; ii++) for(i=0; i<N_TEXTURESTAGESTATES; i++) mTextureStageStates[ii][i]=INVALID_STATE;
#ifdef	LT_DEBUG
	for(i=0; i<N_RENDERSTATES; i++) mRenderStates2[i]=INVALID_STATE;
	for(ii=0; ii<8; ii++) for(i=0; i<N_TEXTURESTAGESTATES; i++) mTextureStageStates2[ii][i]=INVALID_STATE;
#endif
	for(i=0; i<8; i++) mCurrentTextures[i] = (IDirect3DBaseTexture8*)INVALID_STATE;
	mLastVertexShader = INVALID_STATE;
}
//-----------------------------------------------------------------------------
#ifdef LT_DEBUG
char PCLTShell::mNotRI[N_RENDERSTATES] = {	1,1,1,1,1,1,1,1,1,1,	// 0-9 
											1,1,1,1,1,1,1,1,1,1,	// 10-19
											1,1,1,1,1,1,1,1,		// 20-27
											0,						// 28 (fog)				
											1,1,1,1,1,				// 29-33
											0,0,0,0,0,				// 34-38 (fog)
											1,1,1,1,1,1,1,1,1,		// 49-47
											0,						// 48 (fog)
											1,						// 49
											1,1,1,1,1,1,1,1,1,1,	// 50-59
											1,1,1,1,1,1,1,1,1,1,	// 60-69
											1,1,1,1,1,1,1,1,1,1,	// 70-79
											1,1,1,1,1,1,1,1,1,1,	// 80-89
											1,1,1,1,1,1,1,1,1,1,	// 90-99
											1,1,1,1,1,1,1,1,1,1,	// 100-109
											1,1,1,1,1,1,1,1,1,1,	// 110-119
											1,1,1,1,1,1,1,1,1,1,	// 120-129
											1,1,1,1,1,1,1,			// 130-136
											0,						// 137 (light enable)
											1,						// 138
											0,						// 139 (ambient)
											0,						// 140 (one last fog thingy)
											1,1,1,1,1,1,1,1,1,		// 141-149
											1,1,1,1,1,1,1,1,1,1,	// 150-159
											1,1,1,1,1,1,1,1,1,1,	// 160-169
											1,1 };					// 170-171
char* g_rsnames[N_RENDERSTATES] = {
	"0","1","2","3","4","5","6",
	"D3DRS_ZENABLE",	//                   = 7,    /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
	"D3DRS_FILLMODE",	//                  = 8,    /* D3DFILL_MODE        */
	"D3DRS_SHADEMODE",	//                 = 9,    /* D3DSHADEMODE */
	"D3DRS_LINEPATTERN",//               = 10,   /* D3DLINEPATTERN */
	"11","12","13",
	"D3DRS_ZWRITEENABLE",	//              = 14,   /* TRUE to enable z writes */
	"D3DRS_ALPHATESTENABLE",//           = 15,   /* TRUE to enable alpha tests */
	"D3DRS_LASTPIXEL",//                 = 16,   /* TRUE for last-pixel on lines */
	"17","18",
	"D3DRS_SRCBLEND",		//                  = 19,   /* D3DBLEND */
	"D3DRS_DESTBLEND",	//                 = 20,   /* D3DBLEND */
	"21",
	"D3DRS_CULLMODE",	//                  = 22,   /* D3DCULL */
	"D3DRS_ZFUNC",		//                     = 23,   /* D3DCMPFUNC */
	"D3DRS_ALPHAREF",	//                  = 24,   /* D3DFIXED */
	"D3DRS_ALPHAFUNC",	//                 = 25,   /* D3DCMPFUNC */
	"D3DRS_DITHERENABLE",	//              = 26,   /* TRUE to enable dithering */
	"D3DRS_ALPHABLENDENABLE",	//          = 27,   /* TRUE to enable alpha blending */
	"D3DRS_FOGENABLE",	//                 = 28,   /* TRUE to enable fog blending */
	"D3DRS_SPECULARENABLE",	//            = 29,   /* TRUE to enable specular */
	"D3DRS_ZVISIBLE",	//                  = 30,   /* TRUE to enable z checking */
	"31","32","33",
	"D3DRS_FOGCOLOR",	//                  = 34,   /* D3DCOLOR */
	"D3DRS_FOGTABLEMODE",	//              = 35,   /* D3DFOGMODE */
	"D3DRS_FOGSTART",		//              = 36,   /* Fog start (for both vertex and pixel fog) */
	"D3DRS_FOGEND",		//                    = 37,   /* Fog end      */
	"D3DRS_FOGDENSITY",	//                = 38,   /* Fog density  */
	"39",
	"D3DRS_EDGEANTIALIAS",	//             = 40,   /* TRUE to enable edge antialiasing */
	"41","42","43","44","45","46",
	"D3DRS_ZBIAS",		//                     = 47,   /* LONG Z bias */
	"D3DRS_RANGEFOGENABLE",	//            = 48,   /* Enables range-based fog */
	"49","50","51",
	"D3DRS_STENCILENABLE",	//             = 52,   /* BOOL enable/disable stenciling */
	"D3DRS_STENCILFAIL",	//               = 53,   /* D3DSTENCILOP to do if stencil test fails */
	"D3DRS_STENCILZFAIL",	//              = 54,   /* D3DSTENCILOP to do if stencil test passes and Z test fails */
	"D3DRS_STENCILPASS",	//               = 55,   /* D3DSTENCILOP to do if both stencil and Z tests pass */
	"D3DRS_STENCILFUNC",	//               = 56,   /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
	"D3DRS_STENCILREF",	//                = 57,   /* Reference value used in stencil test */
	"D3DRS_STENCILMASK",	//               = 58,   /* Mask value used in stencil test */
	"D3DRS_STENCILWRITEMASK",	//          = 59,   /* Write mask applied to values written to stencil buffer */
	"D3DRS_TEXTUREFACTOR",	//             = 60,   /* D3DCOLOR used for multi-texture blend */
	       "61", "62", "63", "64", "65", "66", "67", "68", "69",
	 "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
	 "80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
 	 "90", "91", "92", "93", "94", "95", "96", "97", "98", "99",
	"100","101","102","103","104","105","106","107","108","109",
	"110","111","112","113","114","115","116","117","118","119",
	"120","121","122","123","124","125","126","127",
	"D3DRS_WRAP0",						//	= 128,  /* wrap for 1st texture coord. set */
	"D3DRS_WRAP1",						//	         = 129,  /* wrap for 2nd texture coord. set */
	"D3DRS_WRAP2",						//                     = 130,  /* wrap for 3rd texture coord. set */
	"D3DRS_WRAP3",						//                     = 131,  /* wrap for 4th texture coord. set */
	"D3DRS_WRAP4",						//                     = 132,  /* wrap for 5th texture coord. set */
	"D3DRS_WRAP5",						//                     = 133,  /* wrap for 6th texture coord. set */
	"D3DRS_WRAP6",						//	                     = 134,  /* wrap for 7th texture coord. set */
	"D3DRS_WRAP7",						//    = 135,  /* wrap for 8th texture coord. set */
	"D3DRS_CLIPPING",					//					    = 136,
	"D3DRS_LIGHTING",					//						= 137,
	"138",
	"D3DRS_AMBIENT",					//		              = 139,
	"D3DRS_FOGVERTEXMODE",				//             = 140,
	"D3DRS_COLORVERTEX",				//               = 141,
	"D3DRS_LOCALVIEWER",				//               = 142,
	"D3DRS_NORMALIZENORMALS",			//          = 143,
	"144",
	"D3DRS_DIFFUSEMATERIALSOURCE",		//     = 145,
	"D3DRS_SPECULARMATERIALSOURCE",		//    = 146,
	"D3DRS_AMBIENTMATERIALSOURCE",		//     = 147,
	"D3DRS_EMISSIVEMATERIALSOURCE",		//    = 148,
	"149","150",
	"D3DRS_VERTEXBLEND",				//               = 151,
	"D3DRS_CLIPPLANEENABLE",			//           = 152,
	"D3DRS_SOFTWAREVERTEXPROCESSING",	//  = 153,
	"D3DRS_POINTSIZE",					//                 = 154,   /* float point size */
	"D3DRS_POINTSIZE_MIN",				//             = 155,   /* float point size min threshold */
	"D3DRS_POINTSPRITEENABLE",			//         = 156,   /* BOOL point texture coord control */
	"D3DRS_POINTSCALEENABLE",			//          = 157,   /* BOOL point size scale enable */
	"D3DRS_POINTSCALE_A",				//              = 158,   /* float point attenuation A value */
	"D3DRS_POINTSCALE_B",				//              = 159,   /* float point attenuation B value */
	"D3DRS_POINTSCALE_C",				//              = 160,   /* float point attenuation C value */
	"D3DRS_MULTISAMPLEANTIALIAS",		//      = 161,  // BOOL - set to do FSAA with multisample buffer
	"D3DRS_MULTISAMPLEMASK",			//           = 162,  // DWORD - per-sample enable/disable
	"D3DRS_PATCHEDGESTYLE",				//            = 163,  // Sets whether patch edges will use float style tessellation
	"D3DRS_PATCHSEGMENTS",				//             = 164,  // Number of segments per edge when drawing patches
	"D3DRS_DEBUGMONITORTOKEN",			//         = 165,  // DEBUG ONLY - token to debug monitor
	"D3DRS_POINTSIZE_MAX",				//             = 166,   /* float point size max threshold */
	"D3DRS_INDEXEDVERTEXBLENDENABLE",	//  = 167,
	"D3DRS_COLORWRITEENABLE",			//          = 168,  // per-channel write enable
	"169",
	"D3DRS_TWEENFACTOR",				//               = 170,   // float tween factor
	"D3DRS_BLENDOP",					//                   = 171,   // D3DBLENDOP setting
};
char* g_tsnames[N_TEXTURESTAGESTATES] = {
	"0",
	"D3DTSS_COLOROP",					//        =  1, /* D3DTEXTUREOP - per-stage blending controls for color channels */
	"D3DTSS_COLORARG1",					//      =  2, /* D3DTA_* (texture arg) */
	"D3DTSS_COLORARG2",					//      =  3, /* D3DTA_* (texture arg) */
	"D3DTSS_ALPHAOP",					//        =  4, /* D3DTEXTUREOP - per-stage blending controls for alpha channel */
	"D3DTSS_ALPHAARG1",					//      =  5, /* D3DTA_* (texture arg) */
	"D3DTSS_ALPHAARG2",					//      =  6, /* D3DTA_* (texture arg) */
	"D3DTSS_BUMPENVMAT00",				//   =  7, /* float (bump mapping matrix) */
	"D3DTSS_BUMPENVMAT01",				//   =  8, /* float (bump mapping matrix) */
	"D3DTSS_BUMPENVMAT10",				//   =  9, /* float (bump mapping matrix) */
	"D3DTSS_BUMPENVMAT11",				//   = 10, /* float (bump mapping matrix) */
	"D3DTSS_TEXCOORDINDEX",				//  = 11, /* identifies which set of texture coordinates index this texture */
	"12",
	"D3DTSS_ADDRESSU",					//       = 13, /* D3DTEXTUREADDRESS for U coordinate */
	"D3DTSS_ADDRESSV",					//       = 14, /* D3DTEXTUREADDRESS for V coordinate */
	"D3DTSS_BORDERCOLOR",				//    = 15, /* D3DCOLOR */
	"D3DTSS_MAGFILTER",					//      = 16, /* D3DTEXTUREFILTER filter to use for magnification */
	"D3DTSS_MINFILTER",					//      = 17, /* D3DTEXTUREFILTER filter to use for minification */
	"D3DTSS_MIPFILTER",					//      = 18, /* D3DTEXTUREFILTER filter to use between mipmaps during minification */
	"D3DTSS_MIPMAPLODBIAS",				//  = 19, /* float Mipmap LOD bias */
	"D3DTSS_MAXMIPLEVEL",				//    = 20, /* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
	"D3DTSS_MAXANISOTROPY",				//  = 21, /* DWORD maximum anisotropy */
	"D3DTSS_BUMPENVLSCALE",				//  = 22, /* float scale for bump map luminance */
	"D3DTSS_BUMPENVLOFFSET",			// = 23, /* float offset for bump map luminance */
	"D3DTSS_TEXTURETRANSFORMFLAGS",		// = 24, /* D3DTEXTURETRANSFORMFLAGS controls texture transform */
	"D3DTSS_ADDRESSW",					//       = 25, /* D3DTEXTUREADDRESS for W coordinate */
	"D3DTSS_COLORARG0",					//      = 26, /* D3DTA_* third arg for triadic ops */
	"D3DTSS_ALPHAARG0",					//      = 27, /* D3DTA_* third arg for triadic ops */
	"D3DTSS_RESULTARG",					//      = 28, /* D3DTA_* arg for result (CURRENT or TEMP) */
};
void PCLTShell::CheckD3DState(char* tag, DWORD* rsexceptions, DWORD* tsexceptions)
{
	if(mD3DErrorCount>20) return;
	mD3DErrorCount++;
	
	// Ensure the state is up to date

	RENDERINFO.Apply(false);

	BOOL areerrors=FALSE;
	char rsx[N_RENDERSTATES];
	char tsx[N_TEXTURESTAGESTATES];
	int i;
	memset(rsx,0,sizeof(rsx));
	memset(tsx,0,sizeof(tsx));
	if(rsexceptions) for(;*rsexceptions; rsexceptions++) rsx[*rsexceptions]=1;
	if(tsexceptions) for(;*tsexceptions; tsexceptions++) tsx[*tsexceptions]=1;
	
	for(i=0; i<N_RENDERSTATES; i++)
	{
		if((rsx[i]==0)&&(mRenderStates2[i]!=UNDEFAULTED)&&(mRenderStates[i]!=mRenderStates2[i]))
		{
			if(!areerrors)
			{
				DBT.Out("%s: ",tag);
				areerrors =TRUE;
			}
			DBT.Out("%s=%d (%d) ",g_rsnames[i],mRenderStates[i],mRenderStates2[i]);
//			ASSERT(0);
		}
	}
	for(int s=0; s<4; s++)
	{
		for(i=0; i<N_TEXTURESTAGESTATES; i++)
		{
			if((tsx[i]==0)&&(mTextureStageStates2[s][i]!=UNDEFAULTED)&&(mTextureStageStates[s][i]!=mTextureStageStates2[s][i]))
			{
				if(!areerrors)
				{
					DBT.Out("%s: ",tag);
					areerrors =TRUE;
				}
				DBT.Out("[%d]%s=%d (%d) ", s, g_tsnames[i],mTextureStageStates[s][i],mTextureStageStates2[s][i]);
//				ASSERT(0);
			}
		}
	}
	if(areerrors) DBT.Out("\n");
}
#endif

// --------------------------------------------------------------------------

int	PCLTShell::GetBPP(D3DFORMAT fmt)
{
	if ((fmt==D3DFMT_A8R8G8B8) || (fmt==D3DFMT_X8R8G8B8)||(D3DFMT_Q8W8V8U8))
		return(32);
	if ((fmt==D3DFMT_A1R5G5B5) || (fmt==D3DFMT_X1R5G5B5) || (fmt==D3DFMT_A4R4G4B4) ||
		(fmt==D3DFMT_D16) || (fmt==D3DFMT_D16_LOCKABLE) || (fmt==D3DFMT_X4R4G4B4) ||
		(fmt==D3DFMT_R5G6B5)||(D3DFMT_V8U8))
		return(16);

	if ((fmt==D3DFMT_DXT1) || (fmt==D3DFMT_DXT2))
		return(16);

	ASSERT(0); // Unknown pixel format!

	return(32); // Guess!
}

// --------------------------------------------------------------------------

BOOL PCLTShell::IsThisAGeForce3()
{	
	D3DAdapterInfo* ai = &m_Adapters[m_dwAdapter];

	char *name=ai->d3dAdapterIdentifier.Description;

	CONSOLE.Print("Adaptor ID is '%s'...",name);

	char *p=name;
	while (*p!=0)
	{
		if ((strnicmp(p,"Geforce 3",9)==0) ||
		    (strnicmp(p,"Geforce3",8)==0) ||
			(strnicmp(p,"ASUS V8200",8)==0) )
		{
			CONSOLE.Print(" ...which is probably a GeForce 3.\n");
			return(TRUE);
		}
		p++;
	}

	CONSOLE.Print(" ...which is probably not a GeForce 3.\n");
	
	return(FALSE);	
}

// --------------------------------------------------------------------------

void PCLTShell::ShowGFXCardInfo()
{	
	D3DAdapterInfo* ai = &m_Adapters[m_dwAdapter];

	CONSOLE.Print("------------------\n");
	CONSOLE.Print("Graphics card info\n");
	CONSOLE.Print("------------------\n");
	CONSOLE.Print("Description    : %s\n",ai->d3dAdapterIdentifier.Description);
	CONSOLE.Print("Driver         : %s\n",ai->d3dAdapterIdentifier.Driver);
	CONSOLE.Print("Driver version : %d\n",ai->d3dAdapterIdentifier.DriverVersion);

	if (CLIPARAMS.mPureDevice)
		CONSOLE.Print("Using pure device\n");
	else
		CONSOLE.Print("Using impure device\n");

	CONSOLE.Print("\n");
}

// --------------------------------------------------------------------------

void PCLTShell::TriggerQuit()
{
	GAME.SetQuit(QT_QUIT_TO_FRONTEND);
	FRONTEND.SetQuit(-1);
#ifdef DEV_VERSION
	MODELVIEWER.SetQuit(-1);
	CUTSCENEEDITOR.SetQuit(-1);
#endif
	
}

// --------------------------------------------------------------------------

ETextureFormat	PCLTShell::GetDisplayFormat() 
{ 
	return CTEXTURE::FromD3DFormat(m_d3dsdBackBuffer.Format);
}; 

HRESULT PCLTShell::D3D_CreateVertexBuffer( UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
		IDirect3DVertexBuffer8** ppVertexBuffer, void **pData)
{
	SANITY_CHECK_AND_RETURN(m_pd3dDevice->CreateVertexBuffer( Length, Usage, FVF, Pool, ppVertexBuffer));
}

int PCLTShell::D3D_ReleaseVertexBuffer(IDirect3DVertexBuffer8 * pVertexBuffer, void *pData)
{
	SAFE_RELEASE(pVertexBuffer);
	return 0;
}

#endif