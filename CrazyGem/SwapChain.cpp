#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"

extern SceneState scene_state;

extern CDXUTTextHelper*                    g_pTxtHelper;
extern CDXUTDialogResourceManager          g_DialogResourceManager;

SwapChainGraphicResources * SCG;
//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* device, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* backBufferSurfaceDesc, void* pUserContext)
{
	float ratio = backBufferSurfaceDesc->Width / (FLOAT)backBufferSurfaceDesc->Height;

	float np = 0.1, fp = 500, fov = D3DX_PI / 3;

	////Some tests
	{
		SimpleMath::Matrix prm(DirectX::XMMatrixPerspectiveFovLH(fov, ratio, np, fp));

		auto t1 = SimpleMath::Vector4::Transform(SimpleMath::Vector4(0, 0, 0.05, 1), prm);
		t1 /= t1.w;

		auto t2 = SimpleMath::Vector4::Transform(SimpleMath::Vector4(0, 0, -1.99, 1), prm);
		t2 /= t2.w;

		auto t3 = SimpleMath::Vector4::Transform(SimpleMath::Vector4(0, 0, -0.05, 1), prm);
	}
	////
	HRESULT hr;

	g_DialogResourceManager.OnD3D11ResizedSwapChain(device, backBufferSurfaceDesc);

	DirectX::XMStoreFloat4x4(&scene_state.mProjection, DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(fov, ratio, np, fp)));

	scene_state.vFrustumParams = SimpleMath::Vector4(backBufferSurfaceDesc->Width, backBufferSurfaceDesc->Height, scene_state.mProjection._22, ratio);

	scene_state.vFrustumNearFar = SimpleMath::Vector4(np, fp, 0, 0);

	SCG = new SwapChainGraphicResources();

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = 1024;// backBufferSurfaceDesc->Width;
	textureDesc.Height = 1024;// backBufferSurfaceDesc->Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;////DXGI_FORMAT_R32G8X24_TYPELESS;// DXGI_FORMAT_R24G8_TYPELESS;// ;// ;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	dsv_desc.Flags = 0;
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT; //DXGI_FORMAT_D32_FLOAT_S8X24_UINT;// DXGI_FORMAT_D24_UNORM_S8_UINT;// ;//;
	dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
	sr_desc.Format = DXGI_FORMAT_R32_FLOAT;////DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;// DXGI_FORMAT_R24_UNORM_X8_TYPELESS;// ;// ;
	sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr_desc.Texture2D.MostDetailedMip = 0;
	sr_desc.Texture2D.MipLevels = 1; //-1

	hr = device->CreateTexture2D(&textureDesc, 0, SCG->depthStencil1T.ReleaseAndGetAddressOf());

	hr = device->CreateDepthStencilView(SCG->depthStencil1T.Get(), &dsv_desc, SCG->depthStencil1V.ReleaseAndGetAddressOf());

	hr = device->CreateShaderResourceView(SCG->depthStencil1T.Get(), &sr_desc, SCG->depthStencil1SRV.ReleaseAndGetAddressOf());

	hr = device->CreateTexture2D(&textureDesc, 0, SCG->depthStencil2T.ReleaseAndGetAddressOf());

	hr = device->CreateDepthStencilView(SCG->depthStencil2T.Get(), &dsv_desc, SCG->depthStencil2V.ReleaseAndGetAddressOf());

	hr = device->CreateShaderResourceView(SCG->depthStencil2T.Get(), &sr_desc, SCG->depthStencil2SRV.ReleaseAndGetAddressOf());

	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	//textureDesc.Width = 1024;
	//textureDesc.Height = 1024;

	hr = device->CreateTexture2D(&textureDesc, nullptr, SCG->depthT.ReleaseAndGetAddressOf());

	hr = device->CreateRenderTargetView(SCG->depthT.Get(), nullptr, SCG->depthV.ReleaseAndGetAddressOf());

	hr = device->CreateShaderResourceView(SCG->depthT.Get(), nullptr, SCG->depthSRV.ReleaseAndGetAddressOf());

	hr = device->CreateTexture2D(&textureDesc, nullptr, SCG->colorT.ReleaseAndGetAddressOf());

	hr = device->CreateRenderTargetView(SCG->colorT.Get(), nullptr, SCG->colorV.ReleaseAndGetAddressOf());

	hr = device->CreateShaderResourceView(SCG->colorT.Get(), nullptr, SCG->colorSRV.ReleaseAndGetAddressOf());

	hr = device->CreateTexture2D(&textureDesc, nullptr, SCG->colorSecondT.ReleaseAndGetAddressOf());

	hr = device->CreateRenderTargetView(SCG->colorSecondT.Get(), nullptr, SCG->colorSecondV.ReleaseAndGetAddressOf());

	hr = device->CreateShaderResourceView(SCG->colorSecondT.Get(), nullptr, SCG->colorSecondSRV.ReleaseAndGetAddressOf());

	return S_OK;
}
//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();

	delete SCG;
}