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

Texture2D<float4> colorMap                 : register( t0 );
Texture2D<float4> normalMap                : register( t1 );
Texture2D<float>  depthMap                 : register( t2 );

/////////////////////////////////////////////////////////////
float3 GetColor(float2 frag)
{
    frag.x = clamp(frag.x, 0, g_vFrustumParams.x-1);
    frag.y = clamp(frag.y, 0, g_vFrustumParams.y-1);
	return colorMap.Load( int3(frag.xy,0) ).xyz;
}

float3 GetNormal(float2 frag)
{
    frag.x = clamp(frag.x, 0, g_vFrustumParams.x-1);
    frag.y = clamp(frag.y, 0, g_vFrustumParams.y-1);
	return normalMap.Load( int3(frag.xy,0) ).xyz;
}

float GetDepth(float2 frag)
{
    frag.x = clamp(frag.x, 0, g_vFrustumParams.x-1);
    frag.y = clamp(frag.y, 0, g_vFrustumParams.y-1);
    return depthMap.Load(  int3(frag.xy,0) ).x;
}

float nonLinearToLinearDepth(float depth)
{
    float near = g_vFrustumNearFar.x;
    float far =  g_vFrustumNearFar.y;
    return near * far / (far - depth * (far - near));
}

/////////////////////////////////////////////////////////////
float3 GetPosition(float2 frag, float depth)
{
    float2 ndc = float2(frag.xy/g_vFrustumParams.xy) * float2(2, -2) + float2(-1, 1); 

    float3 view_p = nonLinearToLinearDepth(depth) * float3(ndc.x * g_vFrustumParams.w/g_vFrustumParams.z, ndc.y * 1/g_vFrustumParams.z, 1);

    return mul( float4( view_p, 1.0 ), g_mInvView ).xyz;
}

float3 GetProjection(float3 position)
{
	 float4 projected_p = mul( float4(position, 1.0f), g_mWorldViewProjection);
     projected_p /= projected_p.w;
	 return float3((float2(0.5f, -0.5f) * projected_p.xy + float2(0.5f, 0.5f))*g_vFrustumParams.xy, projected_p.z);
}
/////////////////////////////////////////////////////////////
float4 PS(in float4 frag: SV_POSITION):SV_TARGET
{ 
    float3 texelColor =       GetColor(frag.xy);
    float3 texelNormal =     GetNormal(frag.xy);
    float  texelDepth =       GetDepth(frag.xy);
    float3 texelPosition = GetPosition(frag.xy, texelDepth);

    float3 cameraPosition = g_mInvView._m30_m31_m32;
    
    float3 V = normalize(cameraPosition - texelPosition);

    float3 R = normalize(reflect(-V, texelNormal));

    float frenel = saturate(pow(1-dot(V, texelNormal), 1));
    
    //return float4(frenel, frenel, frenel, 0);

    float3 newPosition = float3(0,0,0);
    float3 currentPosition = 0;
    float3 nuv = 0;
    float n = 0;
    float L = 1;
    float LDelmiter = 0.001;

    float error = 0;
    for(int i = 0; i < 10; i++)
    {
        currentPosition = texelPosition + R * L;

        nuv = GetProjection(currentPosition);
        if(clamp(nuv.x, 0, g_vFrustumParams.x-1)!=nuv.x){
            error = 1;
            break;
        };

        if(clamp(nuv.y, 0, g_vFrustumParams.y-1)!=nuv.y){
            error = 1;
            break;
        };

        n = GetDepth(nuv.xy);

        newPosition = GetPosition(nuv.xy, n);
        L = length(texelPosition - newPosition);    
    }

    float3 reflectColor = float3(0,0,0);

    //L = saturate(L * LDelmiter);

    //float error = (1 - L);
   
    //if(abs(n-nuv.z)<0.0001)
    if(error!=0)
        reflectColor = GetColor(nuv.xy).rgb; //error*frenel*
   
    //float fresnel = 0.0 + 2.8 * pow(1+dot(viewDir, normal), 2);
    /*
    float4 reflectionDistanceFadeFactor = float4(1,1,1,1);
    float4 edgeFactorPower = float4(1,1,1,1);
    float fresnel = 1;
    
    float distFactor = 1.0 - saturate(L * reflectionDistanceFadeFactor.x);

    float2 vCoordsEdgeFact = float2(1.0, 1.0) - pow(saturate(abs(nuv.xy - float2(0.5, 0.5)) * 2.0), edgeFactorPower.x);
    float fScreenEdgeFactor = saturate(min(vCoordsEdgeFact.x, vCoordsEdgeFact.y));

    float reflectionMultiplier = fresnel * saturate(reflectDir.z) * fScreenEdgeFactor;

    reflectionMultiplier = clamp(reflectionMultiplier, 0.0, 1.0);
    */
    return float4(texelColor + reflectColor, 1);//float4(color*reflectionMultiplier, 1);//float4(color + cnuv, 1);
}