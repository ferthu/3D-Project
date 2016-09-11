#ifndef SHAPES_H
#define SHAPES_H

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

struct Plane
{
	XMFLOAT3 normal;
	float distance;
};

struct Sphere
{
	XMFLOAT3 center;
	float radius;
};

#endif