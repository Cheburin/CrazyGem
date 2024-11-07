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

static const float4 cubeVerts[8] = 
{
	float4(-0.5, -0.5, -0.5, 1),// LB  0
	float4(-0.5, 0.5, -0.5, 1), // LT  1
	float4(0.5, -0.5, -0.5, 1), // RB  2
	float4(0.5, 0.5, -0.5, 1),  // RT  3
	float4(-0.5, -0.5, 0.5, 1), // LB  4
	float4(-0.5, 0.5, 0.5, 1),  // LT  5
	float4(0.5, -0.5, 0.5, 1),  // RB  6
	float4(0.5, 0.5, 0.5, 1)    // RT  7
};

static const int cubeIndices[24] =
{
	0, 1, 2, 3, // front
	7, 6, 3, 2, // right
	7, 5, 6, 4, // back
	4, 0, 6, 2, // bottom
	1, 0, 5, 4, // left
	3, 1, 7, 5  // top
};

struct ClipPos
{
    float4 clip_pos    : SV_POSITION; // Output position
};

[maxvertexcount(36)]
void GS(point ClipPos pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<ClipPos> triStream )
{
	ClipPos v[8];
	[unroll]
	for (int j = 0; j < 8; j++)
	{
		v[j].clip_pos = mul(cubeVerts[j], g_mWorldViewProjection);
	}
	
	[unroll]
	for (int i = 0; i < 6; i++)
	{
		triStream.Append(v[cubeIndices[i * 4 + 1]]);
		triStream.Append(v[cubeIndices[i * 4 + 2]]);
		triStream.Append(v[cubeIndices[i * 4]]);
		triStream.RestartStrip();
		
		triStream.Append(v[cubeIndices[i * 4 + 3]]);
		triStream.Append(v[cubeIndices[i * 4 + 2]]);
		triStream.Append(v[cubeIndices[i * 4 + 1]]);
		triStream.RestartStrip();
	}
}

ClipPos VS( in ClipPos i )
{
    ClipPos output;

    output.clip_pos = i.clip_pos;
    
    return output;
}; 

float4 PS(): SV_TARGET{
    return float4(1,1,1,1);
}

float4 PS_GREEN(): SV_TARGET{
    return float4(0,1,0,1);
}
/// static line 
[maxvertexcount(36)]
void GS_LINE(point ClipPos pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<ClipPos> triStream )
{
	float4 p1 = g_point;
	float4 p2 = 50.0f*g_direction + g_point;
    float4 i1 = p1;//p1+10.0f*(p2-p1);
	float4 i2 = p2;//p1-10.0f*(p2-p1);

	ClipPos p;

	p.clip_pos = mul(i1, g_mWorldViewProjection);
	triStream.Append(p);
	p.clip_pos = mul(i2, g_mWorldViewProjection);
	triStream.Append(p);
	p.clip_pos = mul(i1, g_mWorldViewProjection);
	triStream.Append(p);

	triStream.RestartStrip();
}

ClipPos VS_LINE( in ClipPos i )
{
    ClipPos output;

    output.clip_pos = i.clip_pos;
    
    return output;
}; 

float4 PS_LINE(): SV_TARGET{
    return g_color;
}