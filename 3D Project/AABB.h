#ifndef AABB_H
#define AABB_H

#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

#include "Shapes.h"
#include "RenderObject.h"

using namespace DirectX;

class AABB
{
public:
	XMFLOAT3 maxCorner;
	XMFLOAT3 minCorner;
	AABB* nxpz;
	AABB* pxpz;
	AABB* nxnz;
	AABB* pxnz;
	RenderObject* object;

	void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize, Plane* cameraPlanes, float sphereRadius);
	bool AABBPlaneIntersection(XMFLOAT3 maxCorner, XMFLOAT3 minCorner, Plane plane);
	bool AABBSphereIntersection(Plane plane, Sphere& sphere);

	AABB(XMFLOAT3 maxCorner, XMFLOAT3 minCorner);

	~AABB();
};

#endif