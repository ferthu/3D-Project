#include "Model.h"

Model* Model::CreateModel(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices, UINT vertexSize, Texture* texture)
{
	return new Model(device, vertexData, numVertices, indexData, numIndices, vertexSize, texture);
}

Model::Model(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices, UINT vertexSize, Texture* texture)
{
	this->numIndices = numIndices;
	this->numVertices = numVertices;
	this->texture = texture;

	// create description of vertex buffer
	D3D11_BUFFER_DESC vertexBufferDescription;
	vertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDescription.ByteWidth = vertexSize * numVertices;
	vertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDescription.CPUAccessFlags = 0;
	vertexBufferDescription.MiscFlags = 0;
	vertexBufferDescription.StructureByteStride = 0;

	// create vertex data
	D3D11_SUBRESOURCE_DATA vertexBufferData;
	vertexBufferData.pSysMem = vertexData;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	// create vertex buffer
	device->CreateBuffer(&vertexBufferDescription, &vertexBufferData, &vertexBuffer);

	// create description of index buffer
	D3D11_BUFFER_DESC indexBufferDescription;
	indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDescription.ByteWidth = sizeof(UINT) * numIndices;
	indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDescription.CPUAccessFlags = 0;
	indexBufferDescription.MiscFlags = 0;
	indexBufferDescription.StructureByteStride = 0;

	// create index data
	D3D11_SUBRESOURCE_DATA indexBufferData;
	indexBufferData.pSysMem = indexData;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;

	// create index buffer
	device->CreateBuffer(&indexBufferDescription, &indexBufferData, &indexBuffer);
}

Model::~Model()
{
	if (vertexBuffer != nullptr)
	{
		vertexBuffer->Release();
	}

	if (indexBuffer != nullptr)
	{
		indexBuffer->Release();
	}
}