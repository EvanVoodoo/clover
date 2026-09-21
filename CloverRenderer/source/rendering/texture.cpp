#include "rendering/texture.hpp"

#include <DirectXTex.h>

using namespace clvr;
using namespace DirectX;

Texture::Texture()
	: Resource(ResourceType::Texture)
{
}

Texture::~Texture()
{
    Shutdown();
}

bool Texture::Load(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const wchar_t* filename)
{
    ScratchImage image;
    HRESULT result = LoadFromWICFile(filename, WIC_FLAGS_NONE, nullptr, image);
    if (FAILED(result))
        return false;

    DXGI_FORMAT originalFormat = image.GetMetadata().format;

    ScratchImage converted;
    const ScratchImage* source = &image;

    // Same normalization TextureAtlas::Build() does: fold BGRA and anything
    // else into a single known format before uploading to the GPU.
    bool needsSwap = (originalFormat == DXGI_FORMAT_B8G8R8A8_UNORM ||
        originalFormat == DXGI_FORMAT_B8G8R8X8_UNORM);

    if (originalFormat != DXGI_FORMAT_R8G8B8A8_UNORM && !needsSwap)
    {
        result = Convert(*source->GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM,
            TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
        if (FAILED(result))
            return false;
        source = &converted;
        needsSwap = false; // Convert() already normalized channel order
    }

    const Image* img = source->GetImage(0, 0, 0);
    m_width = static_cast<int>(img->width);
    m_height = static_cast<int>(img->height);

    std::vector<uint8_t> pixels(m_width * m_height * 4);

    for (int row = 0; row < m_height; row++)
    {
        uint8_t* dest = pixels.data() + row * m_width * 4;
        const uint8_t* src = img->pixels + row * img->rowPitch;

        if (needsSwap)
        {
            for (int col = 0; col < m_width; col++)
            {
                dest[col * 4 + 0] = src[col * 4 + 2];
                dest[col * 4 + 1] = src[col * 4 + 1];
                dest[col * 4 + 2] = src[col * 4 + 0];
                dest[col * 4 + 3] = src[col * 4 + 3];
            }
        }
        else
        {
            memcpy(dest, src, m_width * 4);
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
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = m_width * 4;

    result = device->CreateTexture2D(&texDesc, &initData, &m_texture);
    if (FAILED(result))
        return false;

    result = device->CreateShaderResourceView(m_texture.Get(), nullptr, &m_srv);
    if (FAILED(result))
        return false;

    return true;
}

void Texture::Shutdown()
{
    m_srv.Reset();
    m_texture.Reset();
}