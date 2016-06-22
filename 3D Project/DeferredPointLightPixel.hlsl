struct GSoutput
{
	float4 position : SV_POSITION;
};

Texture2D colorBuffer : register(t0);
Texture2D positionBuffer : register(t1);
Texture2D normalBuffer : register(t2);

cbuffer data : register(b0)
{
	float4 positionWS;
	float4 color;
	float range;
	float3 padding;
};

cbuffer ambientCol : register(b1)
{
	float4 ambientColor;
}

cbuffer world : register(b2)
{
	matrix worldMatrix;
}

float4 main(GSoutput input) : SV_Target
{
	//float4 lightPos = mul(position, worldMatrix);

	int3 samplePos = int3(input.position.xy, 0);

	float4 readColor = colorBuffer.Load(samplePos);
	float4 readPosition = positionBuffer.Load(samplePos);
	float4 readNormal = normalBuffer.Load(samplePos);

	float rangeAttenuation = 1 - clamp((distance(positionWS.xyz, readPosition.xyz) / range), 0.0f, 1.0f);

	float angleModifier = clamp(dot(normalize(positionWS.xyz - readPosition.xyz), normalize(readNormal.xyz)), 0.0f, 1.0f);

	return readColor * color * rangeAttenuation * angleModifier;
}