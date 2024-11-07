cbuffer cbMain : register( b0 )
{
	matrix    g_mWorld;                         // World matrix
	matrix    g_mView;                          // View matrix
	matrix    g_mProjection;                    // Projection matrix
	matrix    g_mWorldViewProjection;           // WVP matrix
	matrix    g_mWorldView;                     // WV matrix
	matrix    g_mInvView;                       // Inverse of view matrix

	matrix    g_mObject1;                // VP matrix
	matrix    g_mObject1WorldView;                       // Inverse of view matrix
	matrix    g_mObject1WorldViewProjection;                       // Inverse of view matrix

	matrix    g_mObject2;                // VP matrix
	matrix    g_mObject2WorldView;                       // Inverse of view matrix
	matrix    g_mObject2WorldViewProjection;                       // Inverse of view matrix

	float4    g_vFrustumNearFar;              // Screen resolution
	float4    g_vFrustumParams;              // Screen resolution
	float4    g_viewLightPos;                   //
};

struct ExpandPos
{
    float4 pos      : SV_POSITION;
};

float4 VS(uint VertexID : SV_VERTEXID):SV_POSITION
{
  return float4( 0, 0, 0, 1.0 );
} 

ExpandPos _p(float x, float y){
  ExpandPos p;
  p.pos = float4(x, y, 0.5, 1); 
  return p;
}

[maxvertexcount(4)]
void GS(point ExpandPos pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<ExpandPos> triStream )
{
	triStream.Append(_p(-1, -1));
	triStream.Append(_p(-1,  1));
	triStream.Append(_p( 1, -1));
	triStream.Append(_p( 1,  1));
    triStream.RestartStrip();
}

Texture2D<float4> colorMap                      : register( t0 );

cbuffer Handling : register(b1)
{
	int Radius;
	int reserv0;
	int reserv1;
	int reserv2;

	float4 Weights[129];

	float DepthAnalysisFactor;
	int DepthAnalysis;
	int NormalAnalysis;
	int reserv5;
};

float4 GetColor(float2 uv)
{
	return colorMap.Load( int3(uv.xy * g_vFrustumParams.xy, 0) );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 Blur(float2 UV, float2 SourcePixel, const float2 dir)
{
//
	float4 finalColor = GetColor(UV) * Weights[Radius].x;

	float lDepthC = 0;

	float totalAdditionalWeight = Weights[Radius].x;
	
	for(int i = 1; i <= Radius; i++)
	{
		float2 UVL = UV - SourcePixel * dir * i;
		float2 UVR = UV + SourcePixel * dir * i;

		float depthFactorR = 1.0f;
		float depthFactorL = 1.0f;
		float normalFactorL = 1.0f;
		float normalFactorR = 1.0f;
		
		float cwR = Weights[Radius + i].x * depthFactorR * normalFactorR;
		float cwL = Weights[Radius - i].x * depthFactorL * normalFactorL;

		finalColor += GetColor(UVR) * cwR;
		finalColor += GetColor(UVL) * cwL;

		totalAdditionalWeight += cwR;
		totalAdditionalWeight += cwL;
	}


	return finalColor / totalAdditionalWeight;
//
}

float4 HB(in float4 pos: SV_POSITION) : SV_TARGET
{
    float2 UV = pos.xy / g_vFrustumParams.xy;
	float2 SourcePixel = float2(1.0, 1.0) / g_vFrustumParams.xy;

	return Blur(UV, SourcePixel, float2(1, 0));
}

float4 VB(in float4 pos: SV_POSITION) : SV_TARGET
{
    float2 UV = pos.xy / g_vFrustumParams.xy;
	float2 SourcePixel = float2(1.0, 1.0) / g_vFrustumParams.xy;

	return Blur(UV, SourcePixel, float2(0, 1));
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////