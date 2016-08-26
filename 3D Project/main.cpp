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

using namespace DirectX;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")


#pragma region variable declarations
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
ID3D11DepthStencilView* depthView;

LARGE_INTEGER countFrequency;
LARGE_INTEGER currentTime, previousTime, elapsedTime;
double frameTime;

POINT currentMousePosition;
POINT previousMousePosition;
float mouseSensitivity = 0.003f;
#pragma endregion

Texture* testTexture = nullptr;
Camera* testCam = nullptr;
SpotLight* testSpotLight = nullptr;
PointLight* testPointLight = nullptr;
DirectionalLight* testDirectionalLight = nullptr;

Camera* mainCamera = nullptr;

Model* boxModel = nullptr;
RenderObject* boxObject = nullptr;
Texture* boxTexture = nullptr;
Texture* boxNormalMap = nullptr;

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

	testTexture = Texture::CreateTexture(device, "test4.tga");

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

	swapChainDesc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
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

	XMVector3Normalize(cameraTranslation);

	XMVECTOR newPos = XMLoadFloat3(&(mainCamera->position)) + cameraTranslation * 2;

	XMStoreFloat3(&(mainCamera->position), newPos);

	mainCamera->SetViewMatrix(deviceContext, mainCamera->CreateViewMatrix(XMLoadFloat3(&(mainCamera->position)), mainCamera->pitch, mainCamera->yaw));


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
	testPointLight = new PointLight(device, deviceContext, XMFLOAT4(1.3f, 1.25f, -1.5f, 1), XMFLOAT4(1.0f, 0.0f, 0.0f, 1), 10.0f, pointLightModel);
	testPointLight->Initialize();
	testSpotLight = new SpotLight(device, deviceContext, XMFLOAT4(-1.3, 2, -1.3, 1), XMFLOAT4(0, 1, 0, 1), XMFLOAT4(1, -1.7, 1, 0), 5, 3, spotLightModel);
	testSpotLight->Initialize();
	testDirectionalLight = new DirectionalLight(device, deviceContext, XMFLOAT4(0, 0, 0, 1), XMFLOAT4(0.0f, 0.0f, 0.1f, 1), XMFLOAT4(1, -1, 0.7f, 0), directionalLightModel);
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
	XMFLOAT3 camPos = XMFLOAT3(0.0f, 0.0f, -5.0f);
	mainCamera = Camera::CreateCamera(device, deviceContext, XM_PI * 0.5f, 0.5f, 20.0f, XMLoadFloat3(&camPos), 0.0f, 0.0f, XM_PIDIV2 * 1.0f, -XM_PIDIV2 * 1.0f, ((float)windowHeight) / ((float)windowWidth));
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

	boxObject = RenderObject::CreateRenderObject(device, boxModel);

	delete[] vertices;
	delete[] indices;
#pragma endregion

#pragma region ambient light buffer
	XMFLOAT4 col = XMFLOAT4(0.025f, 0.025f, 0.025f, 0.0f);
	ambientLightColor = XMLoadFloat4(&col);

	// create description of world matrix buffer
	D3D11_BUFFER_DESC ambientLightColorBufferDescription;
	ambientLightColorBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	ambientLightColorBufferDescription.ByteWidth = sizeof(ambientLightColor);
	ambientLightColorBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ambientLightColorBufferDescription.CPUAccessFlags = 0;
	ambientLightColorBufferDescription.MiscFlags = 0;
	ambientLightColorBufferDescription.StructureByteStride = 0;

	// create world matrix buffer
	D3D11_SUBRESOURCE_DATA ambientLightColorBufferData;
	ambientLightColorBufferData.pSysMem = &ambientLightColor;
	ambientLightColorBufferData.SysMemPitch = 0;
	ambientLightColorBufferData.SysMemSlicePitch = 0;

	// create world matrix buffer
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
	ID3D11Buffer* viewProjectionMatrices[] = { (mainCamera->viewMatrixBuffer), (mainCamera->projectionMatrixBuffer) };
	deviceContext->VSSetConstantBuffers(1, 2, viewProjectionMatrices);	// slot 1 view matrix, slot 2 projection matrix
	deviceContext->GSSetConstantBuffers(1, 2, viewProjectionMatrices);	// slot 1 view matrix, slot 2 projection matrix

	// set primitivetopology and input layout
	deviceContext->IASetInputLayout(deferredGeometryInputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// set world buffers + vertex/index buffers + render objects
	UINT vertexSize = 48;
	boxObject->Render(deviceContext, &vertexSize);
	// ...


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
	ID3D11Buffer* lightViewProjectionMatrices[] = { (mainCamera->viewMatrixBuffer), (mainCamera->projectionMatrixBuffer) };
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