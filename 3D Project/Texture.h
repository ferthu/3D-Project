#ifndef TEXTURE_H
#define TEXTURE_H

#include <d3d11.h>
#include <d3dcompiler.h>

class Texture
{
public:
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* textureView;
	
	Texture(ID3D11Device* device, UINT height, UINT width, const void* dataLocation);
	static Texture* CreateTexture(ID3D11Device* device, const char* textureFileName);
	~Texture();
private:
	Texture();
};

#endif