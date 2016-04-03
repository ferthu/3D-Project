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

	// set constant buffers
	deviceContext->VSSetConstantBuffers(0, 1, &worldMatrixBuffer);		// slot 0 world matrix
	deviceContext->GSSetConstantBuffers(0, 1, &worldMatrixBuffer);		// slot 0 world matrix
	deviceContext->PSSetShaderResources(0, 1, &(model->texture->textureView));

	// render
	deviceContext->DrawIndexed(model->numIndices, 0, 0);
}

RenderObject::RenderObject(ID3D11Device* device, Model* model)
{
	this->model = model;
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	vertexBufferOffset = 0;

	// create description of world matrix buffer
	D3D11_BUFFER_DESC worldMatrixBufferDescription;
	worldMatrixBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	worldMatrixBufferDescription.ByteWidth = sizeof(worldMatrix);
	worldMatrixBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	worldMatrixBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	worldMatrixBufferDescription.MiscFlags = 0;
	worldMatrixBufferDescription.StructureByteStride = 0;

	// create world matrix buffer
	D3D11_SUBRESOURCE_DATA worldMatrixBufferData;
	worldMatrixBufferData.pSysMem = &worldMatrix;
	worldMatrixBufferData.SysMemPitch = 0;
	worldMatrixBufferData.SysMemSlicePitch = 0;

	// create world matrix buffer
	device->CreateBuffer(&worldMatrixBufferDescription, &worldMatrixBufferData, &worldMatrixBuffer);
}

RenderObject* RenderObject::CreateRenderObject(ID3D11Device* device, Model* model)
{
	return new RenderObject(device, model);
}

RenderObject::~RenderObject()
{
	worldMatrixBuffer->Release();
}

XMMATRIX& RenderObject::GetWorldMatrix()
{
	return XMLoadFloat4x4(&worldMatrix);
}

void RenderObject::SetWorldMatrix(ID3D11DeviceContext* deviceContext, FXMMATRIX worldMatrix)
{
	XMStoreFloat4x4(&(this->worldMatrix), worldMatrix);

	D3D11_MAPPED_SUBRESOURCE resource;
	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	deviceContext->Map(worldMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	XMMATRIX transposed = XMMatrixTranspose(XMLoadFloat4x4(&(this->worldMatrix)));		// transpose because HLSL expects column major

	memcpy(resource.pData, &(transposed), sizeof(transposed));

	deviceContext->Unmap(worldMatrixBuffer, 0);
}