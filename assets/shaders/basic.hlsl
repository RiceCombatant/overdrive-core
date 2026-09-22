cbuffer TransformBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 CustomParams; // x: useLighting (1: Lit 3D, 0: Unlit/Emissive/Grid)
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal   : NORMAL;
    float4 Color    : COLOR;
    float3 WorldPos : WORLDPOS;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    float4 viewPos  = mul(worldPos, View);
    output.Position = mul(viewPos, Projection);
    
    output.Normal   = normalize(mul(input.Normal, (float3x3)World));
    output.Color    = input.Color;
    output.WorldPos = worldPos.xyz;
    
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    if (CustomParams.x < 0.5f)
    {
        // Unlit / Emissive (for Cyber Grid lines and UI elements)
        return input.Color;
    }
    
    // Directional light from top-right-front (AC Test Hangar lighting)
    float3 lightDir = normalize(float3(0.4f, 0.9f, -0.5f));
    float diff = max(dot(input.Normal, lightDir), 0.0f);
    
    // Ambient light (slate metal tone)
    float3 ambient = float3(0.22f, 0.25f, 0.30f);
    float3 diffuse = diff * float3(0.85f, 0.88f, 0.95f);
    
    float3 finalColor = input.Color.rgb * (ambient + diffuse);
    return float4(finalColor, input.Color.a);
}
