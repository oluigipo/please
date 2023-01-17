struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 texcoords : TEXCOORD0;
	float4 color : COLOR;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	matrix uView;
};

VertexOutput Shader_DefaultVertex(
	float2 inPos : V_POSITION,
	matrix inTransform : I_TRANSFORM,
	float4 inTexcoords : I_TEXCOORDS,
	float4 inColor : I_COLOR)
{
	VertexOutput output;
	
	output.position = mul(mul(uView, inTransform), float4(inPos, 0.0, 1.0));
	output.color = inColor;
	output.texcoords = inTexcoords.xy + inPos * inTexcoords.zw;
	
	return output;
}

Texture2D tex : register(t0);
SamplerState samplerState;

float4 Shader_DefaultPixel(VertexOutput input) : SV_TARGET
{
	return tex.Sample(samplerState, input.texcoords) * input.color;
}
