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

Texture2D diffuseTexture  : register( t0 );
Texture2D<float4> shadowTexture  : register( t1 );

Texture2D<float4> depth1  : register( t2 );

//SamplerState linearSampler : register( s0 );
SamplerState SampleTypeClamp : register( s0 );
SamplerState SampleTypeWrap : register( s1 );

/*
struct Targets
{
    float4 color: SV_Target0;

    float4 normal: SV_Target1;
};
*/
float4 MODEL_FRAG(
    float2 tex            : TEXCOORD1,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   float4 textureColor = diffuseTexture.Sample(SampleTypeWrap, tex);
   if(textureColor.a > 0.8f) 
     return float4(clip_pos.z,clip_pos.z,clip_pos.z,0);//pos.z/pos.w,pos.z/pos.w,pos.z/pos.w,1);//pos.z/pos.w,pos.z/pos.w,pos.z/pos.w,1);///colorMap.Sample( linearSampler, tex.xy); 
   else
     discard; 
   return float4(clip_pos.z,clip_pos.z,clip_pos.z,0);  
   //return float4(1.0,1.0,1.0,0);//pos.z/pos.w,pos.z/pos.w,pos.z/pos.w,1);//pos.z/pos.w,pos.z/pos.w,pos.z/pos.w,1);///colorMap.Sample( linearSampler, tex.xy);
   //return float4(clip_pos.z,clip_pos.z,clip_pos.z,1);///colorMap.Sample( linearSampler, tex.xy);
};

float4 MODEL_PRE_SHADOW(
    float3 norm         : TEXCOORD1,
    float2 tex            : TEXCOORD2,
	float4 L1       : TEXCOORD3,
	float4 L2       : TEXCOORD4,
     float3 P       : TEXCOORD5    
):SV_TARGET
{ 
    float3 normal = normalize(norm);

    const float3 lightDirection = g_viewLightPos.xyz;
    const float4 diffuseColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    const float4 ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);

	float bias;
    float4 color;
	float2 projectTexCoord;
	float depthValue;
	float lightDepthValue;
    float lightIntensity;
	float4 textureColor;
	float3 lightDir;

	bias = 0.001f;

    color = ambientColor;

	textureColor = diffuseTexture.Sample(SampleTypeWrap, tex);

	projectTexCoord.x =  (L1.x / 2.0f) + 0.5f;
	projectTexCoord.y = (-L1.y / 2.0f) + 0.5f;

    bool intoShadowMap = (saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y);
	
    if(intoShadowMap)
	{
		depthValue = depth1.Sample(SampleTypeClamp, projectTexCoord).r;

		lightDepthValue = L1.z / L1.w;

		lightDepthValue = lightDepthValue - bias;
    }
    if((intoShadowMap && lightDepthValue < depthValue) || !intoShadowMap)
    {
        lightDir = -lightDirection;
        lightIntensity = saturate(dot(normal, lightDir));

        if(lightIntensity > 0.0f)
        {
            color += (diffuseColor * lightIntensity);

            color = saturate(color);
        }
        //return float4(0,0,0,1);///color * textureColor;
    }

	color = color * textureColor;

    return color;

};

float4 MODEL_FRAG_SHADOW(
    float3 normal         : TEXCOORD1,
    float2 tex            : TEXCOORD2,
	float4 L1       : TEXCOORD3,
	float4 L2       : TEXCOORD4,
     float3 P       : TEXCOORD5,    
    float4 clip_pos       : SV_POSITION
):SV_TARGET{
    float4 ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);
    float4 diffuseColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float3 lightPosition = float3(5.0f, 8.0f, -5.0f);

    float4 color = ambientColor;
    float3 lightPos = normalize(lightPosition - P);

    // Calculate the amount of light on this pixel.
    float lightIntensity = saturate(dot(normal, lightPos));

    if(lightIntensity > 0.0f)
    {
        // Determine the light color based on the diffuse color and the amount of light intensity.
        color += (diffuseColor * lightIntensity);

        // Saturate the light color.
        color = saturate(color);
    }

    // Sample the pixel color from the texture using the sampler at this texture coordinate location.
    float4 textureColor = diffuseTexture.Sample(SampleTypeWrap, tex);

    // Sample the shadow value from the shadow texture using the sampler at the projected texture coordinate location.
    float4 shadowValue = shadowTexture.Load(int3(clip_pos.xy,0)).r;

    // Combine the shadows with the final color.
    color = color * textureColor * shadowValue;

    return color;    
}

//////////////////
float2 EyeToTex(float3 eye)
{
    float2 g_FocalLen = float2(1,1);

    float2 win = g_FocalLen * eye.xy / eye.z;
    win.xy *= float2(0.5, -0.5);
    win.xy += float2(0.5, 0.5);
    return win;
}

float3 TexToEye(float2 texcoords, float eye_z)
{
    float2 g_FocalLen = float2(1,1);

    texcoords.xy -= float2(0.5, 0.5);
    texcoords.xy /= float2(0.5, -0.5);
    float2 eye_xy = (texcoords.xy / g_FocalLen) * eye_z;
    return float3(eye_xy, eye_z);
}
//////////////////
Texture2D<float4> projectedTexture  : register( t2 );
//////////////////

float4 MODEL_FRAG_WITH_TEXTURE(
    float3 normal         : TEXCOORD1,
    float2 tex            : TEXCOORD2,
	float4 L1       : TEXCOORD3,
	float4 L2       : TEXCOORD4,
     float3 P       : TEXCOORD5,    
    float4 clip_pos       : SV_POSITION
):SV_TARGET{
    float4 ambientColor = float4(0.15f, 0.15f, 0.15f, 1.0f);
    float4 diffuseColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float3 lightPosition = float3(5.0f, 8.0f, -5.0f);

    float4 color = float4(0,0,0,0);//ambientColor;
    float3 lightPos = normalize(lightPosition - P);

    // Calculate the amount of light on this pixel.
    float lightIntensity = saturate(dot(normal, lightPos));

    if(lightIntensity > 0.0f)
    {
        // Determine the light color based on the diffuse color and the amount of light intensity.
        color += (diffuseColor * lightIntensity);

        // Saturate the light color.
        color = saturate(color);
    }

    // Sample the pixel color from the texture using the sampler at this texture coordinate location.
    float4 textureColor = diffuseTexture.Sample(SampleTypeWrap, tex);

    // Sample the shadow value from the shadow texture using the sampler at the projected texture coordinate location.
    float4 shadowValue = 1.0;//shadowTexture.Load(int3(clip_pos.xy,0)).r;

    // Combine the shadows with the final color.
    float2 projectTexCoord = EyeToTex(L1);
    
    float4 projectionColor = float4(1,1,1,1);
    
    // Determine if the projected coordinates are in the 0 to 1 range.  If it is then this pixel is inside the projected view port.
    if((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
    {
       //Sample the projection texture using the projected texture coordinates and then set the output pixel color to be the projected texture color.

       // Sample the color value from the projection texture using the sampler at the projected texture coordinate location.
       
       projectionColor = projectedTexture.Sample(SampleTypeWrap, projectTexCoord);
       
       color = saturate((color * projectionColor * textureColor) + (ambientColor * textureColor));
    }
    else {
       color = ambientColor * textureColor;
    }

    //color = color * textureColor * shadowValue * projectionColor;

    return color;    
}