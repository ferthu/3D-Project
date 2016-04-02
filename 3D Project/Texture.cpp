#include "Texture.h"
#include <fstream>

Texture::Texture(ID3D11Device* device, UINT height, UINT width, const void* dataLocation)
{
	D3D11_TEXTURE2D_DESC textureDescription;
	textureDescription.ArraySize = 1;
	textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDescription.CPUAccessFlags = 0;
	textureDescription.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	textureDescription.Height = height;
	textureDescription.Width = width;
	textureDescription.MipLevels = 1;
	textureDescription.MiscFlags = 0;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;
	textureDescription.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA textureData;
	textureData.pSysMem = dataLocation;
	textureData.SysMemPitch = width * 3 * sizeof(float);
	textureData.SysMemSlicePitch = 0;
	
	device->CreateTexture2D(&textureDescription, &textureData, &texture);

	D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc;
	textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureViewDesc.Format = textureDescription.Format;
	textureViewDesc.Texture2D.MipLevels = 1;
	textureViewDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(texture, &textureViewDesc, &textureView);
}

Texture* Texture::CreateTexture(ID3D11Device* device, const char* textureFileName)
{
	std::ifstream textureFile;
	textureFile.open(textureFileName, std::ios::binary | std::ios::in);

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
	typedef struct
	{
		float r, g, b;
	} tgaPixel;

	tgaHeader textureHeader;

	textureFile.read((char*)&textureHeader, 18);

	tgaPixel* imageData = new tgaPixel[textureHeader.width * textureHeader.height];

	// read pixel data
	int column = 0;
	int row = textureHeader.height - 1;
	unsigned char readR, readG, readB, discard;
	unsigned char header;
	while (!textureFile.read((char*)&header, 1).eof())
	{
		if ((header & 0x80) == 0x80) // run-length packet
		{
			textureFile.read((char*)&readB, 1);
			textureFile.read((char*)&readG, 1);
			textureFile.read((char*)&readR, 1);
			textureFile.read((char*)&discard, 1);

			for (int i = 0; i < (header & 0x7F) + 1; i++)
			{
				imageData[row * textureHeader.width + column].r = ((float)readR) / 255;
				imageData[row * textureHeader.width + column].g = ((float)readG) / 255;
				imageData[row * textureHeader.width + column].b = ((float)readB) / 255;

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
				textureFile.read((char*)&discard, 1);

				imageData[row * textureHeader.width + column].r = ((float)readR) / 255;
				imageData[row * textureHeader.width + column].g = ((float)readG) / 255;
				imageData[row * textureHeader.width + column].b = ((float)readB) / 255;

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

	Texture* returnTexture = new Texture(device, textureHeader.height, textureHeader.width, imageData);

	delete[] imageData;

	return returnTexture;
}

Texture::~Texture()
{
	if (texture != nullptr)
	{
		texture->Release();
	}
	if (texture != nullptr)
	{
		textureView->Release();
	}
}