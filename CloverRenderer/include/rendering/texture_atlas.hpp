#pragma once
#include <d3d11.h>
#include <directxmath.h>
#include <memory>
#include <string>
#include <resources/resource_manager.hpp>

using namespace DirectX;

#define ATLAS_MAX_SIZE 4096

namespace clvr
{
    namespace {
        // Canonical key: forward slashes, lowercase, no leading "./"
        std::string NormalizeKey(std::string s)
        {
            std::replace(s.begin(), s.end(), '\\', '/');
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (s.rfind("./", 0) == 0) s.erase(0, 2);
            return s;
        }
    }

    struct AtlasRegion
    {
        XMFLOAT4 uvRect; // x, y, width, height in UV space
        int pixelW = 0;
        int pixelH = 0;
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

        static std::string MakeKey(ResourceManager::Directory dir, const std::string& relativeName)
        {
            return std::to_string(static_cast<int>(dir)) + ":" + NormalizeKey(relativeName);
        }
        int AddTexture(const ResourceManager::Directory dir, const std::string& relativeName);
        bool TryGetRegion(const ResourceManager::Directory dir, const std::string& relativeName, AtlasRegion& out) const;
        AtlasRegion GetRegion(const ResourceManager::Directory dir, const std::string& relativeName) const;
        bool Build();

        ID3D11ShaderResourceView* GetSRV() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}