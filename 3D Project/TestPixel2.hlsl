struct VSoutput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 uv : TEXCOORD;
};

float4 PSmain(VSoutput input) : SV_Target
{
	float4 outColor = float4(1, 1, 1, 1);

	return outColor;
}