struct VertexOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VertexOutput Shader_DefaultVertex(float4 inPos : POSITION, float4 inColor : COLOR)
{
	VertexOutput output;

	output.position = inPos;
	output.color = inColor;

	return output;
}

float4 Shader_DefaultPixel(VertexOutput input) : SV_TARGET
{
	return input.color;
}
