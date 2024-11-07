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

    float4    g_point;
    float4    g_direction;
    float4    g_color;    
};

struct PosNormalTex2d
{
    float3 pos : SV_Position;
    float3 normal   : NORMAL;
    float2 tex      : TEXCOORD0;
};

struct ClipPosNormalTex2d
{
    float3 normal         : TEXCOORD1;   // Normal vector in world space
    float2 tex            : TEXCOORD2;

	float4 L1       : TEXCOORD3;
	float4 L2       : TEXCOORD4;
    float3 P        : TEXCOORD5;
    float4 clip_pos       : SV_POSITION; // Output position
};

struct ClipPos
{
    float2 tex         : TEXCOORD1;
    float4 clip_pos    : SV_POSITION; // Output position
};
///////////////////////////////////////////////////////////////////////////////////////////////////

ClipPosNormalTex2d MODEL_VERTEX( in PosNormalTex2d i )
{
    ClipPosNormalTex2d output;

    output.normal = normalize( i.normal );

    output.tex = i.tex;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
    
	output.L1 = mul( float4( i.pos, 1.0 ), g_mObject1WorldViewProjection );

	output.L2 = mul( float4( i.pos, 1.0 ), g_mObject2WorldViewProjection );

	output.P = mul( float4( i.pos, 1.0 ), g_mWorld );

    return output;
}; 

ClipPos MODEL_VERTEX1( in PosNormalTex2d i )
{
    ClipPos output;

    output.tex = i.tex;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mObject1WorldViewProjection );

    return output;
}; 

ClipPos MODEL_VERTEX2( in PosNormalTex2d i )
{
    ClipPos output;

    output.tex = i.tex;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mObject2WorldViewProjection );

    return output;
}; 