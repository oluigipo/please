struct VertexInput
{
	float2   pos : TEXCOORD0;
	float2   elemPos : TEXCOORD1;
	float2 elemScaling0 : TEXCOORD2;
	float2 elemScaling1 : TEXCOORD3;
	float2   elemTexIndex : TEXCOORD4;
	float4   elemTexcoords : TEXCOORD5;
	float4   elemColor : TEXCOORD6;
};

struct VertexOutput
{
	float4 texcoords : TEXCOORD8;
	float4 color : COLOR0;
	float4 position : SV_POSITION;
	float2 rawpos : TEXCOORD9;
	float2 rawscale : TEXCOORD10;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	float4 uTexsize01;
	float4 uTexsize23;
	matrix uView;
};

VertexOutput D3d9cShader_QuadVertex(VertexInput input)
{
	VertexOutput output;
	
	float2x2 elemScaling = float2x2(input.elemScaling0, input.elemScaling1);
	float2 pos = mul(elemScaling, input.pos) + input.elemPos;
	
	output.position = mul(uView, float4(pos, 0.0, 1.0));
	output.color = input.elemColor;
	output.texcoords.xy = input.elemTexcoords.xy + input.pos * input.elemTexcoords.zw;
	output.texcoords.zw = input.elemTexIndex;
	output.rawpos = input.pos;
	output.rawscale = float2(length(elemScaling._m00_m10), length(elemScaling._m01_m11));
	
	return output;
}

Texture2D texture0 : register(s0);
Texture2D texture1 : register(s1);
Texture2D texture2 : register(s2);
Texture2D texture3 : register(s3);
SamplerState samplerState;

float4 D3d9cShader_QuadPixel(VertexOutput input) : SV_TARGET
{
	float4 result = 0;
	float2 uv = input.texcoords.xy;
	float2 texsize = 0;
	uint uindex = abs(int(input.texcoords.z));
	uint uswizzle = abs(int(input.texcoords.w));

	switch (uindex)
	{
		default:
		case 0: result = texture0.Sample(samplerState, uv); texsize = uTexsize01.xy; break;
		case 1: result = texture1.Sample(samplerState, uv); texsize = uTexsize01.zw; break;
		case 2: result = texture2.Sample(samplerState, uv); texsize = uTexsize23.xy; break;
		case 3: result = texture3.Sample(samplerState, uv); texsize = uTexsize23.zw; break;
	}
	
	switch (uswizzle)
	{
		default: break;
		case 1: result = float4(1.0, 1.0, 1.0, result.r); break;
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
		case 4:
		{
			float m = min(input.rawscale.x, input.rawscale.y);
			float inv = 1.0 / m;
			float a = (result.r - 0.5 + inv) * (m * 0.5);
			result = float4(1.0, 1.0, 1.0, a);
		} break;
		case 5:
		{
			float2 density = fwidth(uv) * texsize;
			float m = min(density.x, density.y);
			float inv = 1.0 / m;
			float a = (result.r - 128.0/255.0 + 24.0/255.0*m*0.5) * 255.0/24.0 * inv;
			result = float4(1.0, 1.0, 1.0, a);
		} break;
	}

	return result * input.color;
}
