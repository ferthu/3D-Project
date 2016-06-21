struct VSinput
{
	float3 position : POSITION;
};

struct VSoutput
{
	float4 position : SV_POSITION;
};

VSoutput main(VSinput input)
{
	VSoutput output = (VSoutput)0;

	output.position = float4(input.position, 1.0f);

	return output;
}