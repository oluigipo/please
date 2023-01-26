struct VertexInput
{
	float2   pos : VINPUT0;
	float2   elemPos : VINPUT1;
	float2x2 elemScaling : VINPUT2;
	float2   elemTexIndex : VINPUT4;
	float4   elemTexcoords : VINPUT5;
	float4   elemColor : VINPUT6;
};

struct VertexOutput
{
	float4 texcoords : TEXCOORD0;
	float4 color : COLOR0;
	float4 position : SV_POSITION;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	matrix uView;
};

VertexOutput D3d11Shader_QuadVertex(VertexInput input)
{
	VertexOutput output;
	
	float2 pos = mul(input.elemScaling, input.pos) + input.elemPos;
	
	output.position = mul(uView, float4(pos, 0.0, 1.0));
	output.color = input.elemColor;
	output.texcoords.xy = input.elemTexcoords.xy + input.pos * input.elemTexcoords.zw;
	output.texcoords.zw = input.elemTexIndex;
	
	return output;
}

Texture2D textures[4] : register(t0);
SamplerState samplers[4] : register(s0);

float4 D3d11Shader_QuadPixel(VertexOutput input) : SV_TARGET
{
	float4 result = 0;
	float2 uv = input.texcoords.xy;
	uint uindex = abs(int(input.texcoords.z));
	uint uswizzle = abs(int(input.texcoords.w));

	switch (uindex)
	{
		default:
		case 0: result = textures[0].Sample(samplers[0], uv); break;
		case 1: result = textures[1].Sample(samplers[1], uv); break;
		case 2: result = textures[2].Sample(samplers[2], uv); break;
		case 3: result = textures[3].Sample(samplers[3], uv); break;
	}
	
	switch (uswizzle)
	{
		default:
		case 0: break;
		case 1: result = float4(1.0, 1.0, 1.0, result.x); break;
	}

	result *= input.color;
	
	return result;
}
