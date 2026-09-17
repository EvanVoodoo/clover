#include "core/gizmo.hpp"

#include "core/transform.hpp"
#include <core/engine.hpp>
#include "core/ecs.hpp"
#include <rendering/texture.hpp>

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

	void Gizmo::Initialize(const Texture& xAxisTexture, const Texture& yAxisTexture)
	{
		// Setup x axis
		Sprite xSprite;
		// green color for x axis
		xSprite.color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
		xSprite.size = { static_cast<float>(xAxisTexture.GetWidth()), static_cast<float>(xAxisTexture.GetHeight()) };
		m_xAxisParams->sprite = xSprite;

		// Setup y axis
		if (!m_oneAxis)
		{
			Sprite ySprite;
			// red color for y axis
			ySprite.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
			ySprite.size = { static_cast<float>(yAxisTexture.GetWidth()), static_cast<float>(yAxisTexture.GetHeight()) };
			m_yAxisParams->sprite = ySprite;
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

	void Gizmo::SetGizmoPosition(Entity selectedEntity)
	{
		auto& registry = Engine.GetECS()->GetRegistry();
		if (!registry.valid(selectedEntity))
			return;
		
		float spriteWidth = 0.0f;
		float spriteHeight = 0.0f;

		SpriteComponent* spriteComponent = registry.try_get<SpriteComponent>(selectedEntity);
		if (const auto* spriteComponent = registry.try_get<SpriteComponent>(selectedEntity))
		{
			Sprite s = spriteComponent->sprite;
			spriteWidth = s.size.x;
			spriteHeight = s.size.y;
		}

		const Transform& transform = registry.get<Transform>(selectedEntity);

		XMFLOAT2 position = transform.position;
		if (!m_oneAxis)
		{
			if (m_xAxisParams)
				m_xAxisParams->transform.position = 
				XMFLOAT2(
					position.x + m_xAxisParams->offset.x + (spriteWidth * 0.5f * transform.scale.x),
					position.y + m_xAxisParams->offset.y + (spriteHeight * 0.5f * transform.scale.y) 
					- (m_xAxisParams->sprite.size.y * m_xAxisParams->transform.scale.y * 0.5f)
				);
			if (m_yAxisParams)
				m_yAxisParams->transform.position =
				XMFLOAT2(
					position.x + m_yAxisParams->offset.x + (spriteWidth * 0.5f * transform.scale.x)
					- (m_yAxisParams->sprite.size.x * m_yAxisParams->transform.scale.x * 0.5f),
					position.y + m_yAxisParams->offset.y + (spriteHeight * 0.5f * transform.scale.y)
					- (m_yAxisParams->sprite.size.y * m_yAxisParams->transform.scale.y * 0.5f)
				);
		}
		else
		{
			if (m_xAxisParams)
				m_xAxisParams->transform.position =
				XMFLOAT2(
					position.x + m_xAxisParams->offset.x + (spriteWidth * 0.5f * transform.scale.x)
					- (m_xAxisParams->sprite.size.x * m_xAxisParams->transform.scale.x * 0.5f),
					position.y + m_xAxisParams->offset.y + (spriteHeight * 0.5f * transform.scale.y)
					- (m_xAxisParams->sprite.size.y * m_xAxisParams->transform.scale.y * 0.5f)
				);
		}

		if (m_xAxisParams)
			m_xAxisParams->transform.position = position;
		if (m_yAxisParams)
			m_yAxisParams->transform.position = position;
	}
}