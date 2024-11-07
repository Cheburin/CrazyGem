#include "Effects.h"

#include "DXUT.h"
#include "SDKmisc.h"

#include <wrl.h>

#include <map>
#include <algorithm>
#include <codecvt>

HRESULT CreateShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11DeviceChild** ppShader, ID3DBlob** ppShaderBlob = NULL,
	BOOL bDumpShader = FALSE);
HRESULT CreateVertexShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11VertexShader** ppShader, ID3DBlob** ppShaderBlob = NULL,
	BOOL bDumpShader = FALSE);
HRESULT CreateHullShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11HullShader** ppShader, ID3DBlob** ppShaderBlob = NULL,
	BOOL bDumpShader = FALSE);
HRESULT CreateDomainShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11DomainShader** ppShader, ID3DBlob** ppShaderBlob = NULL,
	BOOL bDumpShader = FALSE);
HRESULT CreateGeometryShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11GeometryShader** ppShader, ID3DBlob** ppShaderBlob = NULL,
	BOOL bDumpShader = FALSE);
HRESULT CreatePixelShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11PixelShader** ppShader, ID3DBlob** ppShaderBlob = NULL,
	BOOL bDumpShader = FALSE);
HRESULT CreateComputeShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11ComputeShader** ppShader, ID3DBlob** ppShaderBlob = NULL,
	BOOL bDumpShader = FALSE);


//--------------------------------------------------------------------------------------
// Helper function to create a shader from the specified filename
// This function is called by the shader-specific versions of this
// function located after the body of this function.
//--------------------------------------------------------------------------------------
HRESULT CreateShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11DeviceChild** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader)
{
	HRESULT   hr = D3D_OK;
	ID3DBlob* pShaderBlob = NULL;
	ID3DBlob* pErrorBlob = NULL;
	WCHAR     wcFullPath[256];

	DXUTFindDXSDKMediaFileCch(wcFullPath, 256, pSrcFile);
	// Compile shader into binary blob
	hr = D3DX11CompileFromFile(wcFullPath, pDefines, pInclude, pFunctionName, pProfile,
		Flags1, Flags2, pPump, &pShaderBlob, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		SAFE_RELEASE(pErrorBlob);
		return hr;
	}

	// Create shader from binary blob
	if (ppShader)
	{
		hr = E_FAIL;
		if (strstr(pProfile, "vs"))
		{
			hr = pd3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(),
				pShaderBlob->GetBufferSize(), NULL, (ID3D11VertexShader**)ppShader);
		}
		else if (strstr(pProfile, "hs"))
		{
			hr = pd3dDevice->CreateHullShader(pShaderBlob->GetBufferPointer(),
				pShaderBlob->GetBufferSize(), NULL, (ID3D11HullShader**)ppShader);
		}
		else if (strstr(pProfile, "ds"))
		{
			hr = pd3dDevice->CreateDomainShader(pShaderBlob->GetBufferPointer(),
				pShaderBlob->GetBufferSize(), NULL, (ID3D11DomainShader**)ppShader);
		}
		else if (strstr(pProfile, "gs"))
		{
			hr = pd3dDevice->CreateGeometryShader(pShaderBlob->GetBufferPointer(),
				pShaderBlob->GetBufferSize(), NULL, (ID3D11GeometryShader**)ppShader);
		}
		else if (strstr(pProfile, "ps"))
		{
			hr = pd3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(),
				pShaderBlob->GetBufferSize(), NULL, (ID3D11PixelShader**)ppShader);
		}
		else if (strstr(pProfile, "cs"))
		{
			hr = pd3dDevice->CreateComputeShader(pShaderBlob->GetBufferPointer(),
				pShaderBlob->GetBufferSize(), NULL, (ID3D11ComputeShader**)ppShader);
		}
		if (FAILED(hr))
		{
			OutputDebugString(L"Shader creation failed\n");
			SAFE_RELEASE(pErrorBlob);
			SAFE_RELEASE(pShaderBlob);
			return hr;
		}
	}

	DXUT_SetDebugName(*ppShader, pFunctionName);

	// If blob was requested then pass it otherwise release it
	if (ppShaderBlob)
	{
		*ppShaderBlob = pShaderBlob;
	}
	else
	{
		pShaderBlob->Release();
	}

	// Return error code
	return hr;
}


//--------------------------------------------------------------------------------------
// Create a vertex shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateVertexShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11VertexShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader)
{
	return CreateShaderFromFile(pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile,
		Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob,
		bDumpShader);
}


//--------------------------------------------------------------------------------------
// Create a hull shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateHullShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11HullShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader)
{
	return CreateShaderFromFile(pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile,
		Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob,
		bDumpShader);
}
//--------------------------------------------------------------------------------------
// Create a domain shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateDomainShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11DomainShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader)
{
	return CreateShaderFromFile(pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile,
		Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob,
		bDumpShader);
}


//--------------------------------------------------------------------------------------
// Create a geometry shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateGeometryShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11GeometryShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader)
{
	return CreateShaderFromFile(pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile,
		Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob,
		bDumpShader);
}


//--------------------------------------------------------------------------------------
// Create a pixel shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreatePixelShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11PixelShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader)
{
	return CreateShaderFromFile(pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile,
		Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob,
		bDumpShader);
}


//--------------------------------------------------------------------------------------
// Create a compute shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateComputeShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11ComputeShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader)
{
	return CreateShaderFromFile(pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile,
		Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob,
		bDumpShader);
}


struct EffectShaderFileDef{
	WCHAR * name;
	WCHAR * entry_point;
	WCHAR * profile;
};

class HlslEffect : public DirectX::IEffect
{
protected:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;

	Microsoft::WRL::ComPtr<ID3DBlob>         blob_vs;

	Microsoft::WRL::ComPtr<ID3D11HullShader> hs;

	Microsoft::WRL::ComPtr<ID3D11DomainShader> ds;

	Microsoft::WRL::ComPtr<ID3D11GeometryShader> gs;

	Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;

public:
	//Constructor
	HlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef){
		std::map<const WCHAR*, ID3D11DeviceChild*> shaders;

		std::for_each(fileDef.begin(), fileDef.end(), [this, device, &shaders](std::pair<const WCHAR*, EffectShaderFileDef> p) {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			ID3D11DeviceChild* pShader;

			if (FAILED(CreateShaderFromFile(
				device,
				p.second.name,
				NULL,
				NULL,
				converter.to_bytes(p.second.entry_point).data(),
				converter.to_bytes(p.second.profile).data(),
				D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS, //D3DCOMPILE_PREFER_FLOW_CONTROL
				0,
				NULL,
				&pShader,
				(std::wstring(p.first) == std::wstring(L"VS") ? this->blob_vs.ReleaseAndGetAddressOf() : NULL),
				false
				)))
				throw std::exception("HlslEffect");

			shaders.insert(std::pair<const WCHAR*, ID3D11DeviceChild*>(p.first, pShader));
		});

		vs.Attach((ID3D11VertexShader*)shaders[L"VS"]);
		hs.Attach((ID3D11HullShader*)shaders[L"HS"]);
		ds.Attach((ID3D11DomainShader*)shaders[L"DS"]);
		gs.Attach((ID3D11GeometryShader*)shaders[L"GS"]);
		ps.Attach((ID3D11PixelShader*)shaders[L"PS"]);
	}

	//Destructor
	virtual ~HlslEffect(){

	}

	//IEffect
	virtual void __cdecl Apply(_In_ ID3D11DeviceContext* context) {
		context->VSSetShader(vs.Get(), NULL, 0);
		context->HSSetShader(hs.Get(), NULL, 0);
		context->DSSetShader(ds.Get(), NULL, 0);
		context->GSSetShader(gs.Get(), NULL, 0);
		context->PSSetShader(ps.Get(), NULL, 0);
	}

	virtual void __cdecl GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) {
		*pShaderByteCode = blob_vs->GetBufferPointer();
		*pByteCodeLength = blob_vs->GetBufferSize();
	}
};

std::unique_ptr<DirectX::IEffect> createHlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef){
	return std::unique_ptr<DirectX::IEffect>(new HlslEffect(device, fileDef));
}

