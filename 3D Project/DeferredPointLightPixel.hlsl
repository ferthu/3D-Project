struct GSoutput
{
	float4 position : SV_POSITION;
};

Texture2D colorBuffer : register(t0);
Texture2D positionBuffer : register(t1);
Texture2D normalBuffer : register(t2);

cbuffer data
{
	float4 position;
	float4 color;
	float range;
};

cbuffer ambientCol : register(b1)
{
	float4 ambientColor;
}

cbuffer world : register(b2)
{
	float4x4 worldMatrix;
}

float4 main(GSoutput input) : SV_Target
{
	float4 lightPos = mul(position, worldMatrix);

	int3 samplePos = int3(input.position.xy, 0);

	float4 readColor = colorBuffer.Load(samplePos);
	float4 readPosition = positionBuffer.Load(samplePos);
	float4 readNormal = normalBuffer.Load(samplePos);

	return readColor * color;
}