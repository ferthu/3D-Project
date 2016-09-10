#ifndef AABB_H
#define AABB_H

#include <d3d11.h>
#include <DirectXMath.h>

#include "Shapes.h"

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

	void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize)
	{
		if (object != nullptr)
		{
			object->Render(deviceContext, vertexSize);
		}
		else
		{
			nxpz->Render(deviceContext, vertexSize);
			pxpz->Render(deviceContext, vertexSize);
			nxnz->Render(deviceContext, vertexSize);
			pxnz->Render(deviceContext, vertexSize);
		}
	}

	AABB(XMFLOAT3 maxCorner, XMFLOAT3 minCorner)
	{
		this->maxCorner = maxCorner;
		this->minCorner = minCorner;
		nxpz = nullptr;
		pxpz = nullptr;
		nxnz = nullptr;
		pxnz = nullptr;
		object = nullptr;
	}

	~AABB()
	{
		delete object;
		delete nxpz;
		delete pxpz;
		delete nxnz;
		delete pxnz;
	}
};

#endif