#pragma once

#include <Box2D/types.h>
#include "rendering/render_common.hpp"
#include <vector>

namespace clvr {
	struct DebugDraw2D : public b2DebugDraw {
	public:
		DebugDraw2D();

		// Wire the static callbacks into the b2DebugDraw function pointers
		void Bind();

		// Call once per frame before b2World_Draw
		void Begin();
		void End();

		const std::vector<DebugLine>& Lines() const { return m_lines; }

		void DrawShape(b2ShapeId shape, b2HexColor color);

	private:
		static DebugDraw2D& Self(void* ctx);
		// Context (void*) is `this`, set in Bind()
		static void DrawPolygon(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context);
		static void DrawSolidPolygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context);
		static void DrawCircle(b2Vec2 center, float radius, b2HexColor color, void* context);
		static void DrawSolidCircle(b2Transform transform, float radius, b2HexColor color, void* context);
		static void DrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context);
		static void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context);
		static void DrawTransform(b2Transform transform, void* context);
		static void DrawPoint(b2Vec2 p, float size, b2HexColor color, void* context);
		static void DrawString(b2Vec2 p, const char* s, b2HexColor color, void* context);

		void AddLine(b2Vec2 a, b2Vec2 b, b2HexColor c);

		std::vector<DebugLine> m_lines;
	};
}