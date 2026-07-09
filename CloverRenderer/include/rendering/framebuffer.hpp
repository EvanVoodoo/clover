#pragma once

#include <d3d11.h>

namespace clvr
{
    class Framebuffer
    {
    public:
        Framebuffer();
        ~Framebuffer();

        bool Initialize(ID3D11Device* device, int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
        bool Resize(ID3D11Device* device, int width, int height);
        void Shutdown();
        void Bind(ID3D11DeviceContext* deviceContext);

        ID3D11RenderTargetView* GetRTV() const;
        ID3D11ShaderResourceView* GetSRV() const;

    private:
        ID3D11Texture2D* m_texture;
        ID3D11RenderTargetView* m_renderTargetView;
        ID3D11ShaderResourceView* m_srv;
        DXGI_FORMAT m_format;
    };
}