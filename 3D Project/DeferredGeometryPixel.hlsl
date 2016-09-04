struct GSoutput
{
	float4 position : SV_POSITION;
	float3 positionWS : POSITIONWS;
	float3 normalWS : NORMALWS;
	float3 tangentWS : TANGENTWS;
	float3 uv : TEXCOORDS;
};

struct PSoutput
{
	float4 color : SV_Target0;
	float4 positionWS : SV_Target1;
	float4 normalWS : SV_Target2;
};

float4 objectColor : register (b0);
Texture2D tex : register(t0);
Texture2D normalMap : register(t1);
SamplerState samp;

PSoutput main(GSoutput input)
{
	PSoutput output;

	output.color = tex.Sample(samp, input.uv) * objectColor;
	output.positionWS = float4(input.positionWS, 1.0f);


	float3 readNormal = normalMap.Sample(samp, input.uv);
	// convert to [-1,1] from [0,1] and normalize
	readNormal = normalize(2.0f * readNormal - 1.0f);
	
	// make surface tangent orthogonal to surface normal
	float3 T = normalize(input.tangentWS - dot(input.normalWS, input.tangentWS) * input.normalWS);

	// find bitangent
	float3 B = cross(input.normalWS, T);

	// find matrix to transform from tangent to world space
	float3x3 TBN = float3x3(T, B, input.normalWS);

	// transform normal to world space and write result to normal buffer
	float3 normal = mul(readNormal, TBN);

	// convert back to [0,1]
	normal = (normal / 2.0f) + 0.5f;

	output.normalWS = float4(normal, 0.0f);

	return output;
}