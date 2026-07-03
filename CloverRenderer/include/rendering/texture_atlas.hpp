#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <memory>

using namespace DirectX;

#define ATLAS_MAX_SIZE 4096

namespace clvr
{
    struct AtlasRegion
    {
        XMFLOAT4 uvRect; // x, y, width, height in UV space
    };

    class TextureAtlas
    {
    public:
        TextureAtlas();
        ~TextureAtlas();                                   // defined in .cpp

        TextureAtlas(const TextureAtlas&) = delete;        // atlas owns GPU resources — move-only
        TextureAtlas& operator=(const TextureAtlas&) = delete;
        TextureAtlas(TextureAtlas&&) noexcept;             // defined in .cpp
        TextureAtlas& operator=(TextureAtlas&&) noexcept;  // defined in .cpp

        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int width, int height);
        void Shutdown();

        int AddTexture(const wchar_t* filename);
        AtlasRegion GetRegion(const wchar_t* filename) const;
        bool Build();

        ID3D11ShaderResourceView* GetSRV() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}