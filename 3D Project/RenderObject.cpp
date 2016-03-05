#include <d3d11.h>
#include "RenderObject.h"


void RenderObject::Update()
{

}
void RenderObject::Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize)
{
	// set index and vertex buffers
	deviceContext->IASetVertexBuffers(0, 1, &(model->vertexBuffer), vertexSize, &vertexBufferOffset);
	deviceContext->IASetIndexBuffer(model->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// render
	deviceContext->DrawIndexed(model->numIndices, 0, 0);
}

RenderObject::RenderObject(ID3D11Device* device, Model* model)
{
	this->model = model;
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	vertexBufferOffset = 0;
}

RenderObject* RenderObject::CreateRenderObject(ID3D11Device* device, Model* model)
{
	return new RenderObject(device, model);
}

RenderObject::~RenderObject()
{
	
}