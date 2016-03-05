struct GSoutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

float4 PSmain(GSoutput input) : SV_Target
{
	float4 outColor = float4(input.color, 1);

	return outColor;
}