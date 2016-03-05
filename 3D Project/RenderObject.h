#ifndef RENDEROBJECT_H
#define RENDEROBJECT_H

#include <DirectXMath.h>
#include "VertexStructureDefinitions.h"
#include "Model.h"

using namespace DirectX;

class RenderObject
{
public:
	void Update();
	void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize);
	~RenderObject();

	static RenderObject* CreateRenderObject(ID3D11Device* device, Model* model);

	XMFLOAT4X4 worldMatrix;

private:
	UINT32 vertexBufferOffset;

	Model* model;

	RenderObject(ID3D11Device* device, Model* model);
};

#endif