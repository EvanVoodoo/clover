#include "rendering/texture_atlas.hpp"

#include <DirectXTex.h>
#include <vector>
#include <map>
#include <string>

#define STB_RECT_PACK_IMPLEMENTATION
#include "rendering/stb_rect_pack.h"

using namespace clvr;

struct TextureAtlas::Impl
{
    struct PendingImage
    {
        ScratchImage image;
        int width = 0;
        int height = 0;
    };

    int width = 0;
    int height = 0;

    std::vector<PendingImage> pendingImages;
    std::vector<AtlasRegion> regions;
    std::map<std::wstring, int> filenameToIndex;

    std::vector<uint8_t> atlasData;
    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
};

TextureAtlas::TextureAtlas() : m_impl(std::make_unique<Impl>()) {}
TextureAtlas::~TextureAtlas() = default;
TextureAtlas::TextureAtlas(TextureAtlas&&) noexcept = default;
TextureAtlas& TextureAtlas::operator=(TextureAtlas&&) noexcept = default;

bool TextureAtlas::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int width, int height)
{
    m_impl->device = device;
    m_impl->deviceContext = deviceContext;
    m_impl->width = width;
    m_impl->height = height;
    return true;
}

void TextureAtlas::Shutdown()
{
    if (m_impl->srv)
    {
        m_impl->srv->Release();
        m_impl->srv = nullptr;
    }
    if (m_impl->texture)
    {
        m_impl->texture->Release();
        m_impl->texture = nullptr;
    }
}

int TextureAtlas::AddTexture(const wchar_t* filename)
{
    Impl::PendingImage pendingImage;
    HRESULT result = LoadFromWICFile(filename, WIC_FLAGS_NONE, nullptr, pendingImage.image);
    if (FAILED(result))
        return -1;

    const Image* img = pendingImage.image.GetImage(0, 0, 0);
    pendingImage.width = static_cast<int>(img->width);
    pendingImage.height = static_cast<int>(img->height);

    int index = static_cast<int>(m_impl->pendingImages.size());
    m_impl->pendingImages.push_back(std::move(pendingImage));
    m_impl->regions.push_back(AtlasRegion{ XMFLOAT4(0, 0, 0, 0) });
    m_impl->filenameToIndex[filename] = index;
    return index;
}

AtlasRegion TextureAtlas::GetRegion(const wchar_t* filename) const
{
    auto it = m_impl->filenameToIndex.find(filename);
    if (it == m_impl->filenameToIndex.end())
        return AtlasRegion{ XMFLOAT4(0, 0, 0, 0) };
    return m_impl->regions[it->second];
}

// Helper function to extend the padding of a sprite in the atlas by copying edge pixels and setting alpha to 0
void ExtendSpritePadding(uint8_t* atlas, int atlasStride, int x, int y, int w, int h)
{
    auto pixelAt = [&](int px, int py) -> uint8_t* {
        return atlas + (py * atlasStride + px) * 4;
    };

    // Top / bottom edges
    for (int col = x; col < x + w; ++col)
    {
        uint8_t* dstTop = pixelAt(col, y - 1);
        memcpy(dstTop, pixelAt(col, y), 4);
        dstTop[3] = 0;

        uint8_t* dstBottom = pixelAt(col, y + h);
        memcpy(dstBottom, pixelAt(col, y + h - 1), 4);
        dstBottom[3] = 0;
    }

    // Left / right edges
    for (int row = y; row < y + h; ++row)
    {
        uint8_t* dstLeft = pixelAt(x - 1, row);
        memcpy(dstLeft, pixelAt(x, row), 4);
        dstLeft[3] = 0;

        uint8_t* dstRight = pixelAt(x + w, row);
        memcpy(dstRight, pixelAt(x + w - 1, row), 4);
        dstRight[3] = 0;
    }

    // Corners
    auto setCorner = [&](int cx, int cy, int srcx, int srcy) {
        uint8_t* dst = pixelAt(cx, cy);
        memcpy(dst, pixelAt(srcx, srcy), 4);
        dst[3] = 0;
    };
    setCorner(x - 1, y - 1, x, y);
    setCorner(x + w, y - 1, x + w - 1, y);
    setCorner(x - 1, y + h, x, y + h - 1);
    setCorner(x + w, y + h, x + w - 1, y + h - 1);
}

bool TextureAtlas::Build()
{
    auto& pendingImages = m_impl->pendingImages;
    auto& regions = m_impl->regions;
    const int width = m_impl->width;
    const int height = m_impl->height;

    std::vector<stbrp_rect> rects(pendingImages.size());
    for (int i = 0; i < static_cast<int>(pendingImages.size()); i++)
    {
        rects[i].id = i;
        rects[i].w = pendingImages[i].width + 2;
        rects[i].h = pendingImages[i].height + 2;
    }

    std::vector<stbrp_node> nodes(width);
    stbrp_context context;
    stbrp_init_target(&context, width, height, nodes.data(), width);

    stbrp_pack_rects(&context, rects.data(), static_cast<int>(rects.size()));

    for (size_t i = 0; i < rects.size(); i++)
    {
        regions[i].uvRect = XMFLOAT4(
            (float)(rects[i].x + 1) / width,
            (float)(rects[i].y + 1) / height,
            (float)pendingImages[i].width / width,
            (float)pendingImages[i].height / height
        );
    }

    m_impl->atlasData.resize(width * height * 4, 0);
    auto& atlasData = m_impl->atlasData;

    for (int i = 0; i < (int)pendingImages.size(); i++)
    {
        DXGI_FORMAT originalFormat = pendingImages[i].image.GetMetadata().format;

        ScratchImage converted;
        const ScratchImage* source = &pendingImages[i].image;

        if (originalFormat != DXGI_FORMAT_R8G8B8A8_UNORM && originalFormat != DXGI_FORMAT_B8G8R8A8_UNORM && originalFormat != DXGI_FORMAT_B8G8R8X8_UNORM)
        {
            HRESULT hr = Convert(*source->GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM,
                TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
            if (FAILED(hr))
                return false;
            source = &converted;
        }

        const Image* img = source->GetImage(0, 0, 0);
        int destX = rects[i].x + 1;
        int destY = rects[i].y + 1;

        bool needsSwap = (originalFormat == DXGI_FORMAT_B8G8R8A8_UNORM ||
            originalFormat == DXGI_FORMAT_B8G8R8X8_UNORM);

        for (int row = 0; row < pendingImages[i].height; row++)
        {
            uint8_t* dest = atlasData.data() + ((destY + row) * width + destX) * 4;
            const uint8_t* src = img->pixels + row * img->rowPitch;

            if (needsSwap)
            {
                for (int col = 0; col < pendingImages[i].width; col++)
                {
                    dest[col * 4 + 0] = src[col * 4 + 2];
                    dest[col * 4 + 1] = src[col * 4 + 1];
                    dest[col * 4 + 2] = src[col * 4 + 0];
                    dest[col * 4 + 3] = src[col * 4 + 3];
                }
            }
            else
            {
                memcpy(dest, src, pendingImages[i].width * 4);
            }
        }

        // NEW: extend a 1-texel border around this sprite using its own edge pixels, alpha zeroed
        ExtendSpritePadding(atlasData.data(), width, destX, destY,
                            pendingImages[i].width, pendingImages[i].height);
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = atlasData.data();
    initData.SysMemPitch = width * 4;

    HRESULT result = m_impl->device->CreateTexture2D(&texDesc, &initData, &m_impl->texture);
    if (FAILED(result))
        return false;

    result = m_impl->device->CreateShaderResourceView(m_impl->texture, nullptr, &m_impl->srv);
    if (FAILED(result))
        return false;

    return true;
}

ID3D11ShaderResourceView* TextureAtlas::GetSRV() const
{
    return m_impl->srv;
}