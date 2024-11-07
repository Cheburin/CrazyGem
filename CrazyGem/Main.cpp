//--------------------------------------------------------------------------------------
// File: DetailTessellation11.cpp
//
// This sample demonstrates the use of detail tessellation for improving the 
// quality of material surfaces in real-time rendering applications.
//
// Developed by AMD Developer Relations team.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "main.h"

//--------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------
#define DWORD_POSITIVE_RANDOM(x)    ((DWORD)(( ((x)*rand()) / RAND_MAX )))
#define FLOAT_POSITIVE_RANDOM(x)    ( ((x)*rand()) / RAND_MAX )
#define FLOAT_RANDOM(x)             ((((2.0f*rand())/RAND_MAX) - 1.0f)*(x))

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
//void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, 
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, 
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, 
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, 
                                  double fTime, float fElapsedTime, void* pUserContext );

void InitApp();



//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D10 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    DXUTSetCallbackKeyboard( OnKeyboard );

    InitApp();
    DXUTInit( true, true );
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"DX11BaseApp" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1024, 768);
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}

//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            //DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }

        // Enable 4xMSAA by default
        DXGI_SAMPLE_DESC MSAA4xSampleDesc = { 1, 0 };
        pDeviceSettings->d3d11.sd.SampleDesc = MSAA4xSampleDesc;
    }

    return true;
}