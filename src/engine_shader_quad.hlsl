struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 texcoords : TEXCOORD0;
	float4 color : COLOR;
	float textureIndex : PSIZE;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	matrix uView;
};

VertexOutput D3d11Shader_QuadVertex(
	float2   inPos : V0_,
	float2   inElemPos : V1_,
	float2x2 inElemScaling : V2_,
	float    inElemTexIndex : V3_,
	float4   inElemTexcoords : V4_,
	float4   inElemColor : V5_)
{
	VertexOutput output;
	
	float2 pos = mul(inElemScaling, inPos) + inElemPos;
	
	output.position = mul(uView, float4(pos, 0.0, 1.0));
	output.color = inElemColor;
	output.texcoords = inElemTexcoords.xy + inPos * inElemTexcoords.zw;
	output.textureIndex = inElemTexIndex;
	
	return output;
}

Texture2D textures[8] : register(t0);
SamplerState samplers[8] : register(s0);

float4 D3d11Shader_QuadPixel(VertexOutput input) : SV_TARGET
{
	float4 result = 0;
	
	switch (int(input.textureIndex))
	{
		default:
		case 0: result = textures[0].Sample(samplers[0], input.texcoords) * input.color; break;
		case 1: result = textures[1].Sample(samplers[1], input.texcoords) * input.color; break;
		case 2: result = textures[2].Sample(samplers[2], input.texcoords) * input.color; break;
		case 3: result = textures[3].Sample(samplers[3], input.texcoords) * input.color; break;
		case 4: result = textures[4].Sample(samplers[4], input.texcoords) * input.color; break;
		case 5: result = textures[5].Sample(samplers[5], input.texcoords) * input.color; break;
		case 6: result = textures[6].Sample(samplers[6], input.texcoords) * input.color; break;
		case 7: result = textures[7].Sample(samplers[7], input.texcoords) * input.color; break;
	}
	
	return result;
}
