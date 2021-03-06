#ifndef LIGHTCLASSES_H
#define LIGHTCLASSES_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include "Model.h"
using namespace DirectX;

class Light
{
protected:
	ID3D11DeviceContext* deviceContext;
	ID3D11Device* device;
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4 specularColor;
	XMFLOAT4X4 worldMatrix;

	virtual void updateWorldMatrix() = 0;
	void updateBuffer();
	virtual void* prepareBufferData() = 0;
	virtual int getBufferSize() = 0;

public:
	ID3D11Buffer* dataBuffer;
	ID3D11Buffer* worldMatrixBuffer;
	Model* lightModel;

	DirectX::XMFLOAT4 getPosition();
	void setPosition(DirectX::XMFLOAT4 pos);
	DirectX::XMFLOAT4 getColor();
	void setColor(DirectX::XMFLOAT4 col);
	DirectX::XMFLOAT4 getSpecularColor();
	void setSpecularColor(DirectX::XMFLOAT4 specCol);
	virtual void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize) = 0;
	void Initialize();

	Light(ID3D11Device* device, ID3D11DeviceContext* deviceContext, DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 col, DirectX::XMFLOAT4 specCol);
	virtual ~Light();
};


class DirectionalLight : public Light
{
private:
	DirectX::XMFLOAT4 direction;
	DirectX::XMFLOAT4X4 rotationMatrix;
	float shadowAreaWidth, shadowAreaHeight, shadowAreaNear, shadowAreaFar;

	void updateWorldMatrix();
	void* prepareBufferData();
	int getBufferSize();

public:
	ID3D11Buffer* projectionMatrixBuffer;
	ID3D11Buffer* viewMatrixBuffer;
	ID3D11DepthStencilView* shadowMapDepthView;
	ID3D11ShaderResourceView* shadowMapShaderResourceView;
	UINT shadowMapSize;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;

	DirectX::XMFLOAT4 getDirection();
	void setDirection(DirectX::XMFLOAT4 dir);
	void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize);

	DirectionalLight(ID3D11Device* device, ID3D11DeviceContext* deviceContext, DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 col, DirectX::XMFLOAT4 specCol, DirectX::XMFLOAT4 dir, Model* lightModel, UINT shadowMapSize, float shadowAreaWidth, float shadowAreaHeight, float shadowAreaNear, float shadowAreaFar);
	virtual ~DirectionalLight();
};


class SpotLight : public Light
{
private:
	DirectX::XMFLOAT4 direction;
	DirectX::XMFLOAT4X4 rotationMatrix;
	float range;
	float coneSize;

	void updateWorldMatrix();
	void* prepareBufferData();
	int getBufferSize();

public:
	DirectX::XMFLOAT4 getDirection();
	void setDirection(DirectX::XMFLOAT4 dir);
	float getRange();
	void setRange(float range);
	float getConeSize();
	void setConeSize(float coneSize);
	void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize);

	SpotLight(ID3D11Device* device, ID3D11DeviceContext* deviceContext, DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 col, DirectX::XMFLOAT4 specCol, DirectX::XMFLOAT4 dir, float range, float coneSize, Model* lightModel);
	virtual ~SpotLight();
};


class PointLight : public Light
{
private:
	float range;

	void updateWorldMatrix();
	void* prepareBufferData();
	int getBufferSize();

public:
	float getRange();
	void setRange(float range);
	void Render(ID3D11DeviceContext* deviceContext, UINT* vertexSize);

	PointLight(ID3D11Device* device, ID3D11DeviceContext* deviceContext, DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 col, DirectX::XMFLOAT4 specCol, float range, Model* lightModel);
	virtual ~PointLight();
};

#endif LIGHTCLASSES_H
