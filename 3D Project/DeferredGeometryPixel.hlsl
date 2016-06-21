struct GSoutput
{
	float4 position : SV_POSITION;
	float3 positionWS : POSITIONWS;
	float3 normalWS : NORMALWS;
	float3 uv : TEXCOORDS;
};

struct PSoutput
{
	float4 color : SV_Target0;
	float4 positionWS : SV_Target1;
	float4 normalWS : SV_Target2;
};

Texture2D tex : register(t0);
SamplerState samp;

PSoutput main(GSoutput input)
{
	PSoutput output;

	output.color = tex.Sample(samp, input.uv);
	output.positionWS = float4(input.positionWS, 1.0f);
	output.normalWS = float4(input.normalWS, 1.0f);

	return output;
}