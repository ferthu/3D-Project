struct VSinput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 uv : TEXCOORD;
};

struct VSoutput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 uv : TEXCOORD;
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

VSoutput VSmain(VSinput input)
{
	VSoutput output = (VSoutput)0;

	output.position = mul(float4(input.position, 1), worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);
	output.normal = input.normal;
	output.uv = input.uv;

	return output;
}