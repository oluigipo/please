struct VS_INPUT
{
	float2 pos : VINPUT0;
	float2 normPos : VINPUT1;
	float2 size : VINPUT2;
	int2   texIndex : VINPUT3;
	float2 texcoords : VINPUT4;
	float4 color : VINPUT5;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 texcoords : TEXCOORD0;
	float2 normPos : TEXCOORD1;
	float4 size : TEXCOORD2;
	float4 color : COLOR0;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	matrix uView;
	float4 uTexsize01;
	float4 uTexsize23;
};

VS_OUTPUT Vertex(VS_INPUT input)
{
	VS_OUTPUT output;
	
	output.position = mul(uView, float4(input.pos, 0.0, 1.0));
	output.texcoords.xy = input.texcoords;
	output.texcoords.zw = float2(input.texIndex);
	output.normPos = input.normPos;
	output.size.xy = input.size;
	output.size.zw = 1.0 / input.size;
	output.color = input.color;
	
	return output;
}

Texture2D textures[4] : register(t0);
SamplerState samplers[4] : register(s0);

float4 Pixel(VS_OUTPUT input) : SV_TARGET
{
	float4 result;
	float2 texsize;
	
	float2 uv = input.texcoords.xy;
	int index = int(input.texcoords.z);
	int swizzle = int(input.texcoords.w);
	float2 size = input.size.xy;
	float2 uvfwidth = input.size.zw;
	float2 centeredpos = (input.normPos - 0.5) * 2.0;
	
	if (index == 1)
	{ result = textures[1].Sample(samplers[1], uv); texsize = uTexsize01.zw; }
	else if (index == 2)
	{ result = textures[2].Sample(samplers[2], uv); texsize = uTexsize23.xy; }
	else if (index == 3)
	{ result = textures[3].Sample(samplers[3], uv); texsize = uTexsize23.zw; }
	else
	{ result = textures[0].Sample(samplers[0], uv); texsize = uTexsize01.xy; }
	
	if (swizzle == 1)
	{
		// Derived from: https://www.shadertoy.com/view/4llXD7
		float r = 0.5 * min(size.x, size.y);
		float2 p = centeredpos * size;
		float2 q = abs(p) - size + r;
		float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
		
		if (dist >= -1.0)
			result.w *= max(1.0-(dist+1.0)*0.5, 0.0);
	}
	else if (swizzle == 2)
	{
		float2 density = uvfwidth * texsize;
		float m = min(density.x, density.y);
		float inv = 1.0 / m;
		float a = (result.x - 128.0/255.0 + 24.0/255.0*m*0.5) * 255.0/24.0 * inv;
		
		result = float4(1.0, 1.0, 1.0, a);
	}
	
	return result * input.color;
}
