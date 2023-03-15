struct VS_INPUT
{
	float2   pos : VINPUT0;
	float2   elemPos : VINPUT1;
	float2x2 elemScaling : VINPUT2;
	int2     elemTexIndex : VINPUT4;
	float4   elemTexcoords : VINPUT5;
	float4   elemColor : VINPUT6;
};

struct VS_OUTPUT
{
	float2 texcoords : TEXCOORD0;
	float2 texindex : TEXCOORD3;
	float4 color : COLOR0;
	float4 position : SV_POSITION;
	float2 rawpos : TEXCOORD1;
	float4 rawscale : TEXCOORD2;
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
	
	float2 pos = mul(input.elemScaling, input.pos) + input.elemPos;
	
	output.position = mul(uView, float4(pos, 0.0, 1.0));
	output.color = input.elemColor;
	output.texcoords.xy = input.elemTexcoords.xy + input.pos * input.elemTexcoords.zw;
	output.texindex = float2(input.elemTexIndex);
	output.rawpos = input.pos;
	output.rawscale.xy = float2(length(input.elemScaling._m00_m10), length(input.elemScaling._m01_m11));
	output.rawscale.zw = input.elemTexcoords.zw;
	
	return output;
}

Texture2D textures[4] : register(t0);
SamplerState samplers[4] : register(s0);

float4 Pixel(VS_OUTPUT input) : SV_TARGET
{
	float4 result = 0;
	float2 uv = input.texcoords.xy;
	float2 texsize;
	float2 centeredpos = (input.rawpos - 0.5) * 2.0;
	int uindex = abs(int(input.texindex.x));
	int uswizzle = abs(int(input.texindex.y));
	
#ifndef PROFILE_4_0_level_9_1
	float2 uvfwidth = fwidth(uv);
#else
	float2 uvfwidth = input.rawscale.zw / input.rawscale.xy;
#endif
	
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
		case 1: result = float4(1.0, 1.0, 1.0, result.x); break;
		case 2:
		{
			if (dot(centeredpos, centeredpos) >= 1.0)
				result.w = 0.0;
		} break;
#ifndef PROFILE_4_0_level_9_1
		case 3:
		{
			// Derived from: https://www.shadertoy.com/view/4llXD7
			float r = 0.5 * min(input.rawscale.x, input.rawscale.y);
			float2 p = centeredpos * input.rawscale.xy;
			float2 q = abs(p) - input.rawscale.xy + r;
			float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
			
			if (dist >= -1.0)
				result.w *= max(1.0-(dist+1.0)*0.5, 0.0);
		} break;
#endif
		case 4:
		{
			float2 density = uvfwidth * texsize;
			float m = min(density.x, density.y);
			float inv = 1.0 / m;
			float a = (result.x - 128.0/255.0 + 24.0/255.0*m*0.5) * 255.0/24.0 * inv;
			result = float4(1.0, 1.0, 1.0, a);
		} break;
	}
	
	return result * input.color;
}
