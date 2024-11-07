#pragma once
struct SceneState
{
	DirectX::XMFLOAT4X4    mWorld;                         // World matrix
	DirectX::XMFLOAT4X4    mView;                          // View matrix
	DirectX::XMFLOAT4X4    mProjection;                    // Projection matrix
	DirectX::XMFLOAT4X4    mWorldViewProjection;           // WVP matrix
	DirectX::XMFLOAT4X4    mWorldView;                     // WV matrix
	DirectX::XMFLOAT4X4    mInvView;                       // Inverse of view matrix

	DirectX::XMFLOAT4X4    mObject1;                // VP matrix
	DirectX::XMFLOAT4X4    mObject1WorldView;                       // Inverse of view matrix
	DirectX::XMFLOAT4X4    mObject1WorldViewProjection;                       // Inverse of view matrix

	DirectX::XMFLOAT4X4    mObject2;                // VP matrix
	DirectX::XMFLOAT4X4    mObject2WorldView;                       // Inverse of view matrix
	DirectX::XMFLOAT4X4    mObject2WorldViewProjection;                       // Inverse of view matrix

	DirectX::XMFLOAT4      vFrustumNearFar;              // Screen resolution
	DirectX::XMFLOAT4      vFrustumParams;              // Screen resolution
	DirectX::XMFLOAT4      viewLightPos;                   //

	DirectX::XMFLOAT4	   vPoint;
	DirectX::XMFLOAT4      vDirection;
	DirectX::XMFLOAT4      vColor;
};

#pragma pack (push, 1)
struct BlurHandling{
	int Radius;
	int reserv0;
	int reserv1;
	int reserv2;

	DirectX::XMFLOAT4 Weights[64 * 2 + 1];

	float DepthAnalysisFactor;
	int DepthAnalysis;
	int NormalAnalysis;
	int reserv5;
};
#pragma pack (pop)

#pragma pack (push, 1)
struct LightData
{
	DirectX::XMFLOAT3 position;
	unsigned int lightType;
	DirectX::XMFLOAT3 direction;
	float falloff;
	DirectX::XMFLOAT3 diffuseColor;
	float angle;
	DirectX::XMFLOAT3 ambientColor;
	unsigned int : 32;
	DirectX::XMFLOAT3 specularColor;
	unsigned int : 32;
};
#pragma pack (pop)

