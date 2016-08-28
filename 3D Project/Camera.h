#ifndef CAMERA_H
#define CAMERA_H

#include <DirectXMath.h>
#include <d3d11.h>

using namespace DirectX;

class Camera
{
public:
	XMMATRIX& GetViewMatrix();
	void SetViewMatrix(ID3D11DeviceContext* deviceContext, FXMMATRIX worldMatrix);

	XMMATRIX& GetProjectionMatrix();
	void SetProjectionMatrix(ID3D11DeviceContext* deviceContext, FXMMATRIX worldMatrix);

	XMVECTOR& GetPosition();
	void SetPosition(ID3D11DeviceContext* deviceContext, FXMVECTOR position);

	XMMATRIX CreateViewMatrix(FXMVECTOR position, float pitch, float yaw);
	XMMATRIX CreateProjectionMatrix(float horizontalFOV, float nearPlaneDistance, float farPlaneDistance);

	ID3D11Buffer* viewMatrixBuffer;
	ID3D11Buffer* projectionMatrixBuffer;
	ID3D11Buffer* cameraPositionBuffer;

	float horizontalFOV;
	float nearPlaneDistance;
	float farPlaneDistance;
	float pitch;
	float yaw;

	static Camera* CreateCamera(ID3D11Device* device, ID3D11DeviceContext* deviceContext, float horizontalFOV, float nearPlaneDistance, float farPlaneDistance, FXMVECTOR position, float pitch, float yaw, float pitchUpperLimit, float pitchLowerLimit, float aspectRatio);
	~Camera();

private:
	Camera(ID3D11Device* device, ID3D11DeviceContext* deviceContext, float horizontalFOV, float nearPlaneDistance, float farPlaneDistance, FXMVECTOR position, float pitch, float yaw, float pitchUpperLimit, float pitchLowerLimit, float aspectRatio);

	float aspectRatio;
	float pitchUpperLimit;
	float pitchLowerLimit;

	XMFLOAT3 position;

	XMFLOAT4X4 viewMatrix;
	XMFLOAT4X4 projectionMatrix;
};

#endif