#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <cmath>

#include <sstream>
#include <iostream>
#include <fstream>
#include <list>

#include "RenderConfiguration.h"
#include "RenderObject.h"
#include "VertexStructureDefinitions.h"
#include "Texture.h"
#include "LightClasses.h"
#include "Shapes.h"
#include "AABB.h"

using namespace DirectX;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")


// will hold handle to window
HWND windowHandle = 0;

UINT windowWidth = 800;
UINT windowHeight = 600;

const bool DEBUG = true;

UINT msaaSamples = 1;		// minimum 1

D3D_FEATURE_LEVEL* featureLevel = nullptr;

ID3D11Device* device = nullptr;
ID3D11DeviceContext* deviceContext = nullptr;
IDXGISwapChain* swapChain = nullptr;
ID3D11RenderTargetView* backBufferRenderTargetView;
ID3D11ShaderResourceView* backBufferShaderResourceView;
ID3D11UnorderedAccessView* backBufferUnorderedAccessView;
ID3D11DepthStencilView* depthView;

LARGE_INTEGER countFrequency;
LARGE_INTEGER currentTime, previousTime, elapsedTime;
double frameTime;

POINT currentMousePosition;
POINT previousMousePosition;
float mouseSensitivity = 0.003f;

float terrainSize = 20.0f;
float terrainHeight = 5.0f;

Texture* testTexture = nullptr;
Camera* testCam = nullptr;
SpotLight* testSpotLight = nullptr;
PointLight* testPointLight = nullptr;
DirectionalLight* testDirectionalLight = nullptr;

bool useObserverCamera = false;
bool blur = false;
Camera* mainCamera = nullptr;
Camera* observerCamera = nullptr;

AABB* quadTree = nullptr;
Plane cameraPlanes[6];

Model* boxModel = nullptr;
RenderObject* boxObject = nullptr;
Texture* boxTexture = nullptr;
Texture* boxNormalMap = nullptr;

Model* terrainModel = nullptr;
RenderObject* terrainObject = nullptr;
Texture* terrainTexture = nullptr;
Texture* terrainNormalMap = nullptr;
int terrainVertexWidth;
int terrainVertexHeight;
float* heights;
UINT numHeights;

Model* spotLightModel = nullptr;
Model* pointLightModel = nullptr;
Model* directionalLightModel = nullptr;

XMVECTOR ambientLightColor;
ID3D11Buffer* ambientLightColorBuffer;

ID3D11Texture2D* ColorBuffer = nullptr;
ID3D11RenderTargetView* ColorBufferRenderTargetView = nullptr;
ID3D11ShaderResourceView* ColorBufferShaderResourceView = nullptr;

ID3D11Texture2D* PositionBuffer = nullptr;
ID3D11RenderTargetView* PositionBufferRenderTargetView = nullptr;
ID3D11ShaderResourceView* PositionBufferShaderResourceView = nullptr;

ID3D11Texture2D* NormalBuffer = nullptr;
ID3D11RenderTargetView* NormalBufferRenderTargetView = nullptr;
ID3D11ShaderResourceView* NormalBufferShaderResourceView = nullptr;

ID3D11Texture2D* blurredTexture = nullptr;
ID3D11UnorderedAccessView* blurredTextureUnorderedAccessView = nullptr;
ID3D11ShaderResourceView* blurredTextureShaderResourceView = nullptr;

const int blurRadius = 50;
XMFLOAT4 blurWeights[blurRadius * 2 + 1];
ID3D11Buffer* blurWeightsBuffer;

ID3D11InputLayout* deferredGeometryInputLayout = nullptr;
ID3D11InputLayout* deferredLightInputLayout = nullptr;

ID3D11VertexShader* deferredGeometryVertexShader = nullptr;
ID3D11VertexShader* deferredLightVertexShader = nullptr;
ID3D11GeometryShader* deferredGeometryGeometryShader = nullptr;
ID3D11GeometryShader* deferredLightGeometryShader = nullptr;
ID3D11GeometryShader* deferredDirectionalLightGeometryShader = nullptr;
ID3D11PixelShader* deferredGeometryPixelShader = nullptr;
ID3D11PixelShader* deferredDirectionalLightPixelShader = nullptr;
ID3D11PixelShader* deferredSpotLightPixelShader = nullptr;
ID3D11PixelShader* deferredPointLightPixelShader = nullptr;

ID3D11ComputeShader* horizontalBlurShader = nullptr;
ID3D11ComputeShader* verticalBlurShader = nullptr;

ID3D11RasterizerState* deferredGeometryRasterizerState = nullptr;
ID3D11RasterizerState* deferredLightRasterizerState = nullptr;

ID3D11DepthStencilState* deferredGeometryDepthState = nullptr;
ID3D11DepthStencilState* deferredLightDepthState = nullptr;

ID3D11BlendState* deferredGeometryBlendState = nullptr;
ID3D11BlendState* deferredLightBlendState = nullptr;

// declare window procedure function
LRESULT CALLBACK windowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

void initializeD3D();
void Update();
void Render();
void CreateTestInput();
void UpdateFrameTime();
void CreateTestModel();
void SetupDeferredRendering();
void RenderDeferredRendering();
void CreateGBuffer(ID3D11Texture2D** texture, ID3D11RenderTargetView** renderTargetView, ID3D11ShaderResourceView** shaderResourceView);
void LoadOBJ(std::string fileName, NormalUVVertex*& verticesArray, UINT& numVertices, UINT*& indicesArray, UINT& numIndices);
void LoadLightOBJ(std::string fileName, Vertex*& verticesArray, UINT& numVertices, UINT*& indicesArray, UINT& numIndices);
void CreateLightGeometry();
XMFLOAT4 readObjectColor(char* materialFileName);
void CreateTerrain(char* heightMapName, float size, float heightRange, char* textureName, char* normalMapName, int& terrainVertexWidth, int& terrainVertexHeight, float*& heights, UINT& numHeights);
AABB* createQuadTree(XMFLOAT3 maxCorner, XMFLOAT3 minCorner, ID3D11Device* device, Model* objectModel, XMFLOAT4 objectColor, UINT levels);
void getCameraPlanes(Camera* camera);



int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, int showCommand)
{
#pragma region window setup
	// create and register window class
	WNDCLASS windowClass;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	windowClass.hInstance = instance;
	windowClass.lpfnWndProc = windowProc;
	windowClass.lpszClassName = L"windowClass";
	windowClass.lpszMenuName = 0;
	windowClass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&windowClass);

	// create window with registered window class
	windowHandle = CreateWindow(L"windowClass", L"3D Project", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, 0, 0, instance, 0);

	// show and update window
	ShowWindow(windowHandle, SW_SHOW);
	UpdateWindow(windowHandle);
#pragma endregion

	QueryPerformanceFrequency(&countFrequency);
	QueryPerformanceCounter(&currentTime);
	previousTime = currentTime;

	GetCursorPos(&currentMousePosition);
	previousMousePosition = currentMousePosition;
	
	initializeD3D();

	SetupDeferredRendering();

	CreateLightGeometry();

	CreateTestInput();

	// main loop
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);

			// send to window procedure function
			DispatchMessage(&msg);
		}
		// graphics loop
		else
		{
			Update();

			Render();
		}
	}

	// release resources
#pragma region delete
	delete testCam;
	delete testTexture;
	delete mainCamera;
	delete observerCamera;
	delete boxModel;
	delete boxObject;
	delete boxTexture;
	delete boxNormalMap;
	delete spotLightModel;
	delete pointLightModel;
	delete directionalLightModel;
	delete testSpotLight;
	delete testPointLight;
	delete testDirectionalLight;
	delete terrainModel;
	delete terrainObject;
	delete terrainTexture;
	delete terrainNormalMap;
	delete quadTree;

	device->Release();
	deviceContext->Release();
	swapChain->Release();
	backBufferRenderTargetView->Release();
	deferredGeometryRasterizerState->Release();
	deferredLightRasterizerState->Release();
	deferredGeometryDepthState->Release();
	deferredLightDepthState->Release();
	deferredGeometryVertexShader->Release();
	deferredLightVertexShader->Release();
	deferredGeometryGeometryShader->Release();
	deferredLightGeometryShader->Release();
	deferredGeometryPixelShader->Release();
	deferredDirectionalLightPixelShader->Release();
	deferredSpotLightPixelShader->Release();
	deferredPointLightPixelShader->Release();
	ambientLightColorBuffer->Release();
	ColorBuffer->Release();
	ColorBufferRenderTargetView->Release();
	ColorBufferShaderResourceView->Release();
	PositionBuffer->Release();
	PositionBufferRenderTargetView->Release();
	PositionBufferShaderResourceView->Release();
	NormalBuffer->Release();
	NormalBufferRenderTargetView->Release();
	NormalBufferShaderResourceView->Release();
	deferredGeometryInputLayout->Release();
	deferredLightInputLayout->Release();
	deferredGeometryBlendState->Release();
	deferredLightBlendState->Release();
	blurredTexture->Release();
	blurredTextureShaderResourceView->Release();
	blurredTextureUnorderedAccessView->Release();
	backBufferShaderResourceView->Release();
	backBufferUnorderedAccessView->Release();

	delete[] heights;
#pragma endregion

	// returns parameter of WM_QUIT message
	return (int)msg.wParam;
}

LRESULT CALLBACK windowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	// handle messages
	switch (message)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(windowHandle);
			break;
		}
		return 0;

	case WM_KEYUP:
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(windowHandle, message, wParam, lParam);
}

void initializeD3D()
{
	// create device, device context
	D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		DEBUG == true ? D3D11_CREATE_DEVICE_DEBUG : NULL,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&device,
		featureLevel,
		&deviceContext);
	
	// check supported MSAA quality levels and store in msaaQualityLevels
	UINT msaaQualityLevels;
	device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, msaaSamples, &msaaQualityLevels);

	// describe swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc.Width						= windowWidth;
	swapChainDesc.BufferDesc.Height						= windowHeight;
	swapChainDesc.BufferDesc.RefreshRate.Numerator		= 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator	= 1;
	swapChainDesc.BufferDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering			= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling					= DXGI_MODE_SCALING_UNSPECIFIED;

	swapChainDesc.SampleDesc.Count		= msaaSamples;
	swapChainDesc.SampleDesc.Quality	= msaaQualityLevels - 1;

	swapChainDesc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_UNORDERED_ACCESS;
	swapChainDesc.BufferCount	= 1;
	swapChainDesc.OutputWindow	= windowHandle;
	swapChainDesc.Windowed		= true;
	swapChainDesc.SwapEffect	= DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags			= NULL;
	
	// create swap chain
	IDXGIDevice* DXGIDevice = nullptr;
	device->QueryInterface(__uuidof(IDXGIDevice), (void**) &DXGIDevice);
	IDXGIDevice* DXGIAdapter = nullptr;
	DXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**) &DXGIAdapter);
	IDXGIFactory* DXGIFactory = nullptr;
	DXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**) &DXGIFactory);

	DXGIFactory->CreateSwapChain(device, &swapChainDesc, &swapChain);

	DXGIDevice->Release();
	DXGIAdapter->Release();
	DXGIFactory->Release();

	// create render target view for back buffer
	ID3D11Texture2D* backBuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &backBuffer);
	device->CreateRenderTargetView(backBuffer, 0, &backBufferRenderTargetView);
	
	// create shader resource view for back buffer
	D3D11_SHADER_RESOURCE_VIEW_DESC backBufferSRVDesc;
	backBufferSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	backBufferSRVDesc.Texture2D.MipLevels = 1;
	backBufferSRVDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(backBuffer, &backBufferSRVDesc, &backBufferShaderResourceView);

	// create unordered access view for back buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC backBufferUAVDesc;
	backBufferUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	backBufferUAVDesc.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(backBuffer, &backBufferUAVDesc, &backBufferUnorderedAccessView);

	backBuffer->Release();

	// create depth buffer and view
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	depthBufferDesc.Width				= windowWidth;
	depthBufferDesc.Height				= windowHeight;
	depthBufferDesc.MipLevels			= 1;
	depthBufferDesc.ArraySize			= 1;
	depthBufferDesc.Usage				= D3D11_USAGE_DEFAULT;
	depthBufferDesc.CPUAccessFlags		= 0;
	depthBufferDesc.MiscFlags			= 0;
	depthBufferDesc.BindFlags			= D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count	= msaaSamples;
	depthBufferDesc.SampleDesc.Quality	= msaaQualityLevels > 0 ? msaaQualityLevels - 1 : 0;

	ID3D11Texture2D* depthBuffer;
	device->CreateTexture2D(&depthBufferDesc, 0, &depthBuffer);

	device->CreateDepthStencilView(depthBuffer, 0, &depthView);

	// bind to output merger
	deviceContext->OMSetRenderTargets(1, &backBufferRenderTargetView, depthView);

	// create viewport
	D3D11_VIEWPORT viewPort;
	viewPort.Width = (float) windowWidth;
	viewPort.Height = (float) windowHeight;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	deviceContext->RSSetViewports(1, &viewPort);

	// release resources
	depthBuffer->Release();
	depthView->Release();
}

void Update()
{
	UpdateFrameTime();

	GetCursorPos(&currentMousePosition);

	mainCamera->yaw += (currentMousePosition.x - previousMousePosition.x) * mouseSensitivity;
	mainCamera->pitch += (currentMousePosition.y - previousMousePosition.y) * mouseSensitivity;

	XMFLOAT3 ct = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMVECTOR cameraTranslation = XMLoadFloat3(&ct);

	XMFLOAT3 cameraX = XMFLOAT3(cosf(mainCamera->yaw) * frameTime, 0.0f, -sinf(mainCamera->yaw) * frameTime);
	XMFLOAT3 cameraZ = XMFLOAT3(sinf(mainCamera->yaw) * frameTime, 0.0f, cosf(mainCamera->yaw) * frameTime);

	XMFLOAT3 up = XMFLOAT3(0, frameTime, 0);

	if (GetKeyState('W') & 0x8000)	// highest bit set to 1 means key is pressed
		cameraTranslation += XMLoadFloat3(&cameraZ);

	if (GetKeyState('S') & 0x8000)
		cameraTranslation -= XMLoadFloat3(&cameraZ);

	if (GetKeyState('D') & 0x8000)
		cameraTranslation += XMLoadFloat3(&cameraX);

	if (GetKeyState('A') & 0x8000)
		cameraTranslation -= XMLoadFloat3(&cameraX);

	if (GetKeyState('E') & 0x8000)
		cameraTranslation += XMLoadFloat3(&up);

	if (GetKeyState('Q') & 0x8000)
		cameraTranslation -= XMLoadFloat3(&up);

	if (GetKeyState('R') & 0x8000)
		useObserverCamera = true;
	else
		useObserverCamera = false;

	if (GetKeyState('F') & 0x8000)
		blur = true;
	else
		blur = false;

	XMVector3Normalize(cameraTranslation);

	XMVECTOR newPos = mainCamera->GetPosition() + cameraTranslation * 2;

	float cx = XMVectorGetX(newPos);
	float cy = XMVectorGetY(newPos);
	float cz = XMVectorGetZ(newPos);

	if (cx >= terrainSize / 2.0f)
		cx = terrainSize / 2.0f - 0.0001f;
	if (cx < -terrainSize / 2.0f)
		cx = -terrainSize / 2.0f;
	if (cz >= terrainSize / 2.0f)
		cz = terrainSize / 2.0f - 0.0001f;
	if (cz < -terrainSize / 2.0f)
		cz = -terrainSize / 2.0f;

	// find row and column in terrain
	float terrainStepX = terrainSize / (float)(terrainVertexWidth - 1);
	float terrainStepY = terrainSize / (float)(terrainVertexHeight - 1);

	float cameraRowf = (-cz + (terrainSize / 2.0f)) / terrainStepY;
	float cameraColumnf = (cx + (terrainSize / 2.0f)) / terrainStepX;

	int cameraRow = (int)cameraRowf;
	int cameraColumn = (int)cameraColumnf;

	cameraRowf -= cameraRow;
	cameraColumnf -= cameraColumn;

	int sampleIndices[] = { cameraRow * (terrainVertexWidth) + cameraColumn,
							cameraRow * terrainVertexWidth + cameraColumn + 1,
							(cameraRow + 1) * terrainVertexWidth + cameraColumn,
							(cameraRow + 1) * terrainVertexWidth + cameraColumn + 1 };

	float interpolatedHeight = ((heights[sampleIndices[0]] * 2.0f - 1.0f) * (1.0f - cameraColumnf) +
		(heights[sampleIndices[1]] * 2.0f - 1.0f) * (cameraColumnf)) * (1.0f - cameraRowf)
		+
		((heights[sampleIndices[2]] * 2.0f - 1.0f) * (1.0f - cameraColumnf) +
			(heights[sampleIndices[3]] * 2.0f - 1.0f) * (cameraColumnf)) * (cameraRowf);

	cy = interpolatedHeight * terrainHeight + 1.0f;

	newPos = XMVectorSet(cx, cy, cz, XMVectorGetW(newPos));

	mainCamera->SetPosition(deviceContext, newPos);

	mainCamera->SetViewMatrix(deviceContext, mainCamera->CreateViewMatrix(mainCamera->GetPosition(), mainCamera->pitch, mainCamera->yaw));

	getCameraPlanes(mainCamera);


	previousMousePosition = currentMousePosition;
}

void UpdateFrameTime()
{
	QueryPerformanceCounter(&currentTime);

	elapsedTime.QuadPart = currentTime.QuadPart - previousTime.QuadPart;
	frameTime = elapsedTime.QuadPart;
	frameTime /= countFrequency.QuadPart;

	previousTime = currentTime;

	std::wostringstream strs;
	strs << "3D Project | " << frameTime * 1000 << " ms | " << 1.0 / frameTime << " fps";
	SetWindowText(windowHandle, strs.str().c_str());
}

void Render()
{
	// clear window to background color
	float backgroundColor[4];
	float black[] = { 0, 0, 0, 1 };
	
	// clear geometry buffers
	XMFLOAT4 bgc = XMFLOAT4(0.1f, 0.1f, 0.1f, 1);
	backgroundColor[0] = bgc.x;
	backgroundColor[1] = bgc.y;
	backgroundColor[2] = bgc.z;
	backgroundColor[3] = bgc.w;
	deviceContext->ClearRenderTargetView(backBufferRenderTargetView, backgroundColor);
	deviceContext->ClearRenderTargetView(ColorBufferRenderTargetView, black);
	deviceContext->ClearRenderTargetView(PositionBufferRenderTargetView, black);
	deviceContext->ClearRenderTargetView(NormalBufferRenderTargetView, black);

	// clear depth buffer
	deviceContext->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	RenderDeferredRendering();

	// display frame
	swapChain->Present(0, 0);
}

void CreateTestInput()
{
	// create test lights
	testPointLight = new PointLight(device, deviceContext, XMFLOAT4(1.3f, 1.25f, -1.5f, 1), XMFLOAT4(1.0f, 0.0f, 0.0f, 1), XMFLOAT4(1.0f, 0.0f, 0.0f, 1), 30.0f, pointLightModel);
	testPointLight->Initialize();
	testSpotLight = new SpotLight(device, deviceContext, XMFLOAT4(-1.3, 2, -1.3, 1), XMFLOAT4(0, 1, 0, 1), XMFLOAT4(0, 1, 0, 1), XMFLOAT4(1, -1.7, 1, 0), 5, 3, spotLightModel);
	testSpotLight->Initialize();
	testDirectionalLight = new DirectionalLight(device, deviceContext, XMFLOAT4(0, 0, 0, 1), XMFLOAT4(0.0f, 0.0f, 0.1f, 1), XMFLOAT4(0.0f, 0.0f, 0.1f, 1), XMFLOAT4(1, -1, 0.7f, 0), directionalLightModel);
	testDirectionalLight->Initialize();
}

void SetupDeferredRendering()
{
#pragma region input description
	D3D11_INPUT_ELEMENT_DESC normalMappedVertexInputDescription[] =
	{ { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "TEXCOORDS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 } };

	D3D11_INPUT_ELEMENT_DESC lightMeshVertexInputDescription =
	 { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
#pragma endregion

#pragma region vertex shader, input layout
	// create geometry vertex shader
	ID3DBlob* vs = nullptr;

	HRESULT hr = D3DCompileFromFile(
		L"DeferredGeometryVertex.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_4_0",
		0, 0,
		&vs,
		nullptr);

	device->CreateVertexShader(
		vs->GetBufferPointer(),
		vs->GetBufferSize(),
		nullptr,
		&deferredGeometryVertexShader);

	// create geometry input layout
	device->CreateInputLayout(
		normalMappedVertexInputDescription,
		4,
		vs->GetBufferPointer(),
		vs->GetBufferSize(),
		&deferredGeometryInputLayout);

	// create light vertex shader
	hr = D3DCompileFromFile(
		L"DeferredLightVertex.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_4_0",
		0, 0,
		&vs,
		nullptr);

	device->CreateVertexShader(
		vs->GetBufferPointer(),
		vs->GetBufferSize(),
		nullptr,
		&deferredLightVertexShader);

	// create light input layout
	device->CreateInputLayout(
		&lightMeshVertexInputDescription,
		1,
		vs->GetBufferPointer(),
		vs->GetBufferSize(),
		&deferredLightInputLayout);

	vs->Release();
#pragma endregion

#pragma region geometry shader
	ID3DBlob* gs = nullptr;

	D3DCompileFromFile(
		L"DeferredGeometryGeometry.hlsl",
		nullptr,
		nullptr,
		"main",
		"gs_4_0",
		0, 0,
		&gs,
		nullptr);

	device->CreateGeometryShader(
		gs->GetBufferPointer(),
		gs->GetBufferSize(),
		nullptr,
		&deferredGeometryGeometryShader);

	D3DCompileFromFile(
		L"DeferredLightGeometry.hlsl",
		nullptr,
		nullptr,
		"main",
		"gs_4_0",
		0, 0,
		&gs,
		nullptr);

	device->CreateGeometryShader(
		gs->GetBufferPointer(),
		gs->GetBufferSize(),
		nullptr,
		&deferredLightGeometryShader);

	D3DCompileFromFile(
		L"DeferredDirectionalLightGeometry.hlsl",
		nullptr,
		nullptr,
		"main",
		"gs_4_0",
		0, 0,
		&gs,
		nullptr);

	device->CreateGeometryShader(
		gs->GetBufferPointer(),
		gs->GetBufferSize(),
		nullptr,
		&deferredDirectionalLightGeometryShader);

	gs->Release();
#pragma endregion

#pragma region pixel shader
	// create pixel shader
	ID3DBlob* ps = nullptr;

	D3DCompileFromFile(
		L"DeferredGeometryPixel.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_4_0",
		0, 0,
		&ps,
		nullptr);

	device->CreatePixelShader(
		ps->GetBufferPointer(),
		ps->GetBufferSize(),
		nullptr,
		&deferredGeometryPixelShader);

	D3DCompileFromFile(
		L"DeferredDirectionalLightPixel.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_4_0",
		0, 0,
		&ps,
		nullptr);

	device->CreatePixelShader(
		ps->GetBufferPointer(),
		ps->GetBufferSize(),
		nullptr,
		&deferredDirectionalLightPixelShader);

	D3DCompileFromFile(
		L"DeferredSpotLightPixel.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_4_0",
		0, 0,
		&ps,
		nullptr);

	device->CreatePixelShader(
		ps->GetBufferPointer(),
		ps->GetBufferSize(),
		nullptr,
		&deferredSpotLightPixelShader);

	D3DCompileFromFile(
		L"DeferredPointLightPixel.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_4_0",
		0, 0,
		&ps,
		nullptr);

	device->CreatePixelShader(
		ps->GetBufferPointer(),
		ps->GetBufferSize(),
		nullptr,
		&deferredPointLightPixelShader);

	ps->Release();
#pragma endregion

#pragma region gbuffers
	CreateGBuffer(&ColorBuffer, &ColorBufferRenderTargetView, &ColorBufferShaderResourceView);
	CreateGBuffer(&NormalBuffer, &NormalBufferRenderTargetView, &NormalBufferShaderResourceView);
	CreateGBuffer(&PositionBuffer, &PositionBufferRenderTargetView, &PositionBufferShaderResourceView);
#pragma endregion

#pragma region camera
	XMFLOAT3 camPos = XMFLOAT3(-1.5f, 0.0f, 4.5f);
	mainCamera = Camera::CreateCamera(device, deviceContext, XM_PI * 0.5f, 0.1f, 40.0f, XMLoadFloat3(&camPos), 0.0f, 0.0f, XM_PIDIV2 * 1.0f, -XM_PIDIV2 * 1.0f, ((float)windowHeight) / ((float)windowWidth));

	camPos = XMFLOAT3(0.0f, 20.0f, -20.0f);
	observerCamera = Camera::CreateCamera(device, deviceContext, XM_PI * 0.5f, 0.1f, 100.0f, XMLoadFloat3(&camPos), XM_PIDIV4, 0.0f, XM_PIDIV2 * 1.0f, -XM_PIDIV2 * 1.0f, ((float)windowHeight) / ((float)windowWidth));
#pragma endregion

#pragma region geometry
	NormalUVVertex* vertices = nullptr;
	UINT numVertices = 0;
	UINT* indices = nullptr;
	UINT numIndices = 0;

	boxTexture = Texture::CreateTexture(device, "test4.tga");
	boxNormalMap = Texture::CreateTexture(device, "boxNormalMap.tga");

	LoadOBJ("box.obj", vertices, numVertices, indices, numIndices);
	boxModel = Model::CreateModel(device, vertices, numVertices, indices, numIndices, 48, boxTexture, boxNormalMap);

	boxObject = RenderObject::CreateRenderObject(device, boxModel, readObjectColor("box.mtl"));

	delete[] vertices;
	delete[] indices;

	quadTree = createQuadTree(XMFLOAT3(50.0f, 5.0f, 50.0f), XMFLOAT3(-50.0f, 0.0f, -50.0f), device, boxModel, readObjectColor("box.mtl"), 4);

	CreateTerrain("terrain.tga", terrainSize, terrainHeight, "terrainTexture.tga", "terrainNormalMap.tga", terrainVertexWidth, terrainVertexHeight, heights, numHeights);
#pragma endregion

#pragma region ambient light buffer
	XMFLOAT4 col = XMFLOAT4(0.025f, 0.025f, 0.025f, 0.0f);
	ambientLightColor = XMLoadFloat4(&col);

	// create description of ambient light color buffer
	D3D11_BUFFER_DESC ambientLightColorBufferDescription;
	ambientLightColorBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	ambientLightColorBufferDescription.ByteWidth = sizeof(ambientLightColor);
	ambientLightColorBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ambientLightColorBufferDescription.CPUAccessFlags = 0;
	ambientLightColorBufferDescription.MiscFlags = 0;
	ambientLightColorBufferDescription.StructureByteStride = 0;

	// create ambient light color buffer
	D3D11_SUBRESOURCE_DATA ambientLightColorBufferData;
	ambientLightColorBufferData.pSysMem = &ambientLightColor;
	ambientLightColorBufferData.SysMemPitch = 0;
	ambientLightColorBufferData.SysMemSlicePitch = 0;

	// create ambient light color buffer
	device->CreateBuffer(&ambientLightColorBufferDescription, &ambientLightColorBufferData, &ambientLightColorBuffer);
#pragma endregion

#pragma region rasterizer and depth states
	D3D11_RASTERIZER_DESC geometryRasterizerStateDesc;
	geometryRasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
	geometryRasterizerStateDesc.CullMode = D3D11_CULL_BACK;
	geometryRasterizerStateDesc.FrontCounterClockwise = false;
	geometryRasterizerStateDesc.DepthBias = 0;
	geometryRasterizerStateDesc.SlopeScaledDepthBias = 0.0f;
	geometryRasterizerStateDesc.DepthBiasClamp = 0.0f;
	geometryRasterizerStateDesc.DepthClipEnable = true;
	geometryRasterizerStateDesc.ScissorEnable = false;
	geometryRasterizerStateDesc.MultisampleEnable = false;
	geometryRasterizerStateDesc.AntialiasedLineEnable = false;

	device->CreateRasterizerState(&geometryRasterizerStateDesc, &deferredGeometryRasterizerState);

	D3D11_RASTERIZER_DESC lightRasterizerStateDesc;
	lightRasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
	lightRasterizerStateDesc.CullMode = D3D11_CULL_FRONT;
	lightRasterizerStateDesc.FrontCounterClockwise = false;
	lightRasterizerStateDesc.DepthBias = 0;
	lightRasterizerStateDesc.SlopeScaledDepthBias = 0.0f;
	lightRasterizerStateDesc.DepthBiasClamp = 0.0f;
	lightRasterizerStateDesc.DepthClipEnable = true;
	lightRasterizerStateDesc.ScissorEnable = false;
	lightRasterizerStateDesc.MultisampleEnable = false;
	lightRasterizerStateDesc.AntialiasedLineEnable = false;

	device->CreateRasterizerState(&lightRasterizerStateDesc, &deferredLightRasterizerState);

	D3D11_DEPTH_STENCIL_DESC geometryDepthStateDesc;
	geometryDepthStateDesc.DepthEnable = true;
	geometryDepthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	geometryDepthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	geometryDepthStateDesc.StencilEnable = false;
	geometryDepthStateDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	geometryDepthStateDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	geometryDepthStateDesc.FrontFace.StencilFunc = geometryDepthStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	geometryDepthStateDesc.FrontFace.StencilDepthFailOp = geometryDepthStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	geometryDepthStateDesc.FrontFace.StencilPassOp = geometryDepthStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	geometryDepthStateDesc.FrontFace.StencilFailOp = geometryDepthStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

	device->CreateDepthStencilState(&geometryDepthStateDesc, &deferredGeometryDepthState);

	D3D11_DEPTH_STENCIL_DESC lightDepthStateDesc;
	lightDepthStateDesc.DepthEnable = true;
	lightDepthStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	lightDepthStateDesc.DepthFunc = D3D11_COMPARISON_GREATER;
	lightDepthStateDesc.StencilEnable = false;
	lightDepthStateDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	lightDepthStateDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	lightDepthStateDesc.FrontFace.StencilFunc = geometryDepthStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	lightDepthStateDesc.FrontFace.StencilDepthFailOp = geometryDepthStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	lightDepthStateDesc.FrontFace.StencilPassOp = geometryDepthStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	lightDepthStateDesc.FrontFace.StencilFailOp = geometryDepthStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

	device->CreateDepthStencilState(&lightDepthStateDesc, &deferredLightDepthState);
#pragma endregion

#pragma region blend states
	D3D11_BLEND_DESC geometryBlendStateDesc;
	geometryBlendStateDesc.AlphaToCoverageEnable = false;
	geometryBlendStateDesc.IndependentBlendEnable = false;
	for (int i = 0; i < 3; i++)
	{
		geometryBlendStateDesc.RenderTarget[i].BlendEnable = false;
		geometryBlendStateDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		geometryBlendStateDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
		geometryBlendStateDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		geometryBlendStateDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		geometryBlendStateDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		geometryBlendStateDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		geometryBlendStateDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}

	device->CreateBlendState(&geometryBlendStateDesc, &deferredGeometryBlendState);

	D3D11_BLEND_DESC lightBlendStateDesc;
	lightBlendStateDesc.AlphaToCoverageEnable = false;
	lightBlendStateDesc.IndependentBlendEnable = false;
	lightBlendStateDesc.RenderTarget[0].BlendEnable = true;
	lightBlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	lightBlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	lightBlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	lightBlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	lightBlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	lightBlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	lightBlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device->CreateBlendState(&lightBlendStateDesc, &deferredLightBlendState);
#pragma endregion

#pragma region blurred texture
	D3D11_TEXTURE2D_DESC desc;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Height = windowHeight;
	desc.MipLevels = 0;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = windowWidth;

	device->CreateTexture2D(&desc, 0, &blurredTexture);

	D3D11_UNORDERED_ACCESS_VIEW_DESC blurredTextureUAVDesc;
	blurredTextureUAVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	blurredTextureUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	blurredTextureUAVDesc.Texture2D.MipSlice = 0;

	device->CreateUnorderedAccessView(blurredTexture, &blurredTextureUAVDesc, &blurredTextureUnorderedAccessView);

	D3D11_SHADER_RESOURCE_VIEW_DESC shadDesc;
	shadDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	shadDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shadDesc.Texture2D.MipLevels = 1;
	shadDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(blurredTexture, &shadDesc, &blurredTextureShaderResourceView);
#pragma endregion

#pragma region blur weights
	// fill weights array
	float sigma = 50.0f;
	float divisor = 1.0f / (std::sqrtf(2 * sigma * sigma * XM_PI));
	float sum = 0;

	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		blurWeights[i + blurRadius].x = divisor * std::expf(-( (i * i)/(2 * sigma * sigma) ));
		sum += blurWeights[i + blurRadius].x;
	}

	for (int i = 0; i < blurRadius * 2 + 1; i++)
	{
		blurWeights[i].x /= sum;
	}

	// create description of blur weights buffer
	D3D11_BUFFER_DESC blurWeightsBufferDescription;
	blurWeightsBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	blurWeightsBufferDescription.ByteWidth = sizeof(XMFLOAT4) * (blurRadius * 2 + 1);
	blurWeightsBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	blurWeightsBufferDescription.CPUAccessFlags = 0;
	blurWeightsBufferDescription.MiscFlags = 0;
	blurWeightsBufferDescription.StructureByteStride = 0;

	// create blur weights buffer
	D3D11_SUBRESOURCE_DATA blurWeightsBufferData;
	blurWeightsBufferData.pSysMem = &blurWeights;
	blurWeightsBufferData.SysMemPitch = 0;
	blurWeightsBufferData.SysMemSlicePitch = 0;

	// create blur weights buffer
	device->CreateBuffer(&blurWeightsBufferDescription, &blurWeightsBufferData, &blurWeightsBuffer);
#pragma endregion

#pragma region compute shaders
	ID3DBlob* cs = nullptr;

	D3DCompileFromFile(
		L"HorizontalBlurCompute.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		0, 0,
		&cs,
		nullptr);

	device->CreateComputeShader(
		cs->GetBufferPointer(),
		cs->GetBufferSize(),
		nullptr,
		&horizontalBlurShader);

	D3DCompileFromFile(
		L"VerticalBlurCompute.hlsl",
		nullptr,
		nullptr,
		"main",
		"cs_5_0",
		0, 0,
		&cs,
		nullptr);

	device->CreateComputeShader(
		cs->GetBufferPointer(),
		cs->GetBufferSize(),
		nullptr,
		&verticalBlurShader);
#pragma endregion
}

void CreateTerrain(char* heightMapName, float size, float heightRange, char* textureName, char* normalMapName, int& terrainVertexWidth, int& terrainVertexHeight, float*& heights, UINT& numHeights)
{
	// load heightmap
	std::ifstream textureFile;
	textureFile.open(heightMapName, std::ios::binary | std::ios::in);

	typedef struct
	{
		char idLength;
		char colorMapType;
		char imageTypeCode;
		char colorMapSpecification[5];
		char imageOrigin[4];
		short int width;
		short int height;
		char pixelSize;
		char imageDescriptorByte;
	} tgaHeader;
	tgaHeader textureHeader;
	textureFile.read((char*)&textureHeader, 18);

	terrainVertexWidth = textureHeader.width;
	terrainVertexHeight = textureHeader.height;
	numHeights = textureHeader.width * textureHeader.height;

	float* heightData = new float[textureHeader.width * textureHeader.height];
	heights = heightData;

	// read pixel data
	int column = 0;
	int row = textureHeader.height - 1;
	unsigned char readR, readG, readB, readA;
	unsigned char header;
	while (!textureFile.read((char*)&header, 1).eof())
	{
		if ((header & 0x80) == 0x80) // run-length packet
		{
			textureFile.read((char*)&readB, 1);
			textureFile.read((char*)&readG, 1);
			textureFile.read((char*)&readR, 1);
			textureFile.read((char*)&readA, 1);

			for (int i = 0; i < (header & 0x7F) + 1; i++)
			{
				heightData[row * textureHeader.width + column] = ((float)readR) / 255;

				column++;
				if (column >= textureHeader.width)
				{
					column -= textureHeader.width;
					row--;
				}
			}
		}
		else if ((header & 0x80) == 0x00) // raw packet
		{
			for (int i = 0; i < (header & 0x7F) + 1; i++)
			{
				textureFile.read((char*)&readB, 1);
				textureFile.read((char*)&readG, 1);
				textureFile.read((char*)&readR, 1);
				textureFile.read((char*)&readA, 1);

				heightData[row * textureHeader.width + column] = ((float)readR) / 255;

				column++;
				if (column >= textureHeader.width)
				{
					column -= textureHeader.width;
					row--;
				}
			}
		}
	}
	textureFile.close();

#pragma region positions+indices
	// create vertex + uv positions and indices
	float offset = size / 2.0f;
	int numVertices = textureHeader.height * textureHeader.width;
	row = 0;
	column = 0;
	NormalUVVertex* vertices = new NormalUVVertex[numVertices];

	for (int i = 0; i < numVertices; i++)
	{
		vertices[i].UV.x = ((float)(i % textureHeader.width)) / ((float)textureHeader.width - 1.0f);
		vertices[i].UV.y = ((float)(i / textureHeader.width)) / ((float)textureHeader.height - 1.0f);
		vertices[i].UV.z = 0.0f;

		vertices[i].position.x = vertices[i].UV.x * size - offset;
		vertices[i].position.z = (1.0f - vertices[i].UV.y) * size - offset;

		vertices[i].position.y = (heightData[i] * 2.0f - 1.0f) * heightRange;
	}

	int numIndices = ((textureHeader.width - 1) * (textureHeader.height - 1)) * 6;

	UINT* indices = new UINT[numIndices];
	UINT counter = 0;

	for (UINT column = 0; column < ((UINT)textureHeader.width - 1); column++)
	{
		for (UINT row = 0; row < ((UINT)textureHeader.height - 1); row++)
		{
			indices[counter] = column + row * textureHeader.width; counter++;
			indices[counter] = column + row * textureHeader.width + 1; counter++;
			indices[counter] = column + (row + 1) * textureHeader.width; counter++;
			

			indices[counter] = column + row * textureHeader.width + 1; counter++;
			indices[counter] = column + (row + 1) * textureHeader.width + 1; counter++;
			indices[counter] = column + (row + 1) * textureHeader.width; counter++;
		}
	}
#pragma endregion

#pragma region normals+tangents
	// calculate normals and tangents
	std::list<XMVECTOR>* adjacentFaceTangents = new std::list<XMVECTOR>[numVertices];
	std::list<XMVECTOR>* adjacentFaceNormals = new std::list<XMVECTOR>[numVertices];

	for (int i = 0; i < numIndices; i += 3)
	{
		int V0 = indices[i];
		int V1 = indices[i + 1];
		int V2 = indices[i + 2];

		// calculate face normal
		XMVECTOR faceNormal = XMVector3Normalize( XMVector3Cross(XMVectorSet(vertices[V1].position.x - vertices[V0].position.x,
																			 vertices[V1].position.y - vertices[V0].position.y,
																			 vertices[V1].position.z - vertices[V0].position.z, 0.0f), 
																 XMVectorSet(vertices[V2].position.x - vertices[V0].position.x,
																			 vertices[V2].position.y - vertices[V0].position.y,
																			 vertices[V2].position.z - vertices[V0].position.z, 0.0f)));

		// calculate face tangent
		float du0 = vertices[V1].UV.x - vertices[V0].UV.x;
		float du1 = vertices[V2].UV.x - vertices[V0].UV.x;
		float dv0 = vertices[V1].UV.y - vertices[V0].UV.y;
		float dv1 = vertices[V2].UV.y - vertices[V0].UV.y;

		float divisor = 1.0f / (du0 * dv1 - dv0 * du1);

		XMVECTOR faceTangent = XMVectorSet(divisor * (dv1 * (vertices[V1].position.x - vertices[V0].position.x)
			- dv0 * (vertices[V2].position.x - vertices[V0].position.x)),

			divisor * (dv1 * (vertices[V1].position.y - vertices[V0].position.y)
				- dv0 * (vertices[V2].position.y - vertices[V0].position.y)),

			divisor * (dv1 * (vertices[V1].position.z - vertices[V0].position.z)
				- dv0 * (vertices[V2].position.z - vertices[V0].position.z)),
			0.0f);

		faceTangent = XMVector3Normalize(faceTangent);


		adjacentFaceNormals[V0].push_back(faceNormal);
		adjacentFaceNormals[V1].push_back(faceNormal);
		adjacentFaceNormals[V2].push_back(faceNormal);

		adjacentFaceTangents[V0].push_back(faceTangent);
		adjacentFaceTangents[V1].push_back(faceTangent);
		adjacentFaceTangents[V2].push_back(faceTangent);
	}

	// empty lists and average tangents
	for (int i = 0; i < numVertices; i++)
	{
		XMVECTOR averagedNormal = XMVectorReplicate(0.0f);

		int listSize = adjacentFaceNormals[i].size();

		for (int j = 0; j < listSize; j++)
		{
			averagedNormal += adjacentFaceNormals[i].front();
			adjacentFaceNormals[i].pop_front();
		}

		averagedNormal = XMVector3Normalize(averagedNormal);

		XMStoreFloat3(&(vertices[i].normal), averagedNormal);


		XMVECTOR averagedTangent = XMVectorReplicate(0.0f);

		listSize = adjacentFaceTangents[i].size();

		for (int j = 0; j < listSize; j++)
		{
			averagedTangent += adjacentFaceTangents[i].front();
			adjacentFaceTangents[i].pop_front();
		}

		averagedTangent = XMVector3Normalize(averagedTangent);

		XMStoreFloat3(&(vertices[i].tangent), averagedTangent);
	}

	// remove list
	delete[] adjacentFaceTangents;
#pragma endregion

	// create texture and normal map
	terrainTexture = Texture::CreateTexture(device, textureName);
	terrainNormalMap = Texture::CreateTexture(device, normalMapName);

	// create model
	terrainModel = Model::CreateModel(device, vertices, numVertices, indices, numIndices, 48, terrainTexture, terrainNormalMap);

	// create renderobject
	terrainObject = RenderObject::CreateRenderObject(device, terrainModel, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	delete[] vertices;
}

void RenderDeferredRendering()
{
	// geo
	// set backface culling, depth culling and blending states
	deviceContext->RSSetState(deferredGeometryRasterizerState);
	deviceContext->OMSetDepthStencilState(deferredGeometryDepthState, 0);
	deviceContext->OMSetBlendState(deferredGeometryBlendState, NULL, 0xffffffff);

	// set render targets
	ID3D11ShaderResourceView* emptySRV[3] = { nullptr, nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 3, emptySRV);

	ID3D11RenderTargetView* rtvs[] = { ColorBufferRenderTargetView, PositionBufferRenderTargetView, NormalBufferRenderTargetView };
	deviceContext->OMSetRenderTargets(3, rtvs, depthView);

	// set shaders
	deviceContext->VSSetShader(deferredGeometryVertexShader, nullptr, 0);
	deviceContext->GSSetShader(deferredGeometryGeometryShader, nullptr, 0);
	deviceContext->PSSetShader(deferredGeometryPixelShader, nullptr, 0);

	// set viewproj buffers
	ID3D11Buffer* viewProjectionMatrices[] = {	useObserverCamera ? (observerCamera->viewMatrixBuffer) : (mainCamera->viewMatrixBuffer), 
												useObserverCamera ? (observerCamera->projectionMatrixBuffer) : (mainCamera->projectionMatrixBuffer) };
	deviceContext->VSSetConstantBuffers(1, 2, viewProjectionMatrices);	// slot 1 view matrix, slot 2 projection matrix
	deviceContext->GSSetConstantBuffers(1, 2, viewProjectionMatrices);	// slot 1 view matrix, slot 2 projection matrix

	// set primitivetopology and input layout
	deviceContext->IASetInputLayout(deferredGeometryInputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// set world buffers + vertex/index buffers + render objects
	UINT vertexSize = 48;
	boxObject->Render(deviceContext, &vertexSize);
	terrainObject->Render(deviceContext, &vertexSize);
	quadTree->Render(deviceContext, &vertexSize, cameraPlanes, std::sqrtf(3.0f));


	// lights
	// set backface culling, depth culling and blending states
	deviceContext->RSSetState(deferredLightRasterizerState);
	deviceContext->OMSetDepthStencilState(deferredLightDepthState, 0);
	deviceContext->OMSetBlendState(deferredLightBlendState, NULL, 0xffffffff);

	// set render target
	deviceContext->OMSetRenderTargets(1, &backBufferRenderTargetView, depthView);

	// set shaders
	deviceContext->VSSetShader(deferredLightVertexShader, nullptr, 0);
	deviceContext->GSSetShader(deferredLightGeometryShader, nullptr, 0);

	// set viewproj buffers
	ID3D11Buffer* lightViewProjectionMatrices[] = { useObserverCamera ? (observerCamera->viewMatrixBuffer) : (mainCamera->viewMatrixBuffer),
													useObserverCamera ? (observerCamera->projectionMatrixBuffer) : (mainCamera->projectionMatrixBuffer) };
	deviceContext->VSSetConstantBuffers(1, 2, lightViewProjectionMatrices);	// slot 1 view matrix, slot 2 projection matrix
	deviceContext->GSSetConstantBuffers(1, 2, lightViewProjectionMatrices);	// slot 1 view matrix, slot 2 projection matrix

	// set primitivetopology and input layout
	deviceContext->IASetInputLayout(deferredLightInputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// set gbuffers
	ID3D11ShaderResourceView* srvs[] = { ColorBufferShaderResourceView, PositionBufferShaderResourceView, NormalBufferShaderResourceView };
	deviceContext->PSSetShaderResources(0, 3, srvs);

	// set ambient light
	deviceContext->PSSetConstantBuffers(1, 1, &ambientLightColorBuffer);

	// set camera position buffer
	deviceContext->PSSetConstantBuffers(3, 1, useObserverCamera ? &(observerCamera->cameraPositionBuffer) : &(mainCamera->cameraPositionBuffer));

	// set world buffers + vertex/index buffers + render lights
	vertexSize = 12;
	// directional lights
	deviceContext->GSSetShader(deferredDirectionalLightGeometryShader, nullptr, 0);
	deviceContext->PSSetShader(deferredDirectionalLightPixelShader, nullptr, 0);
	testDirectionalLight->Render(deviceContext, &vertexSize);

	// spotlights
	deviceContext->GSSetShader(deferredLightGeometryShader, nullptr, 0);
	deviceContext->PSSetShader(deferredSpotLightPixelShader, nullptr, 0);
	testSpotLight->Render(deviceContext, &vertexSize);

	// point lights
	deviceContext->PSSetShader(deferredPointLightPixelShader, nullptr, 0);
	testPointLight->Render(deviceContext, &vertexSize);


	// blur
	if (blur)
	{
		UINT threadsPerGroup = 256;

		// clear in/outputs
		ID3D11ShaderResourceView* emptySRV[1] = { nullptr };
		ID3D11UnorderedAccessView* emptyUAV[1] = { nullptr };
		ID3D11RenderTargetView* emptyRTV[1] = { nullptr };
		deviceContext->OMSetRenderTargets(1, emptyRTV, depthView);
		deviceContext->CSSetShaderResources(0, 1, emptySRV);
		deviceContext->CSSetUnorderedAccessViews(0, 1, emptyUAV, 0);

		// horizontal blur
		deviceContext->CSSetShader(horizontalBlurShader, nullptr, 0);

		deviceContext->CSSetConstantBuffers(0, 1, &blurWeightsBuffer);

		deviceContext->CSSetShaderResources(0, 1, &backBufferShaderResourceView);

		deviceContext->CSSetUnorderedAccessViews(0, 1, &blurredTextureUnorderedAccessView, 0);

		deviceContext->Dispatch((UINT)std::ceilf(windowWidth / ((float)threadsPerGroup)), windowHeight, 1);


		// clear in/outputs
		deviceContext->CSSetShaderResources(0, 1, emptySRV);
		deviceContext->CSSetUnorderedAccessViews(0, 1, emptyUAV, 0);

		// vertical blur
		deviceContext->CSSetShader(verticalBlurShader, nullptr, 0);

		deviceContext->CSSetConstantBuffers(0, 1, &blurWeightsBuffer);

		deviceContext->CSSetShaderResources(0, 1, &blurredTextureShaderResourceView);

		deviceContext->CSSetUnorderedAccessViews(0, 1, &backBufferUnorderedAccessView, 0);

		deviceContext->Dispatch(windowWidth, (UINT)std::ceilf(windowHeight / ((float)threadsPerGroup)), 1);

		// clear in/outputs
		deviceContext->CSSetShaderResources(0, 1, emptySRV);
		deviceContext->CSSetUnorderedAccessViews(0, 1, emptyUAV, 0);
	}
}

void CreateGBuffer(ID3D11Texture2D** texture, ID3D11RenderTargetView** renderTargetView, ID3D11ShaderResourceView** shaderResourceView)
{
	D3D11_TEXTURE2D_DESC desc;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Height = windowHeight;
	desc.MipLevels = 0;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = windowWidth;

	device->CreateTexture2D(&desc, 0, texture);

	D3D11_RENDER_TARGET_VIEW_DESC rendDesc;
	rendDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	rendDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rendDesc.Texture2D.MipSlice = 0;

	device->CreateRenderTargetView(*texture, &rendDesc, renderTargetView);

	D3D11_SHADER_RESOURCE_VIEW_DESC shadDesc;
	shadDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	shadDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shadDesc.Texture2D.MipLevels = 1;
	shadDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(*texture, &shadDesc, shaderResourceView);
}

void LoadOBJ(std::string fileName, NormalUVVertex*& verticesArray, UINT& numVertices, UINT*& indicesArray, UINT& numIndices)
{
	std::ifstream modelFile(fileName);

	std::string str;

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<XMFLOAT3> uvs;
	std::vector<UINT> indices;
	std::vector<XMINT3> vertexElementIndices;

	int positionsNum = 0;
	int normalsNum = 0;
	int uvsNum = 0;

	modelFile >> str;

	while (!modelFile.eof())
	{
		if (str == "v")		// vertex position
		{
			if (positions.size() <= positionsNum)
			{
				positions.push_back(XMFLOAT3());
			}

			modelFile >> str;
			positions[positionsNum].x = std::stof(str, nullptr);

			modelFile >> str;
			positions[positionsNum].y = std::stof(str, nullptr);

			modelFile >> str;
			positions[positionsNum].z = std::stof(str, nullptr);

			positionsNum++;
		}
		else if (str == "vn")	// vertex normal
		{
			if (normals.size() <= normalsNum)
			{
				normals.push_back(XMFLOAT3());
			}

			modelFile >> str;
			normals[normalsNum].x = std::stof(str, nullptr);

			modelFile >> str;
			normals[normalsNum].y = std::stof(str, nullptr);

			modelFile >> str;
			normals[normalsNum].z = std::stof(str, nullptr);

			normalsNum++;
		}
		else if (str == "vt")	// vertex UV
		{
			if (uvs.size() <= uvsNum)
			{
				uvs.push_back(XMFLOAT3());
			}

			modelFile >> str;
			uvs[uvsNum].x = std::stof(str, nullptr);

			modelFile >> str;
			uvs[uvsNum].y = std::stof(str, nullptr);

			uvs[uvsNum].z = 0;

			uvsNum++;
		}
		else if (str == "f")
		{
			XMINT3 elementIndices;

			char temp[10];

			for (int i = 0; i < 3; i++)
			{
				modelFile.get(temp, 10, '/');
				str = temp;
				elementIndices.x = std::stoi(str);
				modelFile.get();

				modelFile.get(temp, 10, '/');
				str = temp;
				elementIndices.y = std::stoi(str);
				modelFile.get();

				modelFile >> str;
				elementIndices.z = std::stoi(str);

				int foundIndex = -1;

				for (int i = 0; i < vertexElementIndices.size(); i++)
				{
					if (vertexElementIndices[i].x == elementIndices.x &&
						vertexElementIndices[i].y == elementIndices.y &&
						vertexElementIndices[i].z == elementIndices.z)
					{
						foundIndex = i;
						break;
					}
				}

				if (foundIndex != -1)	// found in vertexElementIndices
				{
					indices.push_back(foundIndex);
				}
				else	// not found
				{
					vertexElementIndices.push_back(elementIndices);
					indices.push_back(vertexElementIndices.size() - 1);
				}
			}
		}
		else	// discard
		{
			std::getline(modelFile, str);
		}

		modelFile >> str;
	}

	numVertices = vertexElementIndices.size();

	delete[] verticesArray;
	verticesArray = new NormalUVVertex[numVertices];

	for (int i = 0; i < numVertices; i++)
	{
		verticesArray[i].position = positions[vertexElementIndices[i].x - 1];
		verticesArray[i].UV = uvs[vertexElementIndices[i].y - 1];
		verticesArray[i].normal = normals[vertexElementIndices[i].z - 1];
		verticesArray[i].tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	numIndices = indices.size();

	delete[] indicesArray;
	indicesArray = new UINT[numIndices];

	for (int i = 0; i < numIndices; i++)
	{
		indicesArray[i] = indices[i];
	}

	modelFile.close();

	// calculate tangents
	std::list<XMVECTOR>* adjacentFaceTangents = new std::list<XMVECTOR>[numVertices];

	for (int i = 0; i < numIndices; i += 3)
	{
		int V0 = indices[i];
		int V1 = indices[i + 1];
		int V2 = indices[i + 2];

		float du0 = verticesArray[V1].UV.x - verticesArray[V0].UV.x;
		float du1 = verticesArray[V2].UV.x - verticesArray[V0].UV.x;
		float dv0 = verticesArray[V1].UV.y - verticesArray[V0].UV.y;
		float dv1 = verticesArray[V2].UV.y - verticesArray[V0].UV.y;

		float divisor = 1.0f / (du0 * dv1 - dv0 * du1);

		XMVECTOR faceTangent = XMVectorSet( divisor * (		dv1 * (verticesArray[V1].position.x - verticesArray[V0].position.x)
														  - dv0 * (verticesArray[V2].position.x - verticesArray[V0].position.x)),

											divisor * (		dv1 * (verticesArray[V1].position.y - verticesArray[V0].position.y)
														  - dv0 * (verticesArray[V2].position.y - verticesArray[V0].position.y)), 

											divisor * (		dv1 * (verticesArray[V1].position.z - verticesArray[V0].position.z)
														  - dv0 * (verticesArray[V2].position.z - verticesArray[V0].position.z)), 
											0.0f);

		faceTangent = XMVector3Normalize(faceTangent);

		adjacentFaceTangents[V0].push_back(faceTangent);
		adjacentFaceTangents[V1].push_back(faceTangent);
		adjacentFaceTangents[V2].push_back(faceTangent);
	}

	// empty lists and average tangents
	for (int i = 0; i < numVertices; i++)
	{
		XMVECTOR averagedTangent = XMVectorReplicate(0.0f);

		int listSize = adjacentFaceTangents[i].size();

		for (int j = 0; j < listSize; j++)
		{
			averagedTangent += adjacentFaceTangents[i].front();
			adjacentFaceTangents[i].pop_front();
		}

		averagedTangent = XMVector3Normalize(averagedTangent);

		XMStoreFloat3(&(verticesArray[i].tangent), averagedTangent);
	}

	// remove list
	delete[] adjacentFaceTangents;
}

void LoadLightOBJ(std::string fileName, Vertex*& verticesArray, UINT& numVertices, UINT*& indicesArray, UINT& numIndices)
{
	std::ifstream modelFile(fileName);

	std::string str;

	std::vector<XMFLOAT3> positions;
	std::vector<UINT> indices;
	std::vector<UINT> vertexElementIndices;

	int positionsNum = 0;

	modelFile >> str;

	while (!modelFile.eof())
	{
		if (str == "v")		// vertex position
		{
			if (positions.size() <= positionsNum)
			{
				positions.push_back(XMFLOAT3());
			}

			modelFile >> str;
			positions[positionsNum].x = std::stof(str, nullptr);

			modelFile >> str;
			positions[positionsNum].y = std::stof(str, nullptr);

			modelFile >> str;
			positions[positionsNum].z = std::stof(str, nullptr);

			positionsNum++;
		}
		else if (str == "f")
		{
			UINT elementIndices;

			char temp[10];

			for (int i = 0; i < 3; i++)
			{
				modelFile.get(temp, 10, '/');
				str = temp;
				elementIndices = std::stoi(str);
				modelFile.get();

				modelFile.get(temp, 10, '/');
				str = temp;
				modelFile.get();

				modelFile >> str;

				int foundIndex = -1;

				for (int i = 0; i < vertexElementIndices.size(); i++)
				{
					if (vertexElementIndices[i] == elementIndices)
					{
						foundIndex = i;
						break;
					}
				}

				if (foundIndex != -1)	// found in vertexElementIndices
				{
					indices.push_back(foundIndex);
				}
				else	// not found
				{
					vertexElementIndices.push_back(elementIndices);
					indices.push_back(vertexElementIndices.size() - 1);
				}
			}
		}
		else	// discard
		{
			std::getline(modelFile, str);
		}

		modelFile >> str;
	}

	numVertices = vertexElementIndices.size();

	delete[] verticesArray;
	verticesArray = new Vertex[numVertices];

	for (int i = 0; i < numVertices; i++)
	{
		verticesArray[i].position = positions[vertexElementIndices[i] - 1];
	}

	numIndices = indices.size();

	delete[] indicesArray;
	indicesArray = new UINT[numIndices];

	for (int i = 0; i < numIndices; i++)
	{
		indicesArray[i] = indices[i];
	}

	modelFile.close();
}

void CreateLightGeometry()
{
	// create light geometry

	// spotlight
	Vertex* vertexData = nullptr;
	UINT numVertices = 0;
	UINT* indexData = nullptr;
	UINT numIndices = 0;

	LoadLightOBJ("cone.obj", vertexData, numVertices, indexData, numIndices);

	spotLightModel = Model::CreateModel(device, vertexData, numVertices, indexData, numIndices, 12, nullptr, nullptr);

	delete[] vertexData;
	delete[] indexData;

	vertexData = nullptr;
	indexData = nullptr;

	// pointlight
	LoadLightOBJ("sphere.obj", vertexData, numVertices, indexData, numIndices);

	pointLightModel = Model::CreateModel(device, vertexData, numVertices, indexData, numIndices, 12, nullptr, nullptr);

	delete[] vertexData;
	delete[] indexData;

	// directional light
	numVertices = 4;
	numIndices = 6;
	Vertex verts[] = {	Vertex(XMFLOAT3(-1, -1, 1.0f)),
						Vertex(XMFLOAT3(-1, 1, 1.0f)),
						Vertex(XMFLOAT3(1, -1, 1.0f)),
						Vertex(XMFLOAT3(1, 1, 1.0f)) };

	Vertex* verData = verts;

	UINT indx[] = {0, 3, 1, 0, 2, 3};
	indexData = indx;

	directionalLightModel = Model::CreateModel(device, verData, numVertices, indexData, numIndices, 12, nullptr, nullptr);
}

XMFLOAT4 readObjectColor(char* materialFileName)
{
	std::ifstream file(materialFileName);

	std::string str;

	XMFLOAT4 readColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	file >> str;

	while (!file.eof())
	{
		if (str == "color")
		{
			file >> str;
			readColor.x = std::stof(str, nullptr);

			file >> str;
			readColor.y = std::stof(str, nullptr);

			file >> str;
			readColor.z = std::stof(str, nullptr);
		}
		else	// discard
		{
			std::getline(file, str);
		}

		file >> str;
	}

	return readColor;
}

AABB* createQuadTree(XMFLOAT3 maxCorner, XMFLOAT3 minCorner, ID3D11Device* device, Model* objectModel, XMFLOAT4 objectColor, UINT levels)
{
	AABB* newAABB = new AABB(maxCorner, minCorner);

	if (levels > 0)
	{
		newAABB->nxpz = createQuadTree(XMFLOAT3(maxCorner.x, maxCorner.y, minCorner.z + (maxCorner.z - minCorner.z) / 2.0f), XMFLOAT3(minCorner.x + (maxCorner.x - minCorner.x) / 2.0f, minCorner.y, minCorner.z), device, objectModel, objectColor, levels - 1);
		newAABB->pxpz = createQuadTree(maxCorner, XMFLOAT3(minCorner.x + (maxCorner.x - minCorner.x) / 2.0f, minCorner.y, minCorner.z + (maxCorner.z - minCorner.z) / 2.0f), device, objectModel, objectColor, levels - 1);;
		newAABB->nxnz = createQuadTree(XMFLOAT3(minCorner.x + (maxCorner.x - minCorner.x) / 2.0f, maxCorner.y, minCorner.z + (maxCorner.z - minCorner.z) / 2.0f), minCorner, device, objectModel, objectColor, levels - 1);;
		newAABB->pxnz = createQuadTree(XMFLOAT3(minCorner.x + (maxCorner.x - minCorner.x) / 2.0f, maxCorner.y, maxCorner.z), XMFLOAT3(minCorner.x, minCorner.y, minCorner.z + (maxCorner.z - minCorner.z) / 2.0f), device, objectModel, objectColor, levels - 1);;
	}
	else
	{
		newAABB->object = RenderObject::CreateRenderObject(device, objectModel, objectColor);
		newAABB->object->SetWorldMatrix(deviceContext, XMMatrixSet(	1.0f, 0.0f, 0.0f, 0.0f,
																	0.0f, 1.0f, 0.0f, 0.0f,
																	0.0f, 0.0f, 1.0f, 0.0f,
																	minCorner.x + (maxCorner.x - minCorner.x) / 2.0f, minCorner.y + (maxCorner.y - minCorner.y) / 2.0f, minCorner.z + (maxCorner.z - minCorner.z) / 2.0f, 1.0f));
	}

	return newAABB;
}

void getCameraPlanes(Camera* camera)
{
	XMFLOAT4X4 m;

	XMMATRIX proj = mainCamera->GetProjectionMatrix();
	XMMATRIX view = mainCamera->GetViewMatrix();
	XMMATRIX ma = XMMatrixMultiply(view, proj);

	XMStoreFloat4x4(&m, ma);

	// left plane
	cameraPlanes[0].normal.x = -(m._14 + m._11);
	cameraPlanes[0].normal.y = -(m._24 + m._21);
	cameraPlanes[0].normal.z = -(m._34 + m._31);
	cameraPlanes[0].distance = -(m._44 + m._41);

	// right plane
	cameraPlanes[1].normal.x = -(m._14 - m._11);
	cameraPlanes[1].normal.y = -(m._24 - m._21);
	cameraPlanes[1].normal.z = -(m._34 - m._31);
	cameraPlanes[1].distance = -(m._44 - m._41);

	// top plane
	cameraPlanes[2].normal.x = -(m._14 - m._12);
	cameraPlanes[2].normal.y = -(m._24 - m._22);
	cameraPlanes[2].normal.z = -(m._34 - m._32);
	cameraPlanes[2].distance = -(m._44 - m._42);

	// bottom plane
	cameraPlanes[3].normal.x = -(m._14 + m._12);
	cameraPlanes[3].normal.y = -(m._24 + m._22);
	cameraPlanes[3].normal.z = -(m._34 + m._32);
	cameraPlanes[3].distance = -(m._44 + m._42);

	// near plane
	cameraPlanes[4].normal.x = -(m._14 + m._13);
	cameraPlanes[4].normal.y = -(m._24 + m._23);
	cameraPlanes[4].normal.z = -(m._34 + m._33);
	cameraPlanes[4].distance = -(m._44 + m._43);

	// far plane
	cameraPlanes[5].normal.x = -(m._14 - m._13);
	cameraPlanes[5].normal.y = -(m._24 - m._23);
	cameraPlanes[5].normal.z = -(m._34 - m._33);
	cameraPlanes[5].distance = -(m._44 - m._43);

	// normalize
	for (int i = 0; i < 6; i++)
	{
		float divisor = 1.0f / XMVectorGetX(XMVector3Length(XMLoadFloat3(&cameraPlanes[i].normal)));

		cameraPlanes[i].normal.x *= divisor;
		cameraPlanes[i].normal.y *= divisor;
		cameraPlanes[i].normal.z *= divisor;
		cameraPlanes[i].distance *= divisor;
	}
}