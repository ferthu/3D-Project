#define blurRadius 50
#define threadsPerGroup 256
#define cacheSize (threadsPerGroup + 2 * blurRadius)

cbuffer weights : register(b0)
{
	float4 blurWeights[blurRadius * 2 + 1]; // weights stored in x-component
};

Texture2D input;
RWTexture2D<float4> output;

groupshared float4 cache[cacheSize];

[numthreads(threadsPerGroup, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchID : SV_DispatchThreadID)
{
	// cache (blurRadius) pixels left of group
	if (groupThreadID.x < blurRadius)
	{
		// clamp to within image
		int xToRead = max(dispatchID.x - blurRadius, 0);

		// copy to cache
		cache[groupThreadID.x] = input[int2(xToRead, dispatchID.y)];
	}

	// cache (blurRadius) pixels right of group
	if (groupThreadID.x >= threadsPerGroup - blurRadius)
	{
		// clamp to within image
		int xToRead = min(dispatchID.x + blurRadius, input.Length.x - 1);

		// copy to cache
		cache[groupThreadID.x + 2 * blurRadius] = input[int2(xToRead, dispatchID.y)];
	}

	// cache (threadsPerGroup) pixels within group
	cache[groupThreadID.x + blurRadius] = input[min(dispatchID.xy, input.Length.xy - 1)];

	GroupMemoryBarrierWithGroupSync();

	// calculate blurred color
	float4 blurredColor = float4(0, 0, 0, 0);

	[unroll]
	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		blurredColor += cache[(groupThreadID.x + blurRadius) + i] * blurWeights[i + blurRadius].x;
	}

	output[dispatchID.xy] = blurredColor;
}