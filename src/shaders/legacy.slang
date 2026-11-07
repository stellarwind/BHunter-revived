// Original CPU lighting and reversed near/z depth; perspective-correct UVs.
[[vk::binding(0, 2)]]
Sampler2D image;
[[vk::binding(0, 3)]]
cbuffer Draw
{
    float4 controls;
    float4 fogColor;
};
[[vk::binding(0, 1)]]
cbuffer Canvas
{
    float4 canvas;
};
struct Input
{
    float4 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 color : TEXCOORD2;
    float4 specular : TEXCOORD3;
};
struct Output
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    noperspective float4 color : COLOR0;
    noperspective float4 specular : COLOR1;
};
Output VSMain(Input input)
{
    Output o;
    float w = 1.0 / max(input.position.w, 1e-20);
    // Map the logical canvas bounds directly to the GPU viewport edges.
    o.position = float4(((input.position.x - 320.0) / canvas.x) * w,
                        ((240.0 - input.position.y) / canvas.y) * w, input.position.z * w, w);
    o.uv = input.uv;
    o.color = input.color.bgra;
    o.specular = input.specular.bgra;
    return o;
}
float4 PSMain(Output input) : SV_Target0
{
    float4 texel = image.Sample(input.uv);
    if (controls.x > 0.5 && texel.a < 0.5)
        discard;
    float3 rgb = saturate(texel.rgb * input.color.rgb + input.specular.rgb);
    if (controls.y > 0.5)
        rgb = lerp(fogColor.rgb, rgb, input.specular.a);
    return float4(rgb, input.color.a);
}
