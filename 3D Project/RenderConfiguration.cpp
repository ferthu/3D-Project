#include <DirectXMath.h>
#include "RenderConfiguration.h"

using namespace DirectX;

RenderConfiguration::RenderConfiguration(
	ID3D11Device* device,
	ID3D11DeviceContext* deviceContext,
	int elementsPerVertex, 
	VertexElementDescription* elementsDescription,
	LPCWSTR vertexShaderName,
	LPCWSTR geometryShaderName,		// pass empty string to disable
	LPCWSTR pixelShaderName,
	Camera* camera)
{
	// set variables to default values
	inputLayout = nullptr;
	vertexDescription = nullptr;

	vertexShader = nullptr;
	geometryShader = nullptr;
	pixelShader = nullptr;

	this->camera = camera;

#pragma region input layout
	// set up input layout
	vertexNumElements = elementsPerVertex;
	vertexDescription = new D3D11_INPUT_ELEMENT_DESC[elementsPerVertex];

	UINT byteOffset = 0;

	for (int i = 0; i < elementsPerVertex; i++)
	{
		vertexDescription[i].SemanticName = elementsDescription[i].semanticName;
		vertexDescription[i].SemanticIndex = elementsDescription[i].semanticIndex;
		vertexDescription[i].Format = elementsDescription[i].vec4 ?
			DXGI_FORMAT_R32G32B32A32_FLOAT :
			DXGI_FORMAT_R32G32B32_FLOAT;
		vertexDescription[i].InputSlot = 0;
		vertexDescription[i].AlignedByteOffset = byteOffset;
		vertexDescription[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		vertexDescription[i].InstanceDataStepRate = 0;

		byteOffset += elementsDescription[i].vec4 ? sizeof(XMFLOAT4) : sizeof(XMFLOAT3);
	}

	vertexSize = byteOffset;
#pragma endregion

#pragma region vertex shader + input layout
	// create vertex shader
	ID3DBlob* vs = nullptr;

	HRESULT hr = D3DCompileFromFile(
		vertexShaderName,
		nullptr,
		nullptr,
		"VSmain",
		"vs_4_0",
		0, 0,
		&vs,
		nullptr);

	device->CreateVertexShader(
		vs->GetBufferPointer(),
		vs->GetBufferSize(),
		nullptr,
		&vertexShader);

	// create input layout
	device->CreateInputLayout(
		vertexDescription,
		vertexNumElements,
		vs->GetBufferPointer(),
		vs->GetBufferSize(),
		&inputLayout);
	
	vs->Release();
#pragma endregion

#pragma region geometry shader
	// create geometry shader
	if (geometryShaderName != L"")
	{
		ID3DBlob* gs = nullptr;

		D3DCompileFromFile(
			geometryShaderName,
			nullptr,
			nullptr,
			"GSmain",
			"gs_4_0",
			0, 0,
			&gs,
			nullptr);

		device->CreateGeometryShader(
			gs->GetBufferPointer(),
			gs->GetBufferSize(),
			nullptr,
			&geometryShader);

		gs->Release();
	}
#pragma endregion

#pragma region pixel shader
	// create pixel shader
	ID3DBlob* ps = nullptr;

	D3DCompileFromFile(
		pixelShaderName,
		nullptr,
		nullptr,
		"PSmain",
		"ps_4_0",
		0, 0,
		&ps,
		nullptr);

	device->CreatePixelShader(
		ps->GetBufferPointer(),
		ps->GetBufferSize(),
		nullptr,
		&pixelShader);

	ps->Release();
#pragma endregion
}

RenderConfiguration::~RenderConfiguration()
{
	if (inputLayout != nullptr)
		inputLayout->Release();

	if (vertexShader != nullptr)
		vertexShader->Release();

	if (geometryShader != nullptr)
		geometryShader->Release();

	if (pixelShader != nullptr)
		pixelShader->Release();

	if (vertexDescription != 0)
	{
		delete[] vertexDescription;
	}

	while (objects.size() > 0)
	{
		delete objects.back();
		objects.pop_back();
	}

	int modelsSize = models.size();
	for (int i = 0; i < modelsSize; i++)
	{
		delete models[i];
	}
	models.clear();
}

RenderConfiguration* RenderConfiguration::CreateRenderConfiguration(ID3D11Device* device,
	ID3D11DeviceContext* deviceContext,
	int elementsPerVertex,
	VertexElementDescription* elementsDescription,
	LPCWSTR vertexShaderName,
	LPCWSTR geometryShaderName,
	LPCWSTR pixelShaderName,
	Camera* camera)
{
	return new RenderConfiguration(device, deviceContext, elementsPerVertex, elementsDescription, vertexShaderName, geometryShaderName, pixelShaderName, camera);
}

void RenderConfiguration::Update()
{ 
	// update every object
	std::list<RenderObject*>::iterator it = objects.begin();
	std::list<RenderObject*>::iterator end = objects.end();

	for (; it != end; ++it)
	{
		it._Ptr->_Myval->Update();
	}
}

void RenderConfiguration::Render(ID3D11DeviceContext* deviceContext)
{
	// set shaders
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->HSSetShader(nullptr, nullptr, 0);
	deviceContext->DSSetShader(nullptr, nullptr, 0);
	deviceContext->GSSetShader(geometryShader, nullptr, 0);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);

	deviceContext->IASetInputLayout(inputLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11Buffer* viewProjectionMatrices[] = { (camera->viewMatrixBuffer), (camera->projectionMatrixBuffer) };

	deviceContext->VSSetConstantBuffers(1, 2, viewProjectionMatrices);

	// render every object
	std::list<RenderObject*>::iterator it = objects.begin();
	std::list<RenderObject*>::iterator end = objects.end();

	for (; it != end; ++it)
	{
		it._Ptr->_Myval->Render(deviceContext, &vertexSize);
	}
}

void RenderConfiguration::CreateObject(ID3D11Device* device, Model* model)
{
	objects.push_back(RenderObject::CreateRenderObject(device, model));
}

void RenderConfiguration::CreateModel(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices)
{
	models.push_back(Model::CreateModel(device, vertexData, numVertices, indexData, numIndices, vertexSize));
}