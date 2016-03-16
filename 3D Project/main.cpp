#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <cmath>

#include <sstream>
#include <iostream>
#include <fstream>

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

LARGE_INTEGER countFrequency;
LARGE_INTEGER currentTime, previousTime, elapsedTime;
double frameTime;

POINT currentMousePosition;
POINT previousMousePosition;
float mouseSensitivity = 0.003f;
#pragma endregion

RenderConfiguration* colorTest;
RenderConfiguration* texUVTest;
Camera* testCam;
float camYaw = 0.0f;

// declare window procedure function
LRESULT CALLBACK windowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

void initializeD3D();
void Update();
void Render();
void CreateTestInput();
void UpdateFrameTime();
void CreateTestModel();

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

	CreateTestInput();
	CreateTestModel();

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

	colorTest->camera->yaw += (currentMousePosition.x - previousMousePosition.x) * mouseSensitivity;
	colorTest->camera->pitch += (currentMousePosition.y - previousMousePosition.y) * mouseSensitivity;

	XMFLOAT3 ct = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMVECTOR cameraTranslation = XMLoadFloat3(&ct);

	XMFLOAT3 cameraX = XMFLOAT3(cosf(colorTest->camera->yaw) * frameTime, 0.0f, -sinf(colorTest->camera->yaw) * frameTime);
	XMFLOAT3 cameraZ = XMFLOAT3(sinf(colorTest->camera->yaw) * frameTime, 0.0f, cosf(colorTest->camera->yaw) * frameTime);

	if (GetKeyState('W') & 0x8000)	// highest bit set to 1 means key is pressed
		cameraTranslation += XMLoadFloat3(&cameraZ);

	if (GetKeyState('S') & 0x8000)
		cameraTranslation -= XMLoadFloat3(&cameraZ);

	if (GetKeyState('D') & 0x8000)
		cameraTranslation += XMLoadFloat3(&cameraX);

	if (GetKeyState('A') & 0x8000)
		cameraTranslation -= XMLoadFloat3(&cameraX);

	XMVector3Normalize(cameraTranslation);

	XMVECTOR newPos = XMLoadFloat3(&(colorTest->camera->position)) + cameraTranslation;

	XMStoreFloat3(&(colorTest->camera->position), newPos);

	colorTest->camera->SetViewMatrix(deviceContext, colorTest->camera->CreateViewMatrix(XMLoadFloat3(&(colorTest->camera->position)), colorTest->camera->pitch, colorTest->camera->yaw));


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
	float backgroundColor[] = {0, 0, 0, 1};
	deviceContext->ClearRenderTargetView(backBufferRenderTargetView, backgroundColor);

	// clear depth buffer
	deviceContext->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	colorTest->Render(deviceContext);
	texUVTest->Render(deviceContext);

	// display frame
	swapChain->Present(0, 0);
}

void CreateTestInput()
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
	testCam = Camera::CreateCamera(device, deviceContext, XM_PI * 0.5f, 0.5f, 20.0f, XMLoadFloat3(&camPos), 0.0f, 0.0f, XM_PIDIV2 * 1.0f, -XM_PIDIV2 * 1.0f, ((float)windowHeight) / ((float)windowWidth));

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

	VertexElementDescription texUVVertexDescription[3];
	texUVVertexDescription[0].semanticName = "POSITION";
	texUVVertexDescription[0].semanticIndex = 0;
	texUVVertexDescription[0].vec4 = false;
	texUVVertexDescription[1].semanticName = "NORMAL";
	texUVVertexDescription[1].semanticIndex = 0;
	texUVVertexDescription[1].vec4 = false;
	texUVVertexDescription[2].semanticName = "TEXCOORD";
	texUVVertexDescription[2].semanticIndex = 0;
	texUVVertexDescription[2].vec4 = false;

	texUVTest = RenderConfiguration::CreateRenderConfiguration(
		device,
		deviceContext,
		3,
		texUVVertexDescription,
		L"TestVertex2.hlsl",
		L"",
		L"TestPixel2.hlsl",
		testCam);
}

void CreateTestModel()
{
	std::ifstream modelFile("box.obj");

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

	NormalUVVertex* vertices = new NormalUVVertex[vertexElementIndices.size()];

	for (int i = 0; i < vertexElementIndices.size(); i++)
	{
		vertices[i].position = positions[vertexElementIndices[i].x - 1];
		vertices[i].UV = uvs[vertexElementIndices[i].y - 1];
		vertices[i].normal = normals[vertexElementIndices[i].z - 1];
	}

	UINT* indexArray = new UINT[indices.size()];

	for (int i = 0; i < indices.size(); i++)
	{
		indexArray[i] = indices[i];
	}

	texUVTest->CreateModel(device, vertices, vertexElementIndices.size(), indexArray, indices.size());
	texUVTest->CreateObject(device, texUVTest->models[0]);

	delete[] vertices;
	delete[] indexArray;

	modelFile.close();
}