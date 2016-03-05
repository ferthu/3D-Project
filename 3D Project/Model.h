#ifndef MODEL_H
#define MODEL_H

#include <d3d11.h>
#include "VertexStructureDefinitions.h"

class Model
{
public:
	static Model* CreateModel(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices, UINT vertexSize);
	~Model();

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	UINT numVertices;
	UINT numIndices;
private:
	Model(ID3D11Device* device, Vertex* vertexData, UINT numVertices, UINT* indexData, UINT numIndices, UINT vertexSize);
};

#endif