struct VSinput
{
	float3 position : POSITION;
	float3 color : COLOR;
};

struct VSoutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

VSoutput VSmain(VSinput input)
{
	VSoutput output = (VSoutput) 0;

	output.position = float4(input.position, 1);
	output.color = input.color;

	return output;
}