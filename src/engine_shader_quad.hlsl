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
	float2 rawpos : TEXCOORD1;
	float2 rawscale : TEXCOORD2;
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
	output.rawpos = input.pos;
	output.rawscale = float2(length(input.elemScaling._m00_m10), length(input.elemScaling._m01_m11));
	
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
		case 2:
		{
			float2 pos = (input.rawpos - 0.5) * 2.0;
			if (dot(pos, pos) >= 1.0)
				result.w = 0.0;
		} break;
		case 3:
		{
			// Derived from: https://www.shadertoy.com/view/4llXD7
			float r = 0.5 * min(input.rawscale.x, input.rawscale.y);
			float2 p = (input.rawpos - 0.5) * 2.0 * input.rawscale;
			float2 q = abs(p) - input.rawscale + r;
			float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;

			if (dist >= -1.0)
				result.w *= max(1.0-(dist+1.0)*0.5, 0.0);
		} break;
		case 4: result = float4(1.0, 1.0, 1.0, min(max((result.x-0.4625)*12.0, 0.0), 1.0)); break;
	}

	result *= input.color;
	
	return result;
}
