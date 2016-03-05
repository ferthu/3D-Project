struct GSoutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

[maxvertexcount(3)]
void GSmain(triangle float4 inputPosition[3] : SV_POSITION, triangle float3 inputColor[3] : COLOR, inout TriangleStream<GSoutput> GSout)
{
	float3 normal = cross(inputPosition[1] - inputPosition[0], inputPosition[2] - inputPosition[0]);

	for (int i = 0; i < 3; i++)
	{
		GSoutput element;
		element.position = inputPosition[i];
		element.color = inputColor[i];
		GSout.Append(element);
	}
}