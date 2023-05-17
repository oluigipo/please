struct VS_INPUT
{
	float2      pos : VINPUT0;
	float2x2    scaling : VINPUT1;
	int2        texindex : VINPUT3;
	float4      texcoords : VINPUT4;
	min16float4 color : VINPUT5;
	
	uint vertexIndex : SV_VertexID;
};

struct VS_OUTPUT
{
	float2 texcoords : TEXCOORD0;
	float2 texindex : TEXCOORD3;
	float4 position : SV_POSITION;
	float2 rawscale : TEXCOORD2;
	float2 rawpos : TEXCOORD1;
	min16float4 color : COLOR0;
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
	
	uint index = input.vertexIndex;
	float2 rawpos = float2(index & 1, index >> 1);
	float2 pos = mul(input.scaling, rawpos) + input.pos;
	
	output.position = mul(uView, float4(pos, 0.0, 1.0));
	output.color = input.color;
	output.texcoords.xy = input.texcoords.xy + rawpos * input.texcoords.zw;
	output.texindex = float2(input.texindex);
	output.rawpos = rawpos;
	output.rawscale.xy = float2(length(input.scaling._m00_m10), length(input.scaling._m01_m11));
	
	return output;
}

Texture2D<min16float4> textures[4] : register(t0);
SamplerState samplers[4] : register(s0);

min16float4 Pixel(VS_OUTPUT input) : SV_TARGET
{
	min16float4 result = 0;
	float2 texsize;
	float2 uv = input.texcoords.xy;
	int uindex = abs(int(input.texindex.x));
	int uswizzle = abs(int(input.texindex.y));
	float2 centeredpos = (input.rawpos - 0.5) * 2.0;
	float2 uvfwidth = fwidth(uv);
	
	switch (uindex)
	{
		default:
		case 0: result = textures[0].Sample(samplers[0], uv); texsize = uTexsize01.xy; break;
		case 1: result = textures[1].Sample(samplers[1], uv); texsize = uTexsize01.zw; break;
		case 2: result = textures[2].Sample(samplers[2], uv); texsize = uTexsize23.xy; break;
		case 3: result = textures[3].Sample(samplers[3], uv); texsize = uTexsize23.zw; break;
	}
	
	switch (uswizzle)
	{
		default: break;
		case 1:
		{
			// Derived from: https://www.shadertoy.com/view/4llXD7
			float r = 0.5 * min(input.rawscale.x, input.rawscale.y);
			float2 p = centeredpos * input.rawscale.xy;
			float2 q = abs(p) - input.rawscale.xy + r;
			float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
			
			if (dist >= -1.0)
				result.w *= min16float(max(1.0-(dist+1.0)*0.5, 0.0));
		} break;
		case 2:
		{
			float2 density = uvfwidth * texsize;
			float m = min(density.x, density.y);
			float inv = 1.0 / m;
			float a = (result.x - 128.0/255.0 + 24.0/255.0*m*0.5) * 255.0/24.0 * inv;
			result = min16float4(1.0, 1.0, 1.0, a);
		} break;
	}
	
	return result * input.color;
}
