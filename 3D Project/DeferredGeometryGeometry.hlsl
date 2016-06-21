struct GSoutput
{
	float4 position : SV_POSITION;
	float3 positionWS : POSITIONWS;
	float3 normalWS : NORMALWS;
	float3 uv : TEXCOORDS;
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
void main(triangle float4 inputPosition[3] : SV_POSITION, triangle float3 inputNormal[3] : NORMAL, triangle float3 inputUV[3] : TEXCOORDS,  inout TriangleStream<GSoutput> GSout)
{
	GSoutput element[3];

	for (int i = 0; i < 3; i++)
	{
		element[i].position = mul(inputPosition[i], worldMatrix);
		element[i].positionWS = element[i].position;
		element[i].normalWS = mul(inputNormal[i], worldMatrix);

		element[i].position = mul(element[i].position, viewMatrix);
	}

	float3 normal = cross(element[1].position - element[0].position, element[2].position - element[0].position);

	if (dot(normal, element[0].position) < 0)
	{
		for (int i = 0; i < 3; i++)
		{
			element[i].position = mul(element[i].position, projectionMatrix);
			element[i].uv = inputUV[i];
			GSout.Append(element[i]);
		}
	}
}