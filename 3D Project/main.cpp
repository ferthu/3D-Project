#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "RenderConfiguration.h"
#include "RenderObject.h"
#include "VertexStructureDefinitions.h"

using namespace DirectX;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")


#pragma region variable declarations
// will hold handle to window
HWND windowHandle = 0;

UINT windowWidth = 800;
UINT windowHeight = 600;

const bool DEBUG = true;

UINT msaaSamples = 8;		// minimum 1

D3D_FEATURE_LEVEL* featureLevel = nullptr;

ID3D11Device* device = nullptr;
ID3D11DeviceContext* deviceContext = nullptr;
IDXGISwapChain* swapChain = nullptr;
ID3D11RenderTargetView* backBufferRenderTargetView;
ID3D11DepthStencilView* depthView;
#pragma endregion

RenderConfiguration* colorTest;
Camera* testCam;
float camYaw = 0.0f;

// declare window procedure function
LRESULT CALLBACK windowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

void initializeD3D();
void Update();
void Render();
void createTestInput();

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

	initializeD3D();

	createTestInput();

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
	delete colorTest;
	delete testCam;

	device->Release();
	deviceContext->Release();
	swapChain->Release();
	backBufferRenderTargetView->Release();

	// returns parameter of WM_QUIT message
	return (int)msg.wParam;
}

LRESULT CALLBACK windowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	// handle messages
	switch (message)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(windowHandle);
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
	camYaw += 0.0001f;

	XMFLOAT3 pos = XMFLOAT3(0.0f, 0.0f, -2.0f);

	colorTest->camera->SetViewMatrix(deviceContext, colorTest->camera->CreateViewMatrix(XMLoadFloat3(&pos), -camYaw, camYaw));
}

void Render()
{
	// clear window to background color
	float backgroundColor[] = {0, 0, 0, 1};
	deviceContext->ClearRenderTargetView(backBufferRenderTargetView, backgroundColor);

	// clear depth buffer
	deviceContext->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	colorTest->Render(deviceContext);

	// display frame
	swapChain->Present(0, 0);
}

void createTestInput()
{
	VertexElementDescription colorVertexDescription[2];
	colorVertexDescription[0].semanticName = "POSITION";
	colorVertexDescription[0].semanticIndex = 0;
	colorVertexDescription[0].vec4 = false;
	colorVertexDescription[1].semanticName = "COLOR";
	colorVertexDescription[1].semanticIndex = 0;
	colorVertexDescription[1].vec4 = false;

	ColorVertex vertexData[3] = 
	{
		{ XMFLOAT3(0.0f, 0.0f, 0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(0.0f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(0.5f, 0.0f, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) }
	};

	UINT indexData[6] = { 0, 1, 2, 0, 2, 1 };

	XMFLOAT3 camPos = XMFLOAT3(0.0f, 0.0f, -2.0f);
	testCam = Camera::CreateCamera(device, deviceContext, XM_PI * 0.5f, 0.5f, 20.0f, XMLoadFloat3(&camPos), 0.0f, 0.0f, XM_PIDIV2 * 0.05f, -XM_PIDIV2 * 0.05f, ((float)windowHeight) / ((float)windowWidth));

	colorTest = RenderConfiguration::CreateRenderConfiguration(
		device,
		deviceContext,
		2,
		colorVertexDescription,
		L"TestVertex.hlsl",
		L"TestGeometry.hlsl",
		L"TestPixel.hlsl",
		testCam);

	colorTest->CreateModel(device, vertexData, 3, indexData, 6);

	colorTest->CreateObject(device, colorTest->models[0]);

	XMMATRIX world = XMMATRIX(1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	colorTest->objects.front()->SetWorldMatrix(deviceContext, world);
}