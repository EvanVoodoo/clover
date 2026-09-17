#pragma once
#include <d3d11.h>
#include <wrl/client.h>

namespace clvr
{
    using Microsoft::WRL::ComPtr;

    class Texture
    {
    public:
        Texture() = default;
        ~Texture() = default;                                // ComPtr releases automatically

        Texture(const Texture&) = delete;                     // Texture owns a GPU resource — move-only
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept = default;                // ComPtr's move ctor nulls the source
        Texture& operator=(Texture&&) noexcept = default;

        bool Load(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const wchar_t* filename);
        void Shutdown();                                       // optional — ComPtr::Reset() does this too

        ID3D11ShaderResourceView* GetSRV() const { return m_srv.Get(); }
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }

    private:
        ComPtr<ID3D11Texture2D> m_texture;
        ComPtr<ID3D11ShaderResourceView> m_srv;
        int m_width = 0;
        int m_height = 0;
    };
}