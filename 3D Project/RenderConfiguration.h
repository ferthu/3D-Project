#ifndef RENDERCONFIGURATION_H
#define RENDERCONFIGURATION_H

#include <d3d11.h>
#include <d3dcompiler.h>

class RenderConfiguration
{
public:
	UINT vertexElements;
	UINT32 vertexSize;
	UINT32 offset;
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	D3D11_INPUT_ELEMENT_DESC* vertexDescription;
	ID3D11InputLayout* inputLayout;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;

	~RenderConfiguration()
	{
		if (vertexBuffer != nullptr)
			vertexBuffer->Release();

		if (indexBuffer != nullptr)
			indexBuffer->Release();

		if (inputLayout != nullptr)
			inputLayout->Release();

		if (vertexShader != nullptr)
			vertexShader->Release();

		if (pixelShader != nullptr)
			pixelShader->Release();
	}
};

#endif