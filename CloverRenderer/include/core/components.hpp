#pragma once
#include <rendering/render_components.hpp>

namespace clvr
{
    struct SpriteComponent
    {
        Sprite sprite;   // reuse your existing Sprite struct as the payload
    };
}