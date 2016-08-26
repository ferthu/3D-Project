#ifndef VERTEXSTRUCTUREDEFINITIONS_H
#define VERTEXSTRUCTUREDEFINITIONS_H

#include <DirectXMath.h>

using namespace DirectX;

struct Vertex
{
	XMFLOAT3 position;

	Vertex()
	{
		position = XMFLOAT3(0, 0, 0);
	}

	Vertex(XMFLOAT3 pos)
	{
		position = pos;
	}
};

struct ColorVertex : Vertex
{
	XMFLOAT3 color;

	ColorVertex()
	{
		color = XMFLOAT3(0, 0, 0);
	}

	ColorVertex(XMFLOAT3 pos, XMFLOAT3 col) : Vertex(pos)
	{
		color = col;
	}
};

struct NormalUVVertex : Vertex
{
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT3 UV;

	NormalUVVertex() : Vertex()
	{
		normal = XMFLOAT3(0, 0, 0);
		tangent = XMFLOAT3(0, 0, 0);
		UV = XMFLOAT3(0, 0, 0);
	}

	NormalUVVertex(XMFLOAT3 pos, XMFLOAT3 n, XMFLOAT3 t, XMFLOAT3 uv) : Vertex(pos)
	{
		normal = n;
		tangent = t;
		UV = uv;
	}
};

#endif