#ifndef RENDERCONFIGURATION_H
#define RENDERCONFIGURATION_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <list>
#include <vector>
#include "RenderObject.h"
#include "Model.h"
#include "Camera.h"

struct VertexElementDescription
{
	LPCSTR semanticName;
	UINT semanticIndex;
	bool vec4;		// true if 4-element vector, false if 3-element
};

class RenderConfiguration
{
public:
	void Update();
	void Render(ID3D11DeviceContext* deviceContext);
	void CreateObject(ID3D11Device* device, Model* model);
	void CreateModel(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices);

	static RenderConfiguration* CreateRenderConfiguration(ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		int elementsPerVertex,
		VertexElementDescription* elementsDescription,
		LPCWSTR vertexShaderName,
		LPCWSTR geometryShaderName,
		LPCWSTR pixelShaderName,
		Camera* camera);
	~RenderConfiguration();

	Camera* camera;
	std::vector<Model*> models;

private:
	UINT vertexNumElements;
	ID3D11InputLayout* inputLayout;
	D3D11_INPUT_ELEMENT_DESC* vertexDescription;
	UINT32 vertexSize;

	ID3D11VertexShader* vertexShader;
	ID3D11GeometryShader* geometryShader;
	ID3D11PixelShader* pixelShader;

	std::list<RenderObject*> objects;

	RenderConfiguration(ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		int elementsPerVertex,
		VertexElementDescription* elementsDescription,
		LPCWSTR vertexShaderName,
		LPCWSTR geometryShaderName,
		LPCWSTR pixelShaderName,
		Camera* camera);
};

#endif