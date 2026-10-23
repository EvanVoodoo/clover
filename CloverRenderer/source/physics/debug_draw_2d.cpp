#include "physics/debug_draw_2d.hpp"
#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <rendering/renderer.hpp>
#include "physics/units.hpp"

namespace clvr {
	DebugDraw2D::DebugDraw2D() { Bind(); }

	void DebugDraw2D::Bind()
	{
		static_cast<b2DebugDraw&>(*this) = b2DefaultDebugDraw(); // sane defaults
		context = this;

		// Box2D doesn't null-check these, so assign every one
		DrawPolygonFcn = DrawPolygon;
		DrawSolidPolygonFcn = DrawSolidPolygon;
		DrawCircleFcn = DrawCircle;
		DrawSolidCircleFcn = DrawSolidCircle;
		DrawSolidCapsuleFcn = DrawSolidCapsule;
		DrawSegmentFcn = DrawSegment;
		DrawTransformFcn = DrawTransform;
		DrawPointFcn = DrawPoint;
		DrawStringFcn = DrawString;

		drawShapes = true;
		drawJoints = true;
	}

	void DebugDraw2D::Begin()
	{
		m_lines.clear();
	}

	void DebugDraw2D::End()
	{
		auto ecs = Engine.GetECS();
		ecs->GetSystem<Renderer>().SubmitDebugLines(m_lines);
		m_lines.clear();
	}

	DebugDraw2D& DebugDraw2D::Self(void* ctx)
	{
		return *static_cast<DebugDraw2D*>(ctx);
	}

	void DebugDraw2D::DrawPolygon(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context)
	{
		for (int i = 0; i < vertexCount; ++i)
		{
			b2Vec2 a = vertices[i];
			b2Vec2 b = vertices[(i + 1) % vertexCount];
			Self(context).AddLine(a, b, color);
		}
	}

	void DebugDraw2D::DrawSolidPolygon(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context)
	{
		for (int i = 0; i < vertexCount; ++i)
		{
			b2Vec2 a = b2TransformPoint(transform, vertices[i]);
			b2Vec2 b = b2TransformPoint(transform, vertices[(i + 1) % vertexCount]);
			Self(context).AddLine(a, b, color);
		}
	}

	void DebugDraw2D::DrawCircle(b2Vec2 center, float radius, b2HexColor color, void* context)
	{
		const int segments = 16;
		for (int i = 0; i < segments; ++i)
		{
			float theta1 = (float) i / segments * 2.0f * 3.1415927f;
			float theta2 = (float) (i + 1) / segments * 2.0f * 3.1415927f;
			b2Vec2 a = { center.x + radius * cosf(theta1), center.y + radius * sinf(theta1) };
			b2Vec2 b = { center.x + radius * cosf(theta2), center.y + radius * sinf(theta2) };
			Self(context).AddLine(a, b, color);
		}
	}

	void DebugDraw2D::DrawSolidCircle(b2Transform transform, float radius, b2HexColor color, void* context)
	{
		const int segments = 16;
		for (int i = 0; i < segments; ++i)
		{
			float theta1 = (float) i / segments * 2.0f * 3.1415927f;
			float theta2 = (float) (i + 1) / segments * 2.0f * 3.1415927f;
			b2Vec2 a = b2TransformPoint(transform, { radius * cosf(theta1), radius * sinf(theta1) });
			b2Vec2 b = b2TransformPoint(transform, { radius * cosf(theta2), radius * sinf(theta2) });
			Self(context).AddLine(a, b, color);
		}
	}

	void DebugDraw2D::DrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context)
	{
		// to be implemented: draw a capsule shape as two circles and a rectangle connecting them
	}

	void DebugDraw2D::DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
	{
		Self(context).AddLine(p1, p2, color);
	}

	void DebugDraw2D::DrawTransform(b2Transform transform, void* context)
	{
		// to be implemented: draw the transform axes
	}

	void DebugDraw2D::DrawPoint(b2Vec2 p, float size, b2HexColor color, void* context)
	{
		Self(context).AddLine(p - b2Vec2(0.5f), p + b2Vec2(0.5f), color);
	}

	void DebugDraw2D::DrawString(b2Vec2 p, const char* s, b2HexColor color, void* context)
	{
		// to be implemented: draw text at position p
	}

	void DebugDraw2D::DrawShape(b2ShapeId shape, b2HexColor color)
	{
		if (!b2Shape_IsValid(shape))
			return;

		// Geometry is in body-local space, so get the body's transform
		b2Transform xf = b2Body_GetTransform(b2Shape_GetBody(shape));

		switch (b2Shape_GetType(shape))
		{
			case b2_polygonShape:
			{
				b2Polygon p = b2Shape_GetPolygon(shape);
				DrawSolidPolygon(xf, p.vertices, p.count, p.radius, color, this);
				break;
			}
			case b2_circleShape:
			{
				b2Circle c = b2Shape_GetCircle(shape);
				DrawCircle(b2TransformPoint(xf, c.center), c.radius, color, this);
				break;
			}
			case b2_capsuleShape:
			{
				b2Capsule c = b2Shape_GetCapsule(shape);
				DrawSolidCapsule(b2TransformPoint(xf, c.center1), b2TransformPoint(xf, c.center2), c.radius, color, this);
				break;
			}
			case b2_segmentShape:
			{
				b2Segment s = b2Shape_GetSegment(shape);
				DrawSegment(b2TransformPoint(xf, s.point1), b2TransformPoint(xf, s.point2), color, this);
				break;
			}
			default:
				break; // chain segments etc. not handled
		}
	}

	void DebugDraw2D::AddLine(b2Vec2 a, b2Vec2 b, b2HexColor c)
	{
		DirectX::XMFLOAT4 color = {
			((c >> 16) & 0xFF) / 255.0f,
			((c >> 8) & 0xFF) / 255.0f,
			(c & 0xFF) / 255.0f,
			1.0f
		};
		XMFLOAT2 aPos = { Units::ToPixels(a.x), Units::ToPixels(a.y) };
		XMFLOAT2 bPos = { Units::ToPixels(b.x), Units::ToPixels(b.y) };
		m_lines.push_back({ aPos, bPos, color });
	}
}