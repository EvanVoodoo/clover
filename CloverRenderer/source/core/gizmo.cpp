#include "core/gizmo.hpp"

#include "core/transform.hpp"
#include <core/engine.hpp>
#include "core/ecs.hpp"
#include <rendering/texture.hpp>
#include <rendering/renderer.hpp>

namespace clvr {
	Gizmo::Gizmo() 
		: Gizmo(
			[] {
				GizmoAxisParams defaultParams;
				Transform transform;
				transform.scale = XMFLOAT2(1.0f, 1.0f);
				defaultParams.transform = transform;
				defaultParams.sprite = Sprite();
				return defaultParams;
			}(),
				[] {
				GizmoAxisParams defaultParams;
				Transform transform;
				transform.scale = XMFLOAT2(1.0f, 1.0f);
				defaultParams.transform = transform;
				defaultParams.sprite = Sprite();
				return defaultParams;
				}(),
					false)
	{
	}

	Gizmo::Gizmo(const GizmoAxisParams& xAxisParams, const GizmoAxisParams& yAxisParams, bool oneAxis)
		: m_xAxisParams(nullptr),
		m_yAxisParams(nullptr),
		m_spriteBatcher(std::make_unique<SpriteBatcher>()),
		m_selectedEntity(entt::null),
		m_lastMousePos(XMINT2(0, 0)),
		m_overXAxis(false),
		m_overYAxis(false),
		m_holdingXAxis(false),
		m_holdingYAxis(false),
		m_hidden(false),
		m_oneAxis(oneAxis)
	{
		m_xAxisParams = std::make_unique<GizmoAxisParams>(xAxisParams);
		if (!oneAxis) {
			m_yAxisParams = std::make_unique<GizmoAxisParams>(yAxisParams);
		}
	}

	void Gizmo::Update(const Camera& camera)
	{
		m_cameraZoom = camera.zoom;
		GetMousePosition(m_lastMousePos.x, m_lastMousePos.y);
	}

	void Gizmo::Draw(const Camera& camera)
	{
		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		renderer.FindOrCreateSpriteLayer(-1)->parallaxFactor = m_layerParallaxFactor;
	}

	void Gizmo::SetSelectedEntity(Entity entity)
	{
		m_selectedEntity = entity;
		if (m_selectedEntity != entt::null) {
			SetGizmoPosition(entity);
		}
	}

	void Gizmo::Hide()
	{
		if (m_xAxisParams) m_xAxisParams->isHidden = true;
		if (m_yAxisParams) m_yAxisParams->isHidden = true;

		m_hidden = true;
	}

	void Gizmo::Show()
	{
		if (m_hidden && m_selectedEntity != entt::null)
		{
			if (m_xAxisParams) m_xAxisParams->isHidden = false;
			if (m_yAxisParams) m_yAxisParams->isHidden = false;

			m_hidden = false;
		}
	}

	void Gizmo::Initialize(const std::shared_ptr<Texture> xAxisTexture, const std::shared_ptr<Texture> yAxisTexture)
	{
		// Setup x axis
		// green color for x axis
		m_xAxisParams->sprite.LoadSpriteTexture(xAxisTexture);
		m_xAxisParams->sprite.size = { static_cast<float>(xAxisTexture->GetWidth()), static_cast<float>(xAxisTexture->GetHeight()) };
		m_xAxisParams->transform.rotation = 0.0f; // No rotation for x axis

		// Setup y axis
		if (!m_oneAxis)
		{
			// red color for y axis
			m_yAxisParams->sprite.LoadSpriteTexture(yAxisTexture);
			m_yAxisParams->sprite.size = { static_cast<float>(yAxisTexture->GetWidth()), static_cast<float>(yAxisTexture->GetHeight()) };
			m_yAxisParams->transform.rotation = XM_PIDIV2; // Rotate 90 degrees for y axis
		}

		Hide();
	}

	void Gizmo::ExamineMousePosition()
	{
		XMINT2 mousePos = {};
		GetMousePosition(mousePos.x, mousePos.y);

		if (!m_xAxisParams)
			return;

		const auto& xAxisTransform = m_xAxisParams->transform;
		auto& xAxisSprite = m_xAxisParams->sprite;

		if (mousePos.x >= xAxisTransform.position.x &&
			mousePos.x <= xAxisTransform.position.x + (xAxisSprite.size.x * xAxisTransform.scale.x) &&
			mousePos.y >= xAxisTransform.position.y &&
			mousePos.y <= xAxisTransform.position.y + (xAxisSprite.size.y * xAxisTransform.scale.y))
		{
			xAxisSprite.color = m_xAxisParams->hoverColor; // Highlight color
			m_overXAxis = true;
		}
		else if (m_holdingXAxis)
		{
			xAxisSprite.color = m_xAxisParams->hoverColor; // Highlight color
			m_overXAxis = true;
		}
		else if (m_holdingYAxis)
		{
			xAxisSprite.color = m_xAxisParams->disabledColor; // Disabled color
			m_overXAxis = false;
		}
		else
		{
			xAxisSprite.color = m_xAxisParams->color; // Default color
			m_overXAxis = false;
		}

		if (m_oneAxis || !m_yAxisParams)
			return;

		const auto& yAxisTransform = m_yAxisParams->transform;
		auto& yAxisSprite = m_yAxisParams->sprite;

		if (mousePos.x >= yAxisTransform.position.x &&
			mousePos.x <= yAxisTransform.position.x + (yAxisSprite.size.x * yAxisTransform.scale.x) &&
			mousePos.y >= yAxisTransform.position.y &&
			mousePos.y <= yAxisTransform.position.y + (yAxisSprite.size.y * yAxisTransform.scale.y))
		{
			yAxisSprite.color = m_yAxisParams->hoverColor; // Highlight color
			m_overYAxis = true;
		}
		else if (m_holdingYAxis)
		{
			yAxisSprite.color = m_yAxisParams->hoverColor; // Highlight color
			m_overYAxis = true;
		}
		else if (m_holdingXAxis)
		{
			yAxisSprite.color = m_yAxisParams->disabledColor; // Disabled color
			m_overYAxis = false;
		}
		else
		{
			yAxisSprite.color = m_yAxisParams->color; // Default color
			m_overYAxis = false;
		}
	}

	float Gizmo::GetDeltaX()
	{
		Input* input = Engine.GetInput();
		// TODO: mouse outside window or out of bounds check
		if (false)
			return 0.0f;
		if (!m_overXAxis && !m_holdingXAxis)
			return 0.0f;

		if (IsMouseMoving() && IsMouseButtonDown(INPUT::MOUSE_BUTTON::LEFT))
		{
			m_holdingXAxis = true;
			return std::ceil((input->GetMouseX() - m_lastMousePos.x) / m_cameraZoom);
		}

		if (WasMouseButtonJustReleased(INPUT::MOUSE_BUTTON::LEFT))
		{
			m_holdingXAxis = false;
		}

		return 0.0f;
	}

	float Gizmo::GetDeltaY()
	{
		Input* input = Engine.GetInput();
		// TODO: mouse outside window or out of bounds check
		if (m_oneAxis)
			return 0.0f;

		if (!m_overYAxis && !m_holdingYAxis)
			return 0.0f;

		if (IsMouseMoving() && IsMouseButtonDown(INPUT::MOUSE_BUTTON::LEFT))
		{
			m_holdingYAxis = true;
			return std::ceil((input->GetMouseY() - m_lastMousePos.y) / m_cameraZoom);
		}

		if (WasMouseButtonJustReleased(INPUT::MOUSE_BUTTON::LEFT))
		{
			m_holdingYAxis = false;
		}

		return 0.0f;
	}

	void Gizmo::SetGizmoPosition(entt::entity selectedEntity)
	{
		auto& registry = Engine.GetECS()->GetRegistry();
		if (!registry.valid(selectedEntity))
			return;

		const Transform* transform = registry.try_get<Transform>(selectedEntity);
		if (!transform)
			return;

		const SpriteComponent* sprite = registry.try_get<SpriteComponent>(selectedEntity);
		if (sprite) {
			const Sprite s = sprite->sprite;
			if (sprite && s.layer)
			{
				m_layerParallaxFactor = s.layer->parallaxFactor;
			}
			else m_layerParallaxFactor = 1.0f;
		}
		else m_layerParallaxFactor = 1.0f; // default parallax factor for gizmo layer

		// TODO: Needs to take the camera's position into account
		// TODO: Shadow bug with the parallax
		const XMFLOAT2 center = {
			transform->position.x,
			transform->position.y
		};

		// Half the on-screen size of an axis handle
		auto halfSize = [](const auto& axis)
			{
				return XMFLOAT2{
					axis.sprite.size.x * axis.transform.scale.x * 0.5f,
					axis.sprite.size.y * axis.transform.scale.y * 0.5f
				};
			};

		if (m_xAxisParams)
		{
			const XMFLOAT2 h = halfSize(*m_xAxisParams);
			m_xAxisParams->transform.position = {
				// Single-axis mode centers the handle; two-axis mode starts it at the center
				center.x + m_xAxisParams->offset.x - (m_oneAxis ? h.x : 0.0f),
				center.y + m_xAxisParams->offset.y - h.y
			};
		}

		if (!m_oneAxis && m_yAxisParams)
		{
			const XMFLOAT2 h = halfSize(*m_yAxisParams);
			m_yAxisParams->transform.position = {
				center.x + m_yAxisParams->offset.x - h.x,
				center.y + m_yAxisParams->offset.y - h.y
			};
		}
	}
}