#include "rendering/texture_atlas.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "rendering/stb_rect_pack.h"

using namespace clvr;

TextureAtlas::TextureAtlas()
{
	m_width = 0;
	m_height = 0;
	m_texture = nullptr;
	m_srv = nullptr;
	m_device = nullptr;
	m_deviceContext = nullptr;
}

TextureAtlas::TextureAtlas(const TextureAtlas&)
{
}

TextureAtlas::~TextureAtlas()
{
}

bool TextureAtlas::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int width, int height)
{
	m_device = device;
	m_deviceContext = deviceContext;
	m_width = width;
	m_height = height;
	return true;
}

void TextureAtlas::Shutdown()
{
	if (m_srv)
	{
		m_srv->Release();
		m_srv = nullptr;
	}
	if (m_texture)
	{
		m_texture->Release();
		m_texture = nullptr;
	}
}

int TextureAtlas::AddTexture(const wchar_t* filename)
{
	PendingImage pendingImage;
	HRESULT result = LoadFromWICFile(filename, WIC_FLAGS_NONE, nullptr, pendingImage.image);
	if (FAILED(result))
		return -1;

	const Image* img = pendingImage.image.GetImage(0, 0, 0);
	pendingImage.width = static_cast<int>(img->width);
	pendingImage.height = static_cast<int>(img->height);

	int index = static_cast<int>(m_pendingImages.size());
	m_pendingImages.push_back(std::move(pendingImage));
	m_regions.push_back(AtlasRegion{ XMFLOAT4(0, 0, 0, 0) }); // placeholder, filled in by Build
	m_filenameToIndex[filename] = index;
	return index;
}

AtlasRegion TextureAtlas::GetRegion(const wchar_t* filename) const
{
	auto it = m_filenameToIndex.find(filename);
	if (it == m_filenameToIndex.end())
		return AtlasRegion{ XMFLOAT4(0, 0, 0, 0) };
	return m_regions[it->second];
}

bool TextureAtlas::Build()
{
	std::vector<stbrp_rect> rects(m_pendingImages.size());
	for (int i = 0; i < static_cast<int>(m_pendingImages.size()); i++)
	{
		rects[i].id = i;
		rects[i].w = m_pendingImages[i].width;
		rects[i].h = m_pendingImages[i].height;
	}

	std::vector<stbrp_node> nodes(m_width);
	stbrp_context context;
	stbrp_init_target(&context, m_width, m_height, nodes.data(), m_width);

	stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size()));

	for (size_t i = 0; i < rects.size(); i++)
	{
		m_regions[i].uvRect = XMFLOAT4(
			(float)rects[i].x / m_width,
			(float)rects[i].y / m_height,
			(float)m_pendingImages[i].width / m_width,
			(float)m_pendingImages[i].height / m_height
		);
	}

	m_atlasData.resize(m_width * m_height * 4, 0);

	for (int i = 0; i < (int)m_pendingImages.size(); i++)
	{
		DXGI_FORMAT originalFormat = m_pendingImages[i].image.GetMetadata().format;

		ScratchImage converted;
		const ScratchImage* source = &m_pendingImages[i].image;

		if (originalFormat != DXGI_FORMAT_R8G8B8A8_UNORM && originalFormat != DXGI_FORMAT_B8G8R8A8_UNORM && originalFormat != DXGI_FORMAT_B8G8R8X8_UNORM)
		{
			HRESULT hr = Convert(*source->GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM,
				TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
			if (FAILED(hr))
				return false;
			source = &converted;
		}

		const Image* img = source->GetImage(0, 0, 0);
		int destX = rects[i].x;
		int destY = rects[i].y;

		bool needsSwap = (originalFormat == DXGI_FORMAT_B8G8R8A8_UNORM ||
			originalFormat == DXGI_FORMAT_B8G8R8X8_UNORM);

		for (int row = 0; row < m_pendingImages[i].height; row++)
		{
			uint8_t* dest = m_atlasData.data() + ((destY + row) * m_width + destX) * 4;
			const uint8_t* src = img->pixels + row * img->rowPitch;

			if (needsSwap)
			{
				for (int col = 0; col < m_pendingImages[i].width; col++)
				{
					dest[col * 4 + 0] = src[col * 4 + 2];
					dest[col * 4 + 1] = src[col * 4 + 1];
					dest[col * 4 + 2] = src[col * 4 + 0];
					dest[col * 4 + 3] = src[col * 4 + 3];
				}
			}
			else
			{
				memcpy(dest, src, m_pendingImages[i].width * 4);
			}
		}
	}

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = m_width;
	texDesc.Height = m_height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = m_atlasData.data();
	initData.SysMemPitch = m_width * 4;

	HRESULT result = m_device->CreateTexture2D(&texDesc, &initData, &m_texture);
	if (FAILED(result))
		return false;

	result = m_device->CreateShaderResourceView(m_texture, nullptr, &m_srv);
	if (FAILED(result))
		return false;

	return true;
}

ID3D11ShaderResourceView* TextureAtlas::GetSRV() const
{
	return m_srv;
}