#pragma once

#include "rendering/render_components.hpp"
#include <d3d11.h>

namespace clvr
{
    constexpr int MAX_SPRITES = 1000;
    constexpr int MAX_VERTICES = MAX_SPRITES * 4;
    constexpr int MAX_INDICES = MAX_SPRITES * 6;

    class SpriteBatcher
    {
    public:
        SpriteBatcher();
        SpriteBatcher(const SpriteBatcher&);
        ~SpriteBatcher();

        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
        void Shutdown();

        void Begin();
        void DrawSprite(const Sprite& sprite);
        void End();

    private:
        bool InitializeBuffers();

        ID3D11Device* m_device;
        ID3D11DeviceContext* m_deviceContext;

        ID3D11Buffer* m_vertexBuffer;
        ID3D11Buffer* m_indexBuffer;

        Vertex* m_vertexBufferBase;
        Vertex* m_vertexBufferPtr;
        int m_indexCount;
    };
}