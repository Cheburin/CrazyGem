float4 VS(float3 pos : SV_Position):SV_POSITION
{
  return float4( pos.xyz, 1.0 );
} 
