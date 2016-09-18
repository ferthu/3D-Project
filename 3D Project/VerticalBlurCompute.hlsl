#define blurRadius 50
#define threadsPerGroup 256
#define cacheSize (threadsPerGroup + 2 * blurRadius)


cbuffer weights : register(b0)
{
	float4 blurWeights[2 * blurRadius + 1]; // weights stored in x-component
}

Texture2D input;
RWTexture2D<float4> output;

groupshared float4 cache[cacheSize];

[numthreads(1, threadsPerGroup, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchID : SV_DispatchThreadID)
{
	// cache (blurRadius) pixels above group
	if (groupThreadID.y < blurRadius)
	{
		// clamp to within image
		int yToRead = max(dispatchID.y - blurRadius, 0);

		// copy to cache
		cache[groupThreadID.y] = input[int2(dispatchID.x, yToRead)];
	}

	// cache (blurRadius) pixels below group
	if (groupThreadID.y >= threadsPerGroup - blurRadius)
	{
		// clamp to within image
		int yToRead = min(dispatchID.y + blurRadius, input.Length.y - 1);

		// copy to cache
		cache[groupThreadID.y + 2 * blurRadius] = input[int2(dispatchID.x, yToRead)];
	}

	// cache (threadsPerGroup) pixels within group
												// clamp to within image
	cache[groupThreadID.y + blurRadius] = input[min(dispatchID.xy, input.Length.xy - 1)];

	GroupMemoryBarrierWithGroupSync();

	// calculate blurred color
	float4 blurredColor = float4(0, 0, 0, 0);

	[unroll]
	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		blurredColor += cache[(groupThreadID.y + blurRadius) + i] * blurWeights[i + blurRadius].x;
	}

	output[dispatchID.xy] = blurredColor;
}