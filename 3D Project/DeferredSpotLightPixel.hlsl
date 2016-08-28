struct GSoutput
{
	float4 position : SV_POSITION;
};

Texture2D colorBuffer : register(t0);
Texture2D positionBuffer : register(t1);
Texture2D normalBuffer : register(t2);

cbuffer data
{
	float4 positionWS;
	float4 color;
	float4 specularColor;
	float4 directionWS;
	float range;
	float edgeDotValue;
};

cbuffer ambientCol : register(b1)
{
	float4 ambientColor;
}

cbuffer world : register(b2)
{
	float4x4 worldMatrix;
}

cbuffer camera : register(b3)
{
	float3 cameraPosition;
}


float4 main(GSoutput input) : SV_Target
{
	int3 samplePos = int3(input.position.xy, 0);

	float4 readColor = colorBuffer.Load(samplePos);
	float4 readPosition = positionBuffer.Load(samplePos); 
	float4 readNormal = normalBuffer.Load(samplePos) * 2.0f - 1.0f;

	float4 lightToSurface = readPosition - positionWS;

	float angleModifier = max(0.0f, dot(directionWS.xyz, normalize(lightToSurface.xyz)) - edgeDotValue) / (1 - edgeDotValue);
	angleModifier = pow(angleModifier, 2.0f);

	float rangeAttenuation = 1 - clamp((length(lightToSurface.xyz) / range), 0.0f, 1.0f);

	float normalAttenuation = max(0.0f, dot(-directionWS.xyz, readNormal.xyz));

	// specular
	float4 specularLight = float4(0, 0, 0, 0);

	float4 direction = normalize(float4(readPosition.xyz - positionWS.xyz, 0));

	[flatten]
	if (dot(readNormal, direction) < 0.0f)
	{
		float3 reflectionVector = reflect(direction, readNormal);

		float specularStrength = pow(max(0.0f, dot(reflectionVector, normalize(cameraPosition - readPosition.xyz))), readColor.w * 255.0f);

		specularLight = specularStrength * specularColor * readColor;
	}

	return (readColor * color * normalAttenuation + specularLight) * angleModifier * rangeAttenuation;
}