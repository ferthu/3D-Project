#ifndef RENDEROBJECT_H
#define RENDEROBJECT_H

#include <DirectXMath.h>
#include "VertexStructureDefinitions.h"

using namespace DirectX;

class RenderObject
{
public:
	void Update();
	void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize);
	~RenderObject();

	static RenderObject* CreateRenderObject(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices, UINT vertexSize);

	XMFLOAT4X4 worldMatrix;

private:
	UINT32 vertexBufferOffset;

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	UINT numIndices;

	RenderObject(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices, UINT vertexSize);
};

#endif