#ifndef VERTEXSTRUCTUREDEFINITIONS_H
#define VERTEXSTRUCTUREDEFINITIONS_H

#include <DirectXMath.h>

using namespace DirectX;

struct Vertex
{
	XMFLOAT3 position;

	Vertex(XMFLOAT3 pos)
	{
		position = pos;
	}
};

struct ColorVertex : Vertex
{
	XMFLOAT3 color;

	ColorVertex(XMFLOAT3 pos, XMFLOAT3 col) : Vertex(pos)
	{
		color = col;
	}
};

#endif