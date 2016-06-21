struct GSoutput
{
	float4 position : SV_POSITION;
};

Texture2D colorBuffer : register(t0);
Texture2D positionBuffer : register(t1);
Texture2D normalBuffer : register(t2);

cbuffer data
{
	float4 color;
	float4 direction;
};

cbuffer ambientCol : register(b1)
{
	float4 ambientColor;
}

float4 main(GSoutput input) : SV_Target
{
	int3 samplePos = int3(input.position.xy, 0);

	float4 readColor = colorBuffer.Load(samplePos);

	return (ambientColor * readColor) + max(0.0f, dot(-direction, normalBuffer.Load(samplePos)));
}