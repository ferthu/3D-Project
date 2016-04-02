struct VSoutput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 uv : TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState samp;

float4 PSmain(VSoutput input) : SV_Target
{
	float4 outColor = float4(tex.Sample(samp, input.uv).xyz, 1);

	return outColor;
}