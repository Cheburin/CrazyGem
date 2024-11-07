#include "GeometricPrimitive.h"
#include "Effects.h"
#include "DirectXHelpers.h"
#include "Model.h"
#include "CommonStates.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "SimpleMath.h"

#include "DirectXMath.h"

#include "DXUT.h"

#include <wrl.h>

#include <map>
#include <algorithm>
#include <array>
#include <memory>
#include <assert.h>
#include <malloc.h>
#include <Exception>

#include "ConstantBuffer.h"

#include "AppBuffers.h"

using namespace DirectX;

struct EffectShaderFileDef{
	WCHAR * name;
	WCHAR * entry_point;
	WCHAR * shader_ver;
};
class IPostProcess
{
public:
	virtual ~IPostProcess() { }

	virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) = 0;
};

inline ID3D11RenderTargetView** renderTargetViewToArray(ID3D11RenderTargetView* rtv1, ID3D11RenderTargetView* rtv2 = 0, ID3D11RenderTargetView* rtv3 = 0){
	static ID3D11RenderTargetView* rtvs[10];
	rtvs[0] = rtv1;
	rtvs[1] = rtv2;
	rtvs[2] = rtv3;
	return rtvs;
};
inline ID3D11ShaderResourceView** shaderResourceViewToArray(ID3D11ShaderResourceView* rtv1, ID3D11ShaderResourceView* rtv2 = 0, ID3D11ShaderResourceView* rtv3 = 0, ID3D11ShaderResourceView* rtv4 = 0, ID3D11ShaderResourceView* rtv5 = 0){
	static ID3D11ShaderResourceView* srvs[10];
	srvs[0] = rtv1;
	srvs[1] = rtv2;
	srvs[2] = rtv3;
	srvs[3] = rtv4;
	srvs[4] = rtv5;
	return srvs;
};
inline ID3D11Buffer** constantBuffersToArray(ID3D11Buffer* c1, ID3D11Buffer* c2){
	static ID3D11Buffer* cbs[10];
	cbs[0] = c1;
	cbs[1] = c2;
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<SceneState> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<BlurHandling> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<SceneState> &c1, DirectX::ConstantBuffer<BlurHandling> &c2){
	static ID3D11Buffer* cbs[10];
	cbs[0] = c1.GetBuffer();
	cbs[1] = c2.GetBuffer();
	return cbs;
};
inline ID3D11SamplerState** samplerStateToArray(ID3D11SamplerState* ss1, ID3D11SamplerState* ss2 = 0){
	static ID3D11SamplerState* sss[10];
	sss[0] = ss1;
	sss[1] = ss2;
	return sss;
};

namespace Camera{
	void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
}
std::unique_ptr<DirectX::IEffect> createHlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef);

bool LoadModel(char* filename, std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices);
std::unique_ptr<DirectX::ModelMeshPart> CreateModelMeshPart(ID3D11Device* device, std::function<void(std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices)> createGeometry);
void CreateSinglePointBuffer(ID3D11Buffer ** vertexBuffer, ID3D11Device* device, DirectX::IEffect * effect, ID3D11InputLayout ** layout);
std::unique_ptr<DirectX::ModelMeshPart> CreateQuadModelMeshPart(ID3D11Device* device);

void set_scene_world_matrix(DirectX::XMFLOAT4X4 transformation);
void scene_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState = nullptr);

void post_proccess(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState);

void sphere_set_world_matrix();
void sphere_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr);

void plane_set_world_matrix();
void plane_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr);

void cube_set_world_matrix();
void cube_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr);

void tree_set_world_matrix();
void tree_trunk_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState);
void tree_leaf_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState);

void ortho_box_set_world_matrix();
void ortho_box_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState);

struct SceneNode{
	DirectX::XMFLOAT4X4 transformation;
	std::vector<DirectX::ModelMeshPart*> mesh;
	std::vector<SceneNode*> children;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> > texture;
	~SceneNode();
	void draw(_In_ ID3D11DeviceContext* deviceContext, _In_ IEffect* ieffect, _In_ ID3D11InputLayout* iinputLayout,
		_In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState);
};

struct BlurParams
{
	int Radius;
	int WeightLength;
	float Weights[64 * 2 + 1];

	BlurParams(int radius = 0)
	{
		WeightLength = (Radius = radius) * 2 + 1;
	}
};
BlurParams GaussianBlur(int radius);

class GraphicResources {
public:
	std::unique_ptr<IEffect> test_effect;

	std::unique_ptr<CommonStates> render_states;

	std::unique_ptr<DirectX::IEffect> quad_effect;

	std::unique_ptr<DirectX::IEffect> model_effect1;

	std::unique_ptr<DirectX::IEffect> model_effect2;

	std::unique_ptr<DirectX::IEffect> model_shadow_effect;

	std::unique_ptr<DirectX::IEffect> blur_h_effect;

	std::unique_ptr<DirectX::IEffect> blur_v_effect;

	std::unique_ptr<DirectX::IEffect> model_with_texture;

	std::unique_ptr<DirectX::IEffect> ortho_box_effect;

	std::unique_ptr<DirectX::ConstantBuffer<SceneState> > scene_constant_buffer;

	std::unique_ptr<DirectX::ConstantBuffer<BlurHandling> > blur_constant_buffer;

	std::unique_ptr<DirectX::ModelMeshPart> cube_model;

	std::unique_ptr<DirectX::ModelMeshPart> plane_model;

	std::unique_ptr<DirectX::ModelMeshPart> sphere_model;

	std::unique_ptr<DirectX::ModelMeshPart> trees_leaf_model;

	std::unique_ptr<DirectX::ModelMeshPart> trees_trunk_model;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> model_input_layout;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ice_texture;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal_texture;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wall_texture;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> projected_texture;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> trunk_texture;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> leaf_texture;

	std::unique_ptr<DirectX::ModelMeshPart> quad_mesh;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> quad_mesh_layout;

	//Microsoft::WRL::ComPtr<ID3D11Buffer> single_point_buffer;

	//Microsoft::WRL::ComPtr<ID3D11InputLayout> single_point_layout;

	Microsoft::WRL::ComPtr<ID3D11BlendState> m_pBlendState;

	std::unique_ptr<GeometricPrimitive> cylinder_model;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> cylinder_model_input_layout;

	std::unique_ptr<DirectX::IEffect> model_wite_wireframe_effect;

	std::unique_ptr<DirectX::IEffect> red_line_effect;

	std::unique_ptr<GeometricPrimitive> sphere_model2;

	std::unique_ptr<DirectX::IEffect> model_green_wireframe_effect;
};

class SwapChainGraphicResources {
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthStencil1SRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencil1V;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil1T;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthStencil2SRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencil2V;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil2T;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> colorSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> colorV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> colorT;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> colorSecondSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> colorSecondV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> colorSecondT;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> depthV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthT;
};