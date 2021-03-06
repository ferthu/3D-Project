#include "AABB.h"

using namespace DirectX;

void AABB::Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize, Plane* cameraPlanes, float sphereRadius)
{
	if (object != nullptr)
	{
		Sphere objectSphere;
		objectSphere.radius = sphereRadius;
		objectSphere.center = XMFLOAT3(minCorner.x + (maxCorner.x - minCorner.x) / 2.0f, minCorner.y + (maxCorner.y - minCorner.y) / 2.0f, minCorner.z + (maxCorner.z - minCorner.z) / 2.0f);

		bool render = true;
		for (int i = 0; i < 6 && render; i++)
		{
			render = AABBSphereIntersection(cameraPlanes[i], objectSphere);
		}
		if (render)
			object->Render(deviceContext, vertexSize);
	}
	else
	{
		bool render = true;
		for (int i = 0; i < 6 && render; i++)
		{
			render = AABBPlaneIntersection(nxpz->maxCorner, nxpz->minCorner, cameraPlanes[i]);
		}
		if (render)
			nxpz->Render(deviceContext, vertexSize, cameraPlanes, sphereRadius);

		render = true;
		for (int i = 0; i < 6 && render; i++)
		{
			render = AABBPlaneIntersection(pxpz->maxCorner, pxpz->minCorner, cameraPlanes[i]);
		}
		if (render)
			pxpz->Render(deviceContext, vertexSize, cameraPlanes, sphereRadius);

		render = true;
		for (int i = 0; i < 6 && render; i++)
		{
			render = AABBPlaneIntersection(nxnz->maxCorner, nxnz->minCorner, cameraPlanes[i]);
		}
		if (render)
			nxnz->Render(deviceContext, vertexSize, cameraPlanes, sphereRadius);

		render = true;
		for (int i = 0; i < 6 && render; i++)
		{
			render = AABBPlaneIntersection(pxnz->maxCorner, pxnz->minCorner, cameraPlanes[i]);
		}
		if (render)
			pxnz->Render(deviceContext, vertexSize, cameraPlanes, sphereRadius);

	}
}

AABB::AABB(XMFLOAT3 maxCorner, XMFLOAT3 minCorner)
{
	this->maxCorner = maxCorner;
	this->minCorner = minCorner;
	nxpz = nullptr;
	pxpz = nullptr;
	nxnz = nullptr;
	pxnz = nullptr;
	object = nullptr;
}

AABB::~AABB()
{
	delete object;
	delete nxpz;
	delete pxpz;
	delete nxnz;
	delete pxnz;
}

bool AABB::AABBPlaneIntersection(XMFLOAT3 maxCorner, XMFLOAT3 minCorner, Plane plane)
{
	XMFLOAT3 minPoint;

	if (plane.normal.x > 0)
		minPoint.x = minCorner.x;
	else
		minPoint.x = maxCorner.x;
	if (plane.normal.y > 0)
		minPoint.y = minCorner.y;
	else
		minPoint.y = maxCorner.y;
	if (plane.normal.z > 0)
		minPoint.z = minCorner.z;
	else
		minPoint.z = maxCorner.z;

	if (minPoint.x * plane.normal.x + minPoint.y * plane.normal.y + minPoint.z * plane.normal.z > -plane.distance)
		return false;
	return true;

}

bool AABB::AABBSphereIntersection(Plane plane, Sphere& sphere)
{
	if(sphere.center.x * plane.normal.x + sphere.center.y * plane.normal.y + sphere.center.z * plane.normal.z - sphere.radius > -plane.distance)
		return false;
	return true;
}