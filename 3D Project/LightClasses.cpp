#include "LightClasses.h"

using namespace DirectX;

DirectX::XMFLOAT4 Light::getPosition()
{
	return position;
}

void Light::setPosition(DirectX::XMFLOAT4 pos)
{
	position = pos;

	updateWorldMatrix();
}

DirectX::XMFLOAT4 Light::getColor()
{
	return color;
}

void Light::setColor(DirectX::XMFLOAT4 col)
{
	color = col;

	updateBuffer();
}

DirectX::XMFLOAT4 Light::getSpecularColor()
{
	return color;
}

void Light::setSpecularColor(DirectX::XMFLOAT4 specCol)
{
	specularColor = specCol;

	updateBuffer();
}

Light::Light(ID3D11Device* device, ID3D11DeviceContext* deviceContext, DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 col, DirectX::XMFLOAT4 specCol)
{
	position = pos;
	color = col;
	specularColor = specCol;

	this->deviceContext = deviceContext;
	this->device = device;
}

void Light::Initialize()
{
	// world matrix buffer
	D3D11_BUFFER_DESC wdesc;
	wdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	wdesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
	wdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	wdesc.MiscFlags = 0;
	wdesc.StructureByteStride = 0;
	wdesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA wdata;
	wdata.pSysMem = &worldMatrix;
	wdata.SysMemPitch = 0;
	wdata.SysMemSlicePitch = 0;

	device->CreateBuffer(&wdesc, &wdata, &worldMatrixBuffer);

	// data buffer
	D3D11_BUFFER_DESC desc;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.ByteWidth = getBufferSize();
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = prepareBufferData();
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	device->CreateBuffer(&desc, &data, &dataBuffer);

	delete[] data.pSysMem;

	updateWorldMatrix();
	updateBuffer();
}

Light::~Light()
{
	//dataBuffer->Release();
	//worldMatrixBuffer->Release();
}

void Light::updateBuffer()
{
	D3D11_MAPPED_SUBRESOURCE resource;
	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	deviceContext->Map(dataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	memcpy(resource.pData, prepareBufferData(), getBufferSize());

	deviceContext->Unmap(dataBuffer, 0);
}

void DirectionalLight::updateWorldMatrix()
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());

	XMFLOAT4X4 transposed;
	XMStoreFloat4x4(&transposed, XMMatrixTranspose(XMLoadFloat4x4(&worldMatrix)));

	D3D11_MAPPED_SUBRESOURCE resource;
	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	deviceContext->Map(worldMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	memcpy(resource.pData, &transposed, getBufferSize());

	deviceContext->Unmap(worldMatrixBuffer, 0);
}

XMFLOAT4 DirectionalLight::getDirection()
{
	return direction;
}
void DirectionalLight::setDirection(XMFLOAT4 dir)
{
	XMStoreFloat4(&direction, XMVector4Normalize(XMLoadFloat4(&dir)));

	updateBuffer();
}

DirectionalLight::DirectionalLight(ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMFLOAT4 pos, XMFLOAT4 col, XMFLOAT4 specCol, XMFLOAT4 dir, Model* lightModel) : Light(device, deviceContext, pos, col, specCol)
{
	this->lightModel = lightModel;
	XMStoreFloat4(&direction, XMVector4Normalize(XMLoadFloat4(&dir)));
}
DirectionalLight::~DirectionalLight()
{
	//Light::~Light();
	dataBuffer->Release();
	worldMatrixBuffer->Release();
}
void* DirectionalLight::prepareBufferData()
{
	XMMATRIX transposedMatrix = XMMatrixTranspose(XMLoadFloat4x4(&worldMatrix));

	char* bd = new char[getBufferSize()];

	memcpy(bd, &position, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4), &color, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4) * 2, &specularColor, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4) * 3, &direction, sizeof(XMFLOAT4));

	return bd;
}
int DirectionalLight::getBufferSize()
{
	return sizeof(XMFLOAT4) * 4;
}


void SpotLight::updateWorldMatrix()
{
	XMFLOAT3 forwardVector = XMFLOAT3(0, 0, 1);
	XMFLOAT3 dir = XMFLOAT3(direction.x, direction.y, -direction.z); // -z because of angle
	XMFLOAT3 angle;
	XMFLOAT3 cross;
	XMStoreFloat3(&angle, XMVector3AngleBetweenVectors(XMLoadFloat3(&forwardVector), XMLoadFloat3(&dir)));
	XMStoreFloat3(&cross, XMVector3Cross(XMLoadFloat3(&dir), XMLoadFloat3(&forwardVector)));

	XMStoreFloat4x4(&rotationMatrix, XMMatrixRotationAxis(XMLoadFloat3(&cross), angle.x));

	XMStoreFloat4x4(&worldMatrix, XMMatrixMultiply(XMMatrixScaling(coneSize, coneSize, range), XMLoadFloat4x4(&rotationMatrix)));
	XMStoreFloat4x4(&worldMatrix, XMMatrixMultiply(XMLoadFloat4x4(&worldMatrix), XMMatrixTranslationFromVector(XMLoadFloat4(&position))));
	
	XMFLOAT4X4 transposed;
	XMStoreFloat4x4(&transposed, XMMatrixTranspose(XMLoadFloat4x4(&worldMatrix)));

	D3D11_MAPPED_SUBRESOURCE resource;
	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	deviceContext->Map(worldMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	memcpy(resource.pData, &transposed, sizeof(XMFLOAT4X4));

	deviceContext->Unmap(worldMatrixBuffer, 0);
}

XMFLOAT4 SpotLight::getDirection()
{
	return direction;
}
void SpotLight::setDirection(XMFLOAT4 dir)
{
	XMStoreFloat4(&direction, XMVector4Normalize(XMLoadFloat4(&dir)));

	updateBuffer();
	updateWorldMatrix();
}
float SpotLight::getRange()
{
	return range;
}
void SpotLight::setRange(float range)
{
	this->range = range;

	updateBuffer();
}

float SpotLight::getConeSize()
{
	return coneSize;
}
void SpotLight::setConeSize(float coneSize)
{
	this->coneSize = coneSize;

	updateWorldMatrix();
}

SpotLight::SpotLight(ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMFLOAT4 pos, XMFLOAT4 col, XMFLOAT4 specCol, XMFLOAT4 dir, float range, float coneSize, Model* lightModel) : Light(device, deviceContext, pos, col, specCol)
{
	XMStoreFloat4(&direction, XMVector4Normalize(XMLoadFloat4(&dir)));
	this->range = range;
	this->coneSize = coneSize;
	this->lightModel = lightModel;
}
SpotLight::~SpotLight()
{
	//Light::~Light();
	dataBuffer->Release();
	worldMatrixBuffer->Release();
}
void* SpotLight::prepareBufferData()
{
	char* bd = new char[getBufferSize()];

	XMFLOAT3 edgeDotValues;
	XMFLOAT3 forward = XMFLOAT3(0, 0, 1);
	XMFLOAT3 edge = XMFLOAT3(1.0f / sqrtf(2.0f), 0, 1.0f / sqrtf(2.0f));
	XMMATRIX scaling = XMMatrixScaling(coneSize, coneSize, range);
	XMStoreFloat3(&edge, XMVector3Transform(XMLoadFloat3(&edge), scaling));
	XMStoreFloat3(&edgeDotValues, XMVector3Dot(XMLoadFloat3(&forward), XMVector3Normalize(XMLoadFloat3(&edge))));
	float edgeDotValue = edgeDotValues.x;

	XMFLOAT4 posWS;
	XMStoreFloat4(&posWS, XMLoadFloat4(&position)); 

	memcpy(bd										, &posWS, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4)					, &color, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4) * 2				, &specularColor, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4) * 3				, &direction, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4) * 4				, &range, sizeof(float));
	memcpy(bd + sizeof(XMFLOAT4) * 4 + sizeof(float), &edgeDotValue, sizeof(float));

	return bd;
}
int SpotLight::getBufferSize()
{													 // filler
	return sizeof(XMFLOAT4) * 4 + sizeof(float) * 2 + 8;
}


void PointLight::updateWorldMatrix()
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity() * XMMatrixScaling(range, range, range) *XMMatrixTranslationFromVector(XMLoadFloat4(&position))); // flickering error

	XMFLOAT4X4 transposed;
	XMStoreFloat4x4(&transposed, XMMatrixTranspose(XMLoadFloat4x4(&worldMatrix)));

	D3D11_MAPPED_SUBRESOURCE resource;
	ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	deviceContext->Map(worldMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	memcpy(resource.pData, &transposed, sizeof(XMFLOAT4X4));

	deviceContext->Unmap(worldMatrixBuffer, 0);
}
float PointLight::getRange()
{
	return range;
}
void PointLight::setRange(float range)
{
	this->range = range;

	updateBuffer();
	updateWorldMatrix();
}

PointLight::PointLight(ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMFLOAT4 pos, XMFLOAT4 col, XMFLOAT4 specCol, float range, Model* lightModel) : Light(device, deviceContext, pos, col, specCol)
{
	this->range = range;
	this->lightModel = lightModel;
}
PointLight::~PointLight()
{
	//Light::~Light();
	dataBuffer->Release();
	worldMatrixBuffer->Release();
}
void* PointLight::prepareBufferData()
{
	char* bd = new char[getBufferSize()];
	//char filler[12] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l' };

	memcpy(bd						, &position, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4)	, &color, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4) * 2, &specularColor, sizeof(XMFLOAT4));
	memcpy(bd + sizeof(XMFLOAT4) * 3, &range, sizeof(float));
	//memcpy(bd + sizeof(XMFLOAT4) * 2 + sizeof(float), &filler, sizeof(float));

	return bd;
}
int PointLight::getBufferSize()
{												  // filler
	return sizeof(XMFLOAT4) * 3 + sizeof(float) + 12;
}

void DirectionalLight::Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize)
{
	// set index and vertex buffers
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &(lightModel->vertexBuffer), vertexSize, &offset);
	deviceContext->IASetIndexBuffer(lightModel->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	deviceContext->GSSetConstantBuffers(0, 1, &worldMatrixBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &dataBuffer);

	// render
	deviceContext->DrawIndexed(lightModel->numIndices, 0, 0);
}

void PointLight::Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize)
{
	// set index and vertex buffers
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &(lightModel->vertexBuffer), vertexSize, &offset);
	deviceContext->IASetIndexBuffer(lightModel->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	
	deviceContext->GSSetConstantBuffers(0, 1, &worldMatrixBuffer);	
	deviceContext->PSSetConstantBuffers(2, 1, &worldMatrixBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &dataBuffer);		

	// render
	deviceContext->DrawIndexed(lightModel->numIndices, 0, 0);
}

void SpotLight::Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize)
{
	// set index and vertex buffers
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &(lightModel->vertexBuffer), vertexSize, &offset);
	deviceContext->IASetIndexBuffer(lightModel->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	deviceContext->GSSetConstantBuffers(0, 1, &worldMatrixBuffer);
	deviceContext->PSSetConstantBuffers(2, 1, &worldMatrixBuffer);
	deviceContext->PSSetConstantBuffers(0, 1, &dataBuffer);

	// render
	deviceContext->DrawIndexed(lightModel->numIndices, 0, 0);
}