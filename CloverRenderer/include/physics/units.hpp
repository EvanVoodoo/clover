#pragma once

#include <Box2D/box2d.h>

namespace clvr::Units
{
    constexpr float PixelsPerMeter = 32.0f;

    constexpr float ToMeters(float px) { return px / PixelsPerMeter; }
    constexpr float ToPixels(float m) { return m * PixelsPerMeter; }

    inline b2Vec2 ToMeters(b2Vec2 px) { return { px.x / PixelsPerMeter, px.y / PixelsPerMeter }; }
    inline b2Vec2 ToPixels(b2Vec2 m) { return { m.x * PixelsPerMeter, m.y * PixelsPerMeter }; }
}