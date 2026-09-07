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

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 color    : COLOR;
    float3 worldPos : WORLDPOS;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = mul(float4(input.position, 1.0f), WorldViewProjection);
    output.worldPos = mul(float4(input.position, 1.0f), World).xyz;
    output.normal = normalize(mul(float4(input.normal, 0.0f), World).xyz);
    output.texcoord = input.texcoord;
    output.color = input.color;
    
    return output;
}