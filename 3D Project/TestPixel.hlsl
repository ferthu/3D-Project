struct VSoutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

float4 PSmain(VSoutput input) : SV_Target
{
	float4 outColor = float4(input.color, 1);

	return outColor;
}