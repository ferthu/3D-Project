struct GSoutput
{
	float4 position : SV_POSITION;
};

Texture2D colorBuffer : register(t0);
Texture2D positionBuffer : register(t1);
Texture2D normalBuffer : register(t2);
Texture2D shadowMap : register(t3);

cbuffer data
{
	float4 position;
	float4 color;
	float4 specularColor;
	float4 direction;
};

cbuffer ambientCol : register(b1)
{
	float4 ambientColor;
}

cbuffer camera : register(b3)
{
	float3 cameraPosition;
}

cbuffer lightView : register(b4)
{
	float4x4 lightViewMatrix;
}

cbuffer lightProjection : register(b5)
{
	float4x4 lightProjectionMatrix;
}

SamplerComparisonState shadowSampler : register(s0);

#define shadowMapSize 1024.0f


float4 main(GSoutput input) : SV_Target
{
	int3 samplePos = int3(input.position.xy, 0);

	float4 readColor = colorBuffer.Load(samplePos);
	float4 readPosition = positionBuffer.Load(samplePos);
	float4 readNormal = normalBuffer.Load(samplePos) * 2.0f - 1.0f;

	float4 shadowPos = mul(float4(readPosition.xyz, 1), lightViewMatrix);
	shadowPos = mul(shadowPos, lightProjectionMatrix);

	float2 shadowSamplePos = float2(0.5f * shadowPos.x + 0.5f, -0.5f * shadowPos.y + 0.5f);

	float dxy = 1.0f / shadowMapSize;

	float s0 = shadowMap.SampleCmp(shadowSampler, shadowSamplePos, shadowPos.z);
	float s1 = shadowMap.SampleCmp(shadowSampler, shadowSamplePos + float2(dxy, 0), shadowPos.z);
	float s2 = shadowMap.SampleCmp(shadowSampler, shadowSamplePos + float2(0, dxy), shadowPos.z);
	float s3 = shadowMap.SampleCmp(shadowSampler, shadowSamplePos + float2(dxy, dxy), shadowPos.z);

	float2 texPos = shadowSamplePos * shadowMapSize;
	float2 fracs = frac(texPos);

	float shadowRatio = lerp(lerp(s0, s1, fracs.x), lerp(s2, s3, fracs.x), fracs.y);

	// specular
	float4 specularLight = float4(0, 0, 0, 0);

	float3 reflectionVector = reflect(direction, readNormal);

	[flatten]
	if (dot(readNormal, direction) < 0.0f)
	{
		float specularStrength = pow(max(0.0f, dot(reflectionVector, normalize(cameraPosition - readPosition.xyz))), readColor.w * 255.0f);

		specularLight = specularStrength * specularColor * readColor;
	}

	//return readPosition;
	//return shadowPos;
	//return float4(shadowRatio, shadowRatio, shadowRatio, 1);

	return (ambientColor * readColor) + (max(0.0f, dot(-direction, readNormal)) * color * readColor + specularLight) * shadowRatio;
}