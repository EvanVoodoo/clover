#pragma once
#include <d3d11.h>
#include <DirectXTex.h>
#include <directxmath.h>
#include <vector>
#include <map>
#include <string>

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
        TextureAtlas(const TextureAtlas&);
        ~TextureAtlas();

        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int width, int height);
        void Shutdown();

        int AddTexture(const wchar_t* filename);
        AtlasRegion GetRegion(const wchar_t* filename) const;
        bool Build();

        ID3D11ShaderResourceView* GetSRV() const;

    private:
        struct PendingImage
        {
            ScratchImage image;
            int width;
            int height;
        };

        int m_width;
        int m_height;

        std::vector<PendingImage> m_pendingImages;
        std::vector<AtlasRegion> m_regions;

        std::map<std::wstring, int> m_filenameToIndex;

        std::vector<uint8_t> m_atlasData;
        ID3D11Texture2D* m_texture;
        ID3D11ShaderResourceView* m_srv;
        ID3D11Device* m_device;
        ID3D11DeviceContext* m_deviceContext;
    };
}