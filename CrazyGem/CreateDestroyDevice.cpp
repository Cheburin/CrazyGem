#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"

HWND DXUTgetWindow();

GraphicResources * G;

SceneState scene_state;

BlurHandling blur_handling;

BlurParams blurParams;

std::unique_ptr<Keyboard> _keyboard;
std::unique_ptr<Mouse> _mouse;

CDXUTDialogResourceManager          g_DialogResourceManager;
CDXUTTextHelper*                    g_pTxtHelper = NULL;

#include <codecvt>
std::unique_ptr<SceneNode> loadSponza(ID3D11Device* device, ID3D11InputLayout** l, DirectX::IEffect *e);

inline float lerp(float x1, float x2, float t){
	return x1*(1.0 - t) + x2*t;
}

inline float nextFloat(float x1, float x2){
	return lerp(x1, x2, (float)std::rand() / (float)RAND_MAX);
}

BlurParams GaussianBlur(int radius)
{
	BlurParams p(radius);

	float twoRadiusSquaredRecip = 1.0f / (2.0f * radius * radius);
	float sqrtTwoPiTimesRadiusRecip = 1.0f / ((float)sqrt(2.0f * D3DX_PI) * radius);
	float radiusModifier = 1.0f;

	float WeightsSum = 0;
	for (int i = 0; i < p.WeightLength; i++)
	{
		double x = (-radius + i) * radiusModifier;
		x *= x;
		p.Weights[i] = sqrtTwoPiTimesRadiusRecip * (float)exp(-x * sqrtTwoPiTimesRadiusRecip);
		WeightsSum += p.Weights[i];
	}

	/* NORMALIZE */
	float div = WeightsSum;
	for (int i = 0; i < p.WeightLength; i++)
		p.Weights[i] /= div;

	return p;
}


DirectX::XMFLOAT4X4    lightViewProjection;
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* device, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	DirectX::XMStoreFloat4x4(&lightViewProjection, DirectX::XMMatrixPerspectiveFovLH(D3DX_PI / 2, 1, 0.1, 500));

	const float SHADOWMAP_DEPTH = 500;
	const float SHADOWMAP_NEAR = 1.0f;

	DirectX::XMStoreFloat4x4(&lightViewProjection, DirectX::XMMatrixOrthographicLH(15, 15, SHADOWMAP_NEAR, SHADOWMAP_DEPTH)); //(20.0f, SHADOWMAP_DEPTH, SHADOWMAP_NEAR);


	std::srand(unsigned(std::time(0)));

	HRESULT hr;

	ID3D11DeviceContext* context = DXUTGetD3D11DeviceContext();

	G = new GraphicResources();
	G->render_states = std::make_unique<CommonStates>(device);
	G->scene_constant_buffer = std::make_unique<ConstantBuffer<SceneState> >(device);
	G->blur_constant_buffer = std::make_unique<ConstantBuffer<BlurHandling> >(device);

	_keyboard = std::make_unique<Keyboard>();
	_mouse = std::make_unique<Mouse>();
	HWND hwnd = DXUTgetWindow();
	_mouse->SetWindow(hwnd);

	g_DialogResourceManager.OnD3D11CreateDevice(device, context);
	g_pTxtHelper = new CDXUTTextHelper(device, context, &g_DialogResourceManager, 15);

	//effects
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL_VERTEX", L"vs_5_0" };
		shaderDef[L"PS"] = { L"orthoBox.hlsl", L"PS", L"ps_5_0" };

		G->model_wite_wireframe_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL_VERTEX", L"vs_5_0" };
		shaderDef[L"PS"] = { L"orthoBox.hlsl", L"PS_GREEN", L"ps_5_0" };

		G->model_green_wireframe_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"QuadVS.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"QuadPS.hlsl", L"PS", L"ps_5_0" };

		G->quad_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL_VERTEX1", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"MODEL_FRAG", L"ps_5_0" };

		G->model_effect1 = createHlslEffect(device, shaderDef);
	}

	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL_VERTEX", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"MODEL_PRE_SHADOW", L"ps_5_0" };

		G->model_effect2 = createHlslEffect(device, shaderDef);
	}

	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL_VERTEX", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"MODEL_FRAG_SHADOW", L"ps_5_0" };

		G->model_shadow_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"blur.hlsl", L"HB", L"ps_5_0" };

		G->blur_h_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"blur.hlsl", L"VB", L"ps_5_0" };

		G->blur_v_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL_VERTEX", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"MODEL_FRAG_WITH_TEXTURE", L"ps_5_0" };

		G->model_with_texture = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"GS"] = { L"orthoBox.hlsl", L"GS", L"gs_5_0" };
		shaderDef[L"VS"] = { L"orthoBox.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"orthoBox.hlsl", L"PS", L"ps_5_0" };

		G->ortho_box_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"GS"] = { L"orthoBox.hlsl", L"GS_LINE", L"gs_5_0" };
		shaderDef[L"VS"] = { L"orthoBox.hlsl", L"VS_LINE", L"vs_5_0" };
		shaderDef[L"PS"] = { L"orthoBox.hlsl", L"PS_LINE", L"ps_5_0" };

		G->red_line_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"GS"] = { L"voxelizer.hlsl", L"GS_GENVELOCITY", L"gs_4_0" };
		shaderDef[L"VS"] = { L"voxelizer.hlsl", L"VS_GENVELOCITY", L"vs_4_0" };
		shaderDef[L"PS"] = { L"voxelizer.hlsl", L"PS_GENVELOCITY", L"ps_4_0" };

		G->test_effect = createHlslEffect(device, shaderDef);
	}
	/*
	{
	std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
	shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
	shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
	shaderDef[L"PS"] = { L"blur.hlsl", L"HB", L"ps_5_0" };

	G->blur_h_effect = createHlslEffect(device, shaderDef);
	}
	{
	std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
	shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
	shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
	shaderDef[L"PS"] = { L"blur.hlsl", L"VB", L"ps_5_0" };

	G->blur_v_effect = createHlslEffect(device, shaderDef);
	}
	*/
	//models
	{
		G->cube_model = CreateModelMeshPart(device, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
			LoadModel("models\\cube.txt", _vertices, _indices);
		});
		G->plane_model = CreateModelMeshPart(device, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
			LoadModel("models\\plane01.txt", _vertices, _indices);
		});
		G->sphere_model = CreateModelMeshPart(device, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
			LoadModel("models\\sphere.txt", _vertices, _indices);
		});

		G->trees_leaf_model = CreateModelMeshPart(device, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
			LoadModel("models\\trees\\leaf001.txt", _vertices, _indices);
		});
		G->trees_trunk_model = CreateModelMeshPart(device, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
			LoadModel("models\\trees\\trunk001.txt", _vertices, _indices);
		});

		G->plane_model->CreateInputLayout(device, G->model_effect1.get(), G->model_input_layout.ReleaseAndGetAddressOf());

		hr = D3DX11CreateShaderResourceViewFromFile(device, L"models\\ice.dds", NULL, NULL, G->ice_texture.ReleaseAndGetAddressOf(), NULL);
		hr = D3DX11CreateShaderResourceViewFromFile(device, L"models\\dirt.dds", NULL, NULL, G->metal_texture.ReleaseAndGetAddressOf(), NULL);
		hr = D3DX11CreateShaderResourceViewFromFile(device, L"models\\seafloor.dds", NULL, NULL, G->wall_texture.ReleaseAndGetAddressOf(), NULL);
		hr = D3DX11CreateShaderResourceViewFromFile(device, L"models\\grate.dds", NULL, NULL, G->projected_texture.ReleaseAndGetAddressOf(), NULL);

		hr = D3DX11CreateShaderResourceViewFromFile(device, L"models\\trees\\trunk001.dds", NULL, NULL, G->trunk_texture.ReleaseAndGetAddressOf(), NULL);
		hr = D3DX11CreateShaderResourceViewFromFile(device, L"models\\trees\\leaf001.dds", NULL, NULL, G->leaf_texture.ReleaseAndGetAddressOf(), NULL);

		//CreateSinglePointBuffer(G->single_point_buffer.ReleaseAndGetAddressOf(), device, G->quad_effect.get(), G->single_point_layout.ReleaseAndGetAddressOf());
		G->quad_mesh = CreateQuadModelMeshPart(device);
		G->quad_mesh->CreateInputLayout(device, G->quad_effect.get(), G->quad_mesh_layout.ReleaseAndGetAddressOf());
	}

	{
		G->cylinder_model = GeometricPrimitive::CreateCylinder(context, 1, 2, 64U, false);
		G->cylinder_model->CreateInputLayout(G->model_wite_wireframe_effect.get(), G->cylinder_model_input_layout.ReleaseAndGetAddressOf());

		G->sphere_model2 = GeometricPrimitive::CreateSphere(context, 0.5, 32, false, false);
	}
	//blur params
	{
		//blur paarams
		{
			blurParams = GaussianBlur(4);

			for (int i = 0; i < blurParams.WeightLength; i++)
				blur_handling.Weights[i] = SimpleMath::Vector4(blurParams.Weights[i], 0, 0, 0);

			blur_handling.Radius = 4;
			blur_handling.DepthAnalysis = 1;
			blur_handling.NormalAnalysis = 1;
			blur_handling.DepthAnalysisFactor = 1;

			G->blur_constant_buffer->SetData(context, blur_handling);
		}
	}

	D3D11_BLEND_DESC BSDesc;
	ZeroMemory(&BSDesc, sizeof(D3D11_BLEND_DESC));
	BSDesc.AlphaToCoverageEnable = true;
	BSDesc.IndependentBlendEnable = false;
	BSDesc.RenderTarget[0].BlendEnable = TRUE;
	BSDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BSDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BSDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	BSDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BSDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BSDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;

	BSDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = (device->CreateBlendState(&BSDesc, G->m_pBlendState.ReleaseAndGetAddressOf()));

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	delete g_pTxtHelper;

	g_DialogResourceManager.OnD3D11DestroyDevice();

	_mouse = 0;

	_keyboard = 0;

	delete G;
}
