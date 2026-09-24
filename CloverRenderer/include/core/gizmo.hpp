#pragma once
#include <DirectXMath.h>
#include <memory>
#include "entt/entity/fwd.hpp"
#include <rendering/sprite_batcher.hpp>

namespace clvr {
	struct Transform;
	class Texture;

	struct GizmoAxisParams {
		Transform transform;
		Sprite sprite;
		bool isHidden = true;
		bool isHovered = false;
		bool isDragging = false;
		XMFLOAT2 offset = { 0.0f, 0.0f };
		XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		XMFLOAT4 hoverColor = { 1.0f, 1.0f, 0.0f, 1.0f };
		XMFLOAT4 disabledColor = { 0.5f, 0.5f, 0.5f, 1.0f };
	};

	class Gizmo {
	public:
		Gizmo();
		Gizmo(const GizmoAxisParams& xAxisParams, const GizmoAxisParams& yAxisParams, bool oneAxis = false);

		virtual void Update(const Camera& camera);
		virtual void Draw(const Camera& camera) = 0;
		
		void SetSelectedEntity(entt::entity entity);
		void Hide();
		void Show();

		inline bool IsHovered() const { return false; }

	protected:
		void Initialize(const std::shared_ptr<Texture> xAxisTexture, const std::shared_ptr<Texture> yAxisTexture);
		void ExamineMousePosition();

		float GetDeltaX();
		float GetDeltaY();

		void SetGizmoPosition(entt::entity selectedEntity);

	protected:
		std::unique_ptr<GizmoAxisParams> m_xAxisParams;
		std::unique_ptr<GizmoAxisParams> m_yAxisParams;
		std::unique_ptr<SpriteBatcher> m_spriteBatcher;

		entt::entity m_selectedEntity;
		XMINT2 m_lastMousePos;

		bool m_overXAxis = false;
		bool m_overYAxis = false;
		bool m_holdingXAxis = false;
		bool m_holdingYAxis = false;
		bool m_hidden = false;
		bool m_oneAxis = false;

		float m_cameraZoom = 1.0f;
	};
}