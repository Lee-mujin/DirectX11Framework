cbuffer TerrainConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    matrix WorldViewProjection;
    float4 LightDirection;
    float4 LightColor;
    float4 AmbientColor;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
    float3 worldPos : WORLDPOS;
};

Texture2D    TerrainTexture : register(t0);
SamplerState Sampler        : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
    // Directional lighting
    float3 lightDir = normalize(-LightDirection.xyz);
    float3 normal = normalize(input.normal);
    float nDotL = max(dot(normal, lightDir), 0.0f);
    
    float4 diffuse = LightColor * nDotL;
    float4 finalLight = AmbientColor + diffuse;
    
    // Clamp light intensity
    finalLight = saturate(finalLight);
    
    float4 baseColor = input.color;
    float4 finalColor = baseColor * finalLight;
    finalColor.a = 1.0f;
    
    return finalColor;
}
