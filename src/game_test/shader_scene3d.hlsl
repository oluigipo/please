struct VS_INPUT
{
	float3 pos : VINPUT0;
	float2 texcoord : VINPUT1;
	float3 normal : VINPUT2;
};

struct VS_OUTPUT
{
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL;
	float4 position : SV_POSITION;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	matrix uView;
	matrix uModel;
};

VS_OUTPUT Vertex(VS_INPUT input)
{
	VS_OUTPUT output;

	output.position = mul(mul(uView, uModel), float4(input.pos, 1.0));
	output.normal = input.normal;
	output.texcoord = input.texcoord;

	return output;
};

Texture2D textures[4] : register(t0);
SamplerState samplers[4] : register(s0);

float4 Pixel(VS_OUTPUT input) : SV_TARGET
{
	return textures[0].Sample(samplers[0], input.texcoord);
}

