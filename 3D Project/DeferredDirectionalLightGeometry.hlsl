struct VSoutput
{
	float4 position : SV_POSITION;
};

struct GSoutput
{
	float4 position : SV_POSITION;
};

cbuffer world : register(b0)
{
	float4x4 worldMatrix;
}

cbuffer view : register(b1)
{
	float4x4 viewMatrix;
}

cbuffer projection : register(b2)
{
	float4x4 projectionMatrix;
}

[maxvertexcount(3)]
void main(triangle float4 inputPosition[3] : SV_POSITION, inout TriangleStream<GSoutput> GSout)
{
	GSoutput element[3];

	for (int i = 0; i < 3; i++)
	{	
		element[i].position = inputPosition[i];
		GSout.Append(element[i]);
	}
}