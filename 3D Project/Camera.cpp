#include "Camera.h"
#include <math.h>

XMMATRIX& Camera::GetViewMatrix()
{
	return XMLoadFloat4x4(&viewMatrix);
}

void Camera::SetViewMatrix(ID3D11DeviceContext* deviceContext, FXMMATRIX viewMatrix)
{
	XMStoreFloat4x4(&(this->viewMatrix), viewMatrix);

	deviceContext->UpdateSubresource(viewMatrixBuffer, 0, NULL, &(this->viewMatrix), sizeof(this->viewMatrix), sizeof(this->viewMatrix));
}

XMMATRIX& Camera::GetProjectionMatrix()
{
	return XMLoadFloat4x4(&projectionMatrix);
}

void Camera::SetProjectionMatrix(ID3D11DeviceContext* deviceContext, FXMMATRIX projectionMatrix)
{
	XMStoreFloat4x4(&(this->projectionMatrix), projectionMatrix);

	deviceContext->UpdateSubresource(projectionMatrixBuffer, 0, NULL, &(this->projectionMatrix), sizeof(this->projectionMatrix), sizeof(this->projectionMatrix));
}

Camera* Camera::CreateCamera(ID3D11Device* device, float horizontalFOV, float nearPlaneDistance, float farPlaneDistance, FXMVECTOR position, float pitch, float yaw, float pitchUpperLimit, float pitchLowerLimit, float aspectRatio)
{
	return new Camera(device, horizontalFOV, nearPlaneDistance, farPlaneDistance, position, pitch, yaw, pitchUpperLimit, pitchLowerLimit, aspectRatio);
}

Camera::Camera(ID3D11Device* device, float horizontalFOV, float nearPlaneDistance, float farPlaneDistance, FXMVECTOR position, float pitch, float yaw, float pitchUpperLimit, float pitchLowerLimit, float aspectRatio)
{
	this->horizontalFOV = horizontalFOV;
	this->nearPlaneDistance = nearPlaneDistance;
	this->farPlaneDistance = farPlaneDistance;
	XMStoreFloat3(&(this->position), position);

	this->pitch = pitch;
	this->yaw = yaw;
	this->pitchUpperLimit = pitchUpperLimit;
	this->pitchLowerLimit = pitchLowerLimit;
	this->aspectRatio = aspectRatio;

	XMStoreFloat4x4(&viewMatrix, CreateViewMatrix(XMLoadFloat3(&(this->position)), pitch, yaw));
	XMStoreFloat4x4(&projectionMatrix, CreateProjectionMatrix(horizontalFOV, nearPlaneDistance, farPlaneDistance));

	// create description of view matrix buffer
	D3D11_BUFFER_DESC viewMatrixBufferDescription;
	viewMatrixBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	viewMatrixBufferDescription.ByteWidth = sizeof(viewMatrix);
	viewMatrixBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	viewMatrixBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	viewMatrixBufferDescription.MiscFlags = 0;
	viewMatrixBufferDescription.StructureByteStride = 0;

	// create view matrix buffer
	D3D11_SUBRESOURCE_DATA viewMatrixBufferData;
	viewMatrixBufferData.pSysMem = &viewMatrix;
	viewMatrixBufferData.SysMemPitch = 0;
	viewMatrixBufferData.SysMemSlicePitch = 0;

	// create view matrix buffer
	device->CreateBuffer(&viewMatrixBufferDescription, &viewMatrixBufferData, &viewMatrixBuffer);

	// create description of projection matrix buffer
	D3D11_BUFFER_DESC projectionMatrixBufferDescription;
	projectionMatrixBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
	projectionMatrixBufferDescription.ByteWidth = sizeof(viewMatrix);
	projectionMatrixBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	projectionMatrixBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	projectionMatrixBufferDescription.MiscFlags = 0;
	projectionMatrixBufferDescription.StructureByteStride = 0;

	// create projection matrix buffer
	D3D11_SUBRESOURCE_DATA projectionMatrixBufferData;
	projectionMatrixBufferData.pSysMem = &viewMatrix;
	projectionMatrixBufferData.SysMemPitch = 0;
	projectionMatrixBufferData.SysMemSlicePitch = 0;

	// create projection matrix buffer
	device->CreateBuffer(&projectionMatrixBufferDescription, &projectionMatrixBufferData, &projectionMatrixBuffer);
}

Camera::~Camera()
{
	viewMatrixBuffer->Release();
	projectionMatrixBuffer->Release();
}

XMMATRIX Camera::CreateViewMatrix(FXMVECTOR position, float pitch, float yaw)
{
	if (yaw > XM_2PI)
		yaw -= XM_2PI;
	if (yaw < 0.0f)
		yaw += XM_2PI;

	if (pitch > pitchUpperLimit)
		pitch = pitchUpperLimit;
	if (pitch < pitchLowerLimit)
		pitch = pitchLowerLimit;

	XMMATRIX view = XMMatrixIdentity();
	XMFLOAT4 pos = XMFLOAT4(-position.m128_f32[0], -position.m128_f32[1], -position.m128_f32[2], 1.0f);
	view.r[3] = XMLoadFloat4(&pos);

	view *= XMMatrixRotationRollPitchYaw(-pitch, -yaw, 0);

	return view;
}

XMMATRIX Camera::CreateProjectionMatrix(float horizontalFOV, float nearPlaneDistance, float farPlaneDistance)
{
	float r = 1 / aspectRatio;
	float angleDivide = 1 / tanf(horizontalFOV * 0.5f);
	float planeDivide = 1 / (farPlaneDistance - nearPlaneDistance);

	XMFLOAT4X4 projection = 
		XMFLOAT4X4(	angleDivide, 0, 0, 0,
					0, angleDivide * r, 0, 0,
					0, 0, farPlaneDistance * planeDivide, 1,
					0, 0, -nearPlaneDistance * farPlaneDistance * planeDivide, 0);

	return XMLoadFloat4x4(&projection);
}
