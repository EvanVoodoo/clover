#include "core/translate_gizmo.hpp"
#include "core/engine.hpp"
#include <rendering/renderer.hpp>
#include "rendering/texture.hpp"

namespace clvr {
	TranslateGizmo::TranslateGizmo()
		: Gizmo()
	{
		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		auto texture = Engine.GetResourceManager()->Load<Texture>(static_cast<ID3D11Device*>(renderer.GetNativeDeviceHandle()), Engine.GetResourceManager()->GetPath(ResourceManager::Directory::SharedAssets, "textures/DragArrow.png"));
		Initialize(
			texture,
			texture
		);

		m_xAxisParams->color = { 0.0f, 1.0f, 0.0f, 1.0f }; // green
		m_yAxisParams->color = { 1.0f, 0.0f, 0.0f, 1.0f }; // red
		
		m_xAxisParams->hoverColor = { 1.0f, 1.0f, 0.0f, 1.0f }; // yellow
		m_yAxisParams->hoverColor = { 1.0f, 1.0f, 0.0f, 1.0f }; // yellow

		// still need to link offset with gizmo position
		m_xAxisParams->offset = { m_xAxisParams->sprite.size.x / 2.0f, 0.0f };
		m_yAxisParams->offset = { 0.0f, m_yAxisParams->sprite.size.x / 2.0f };
	}
	TranslateGizmo::TranslateGizmo(const GizmoAxisParams& xAxisParams, const GizmoAxisParams& yAxisParams, bool oneAxis)
	{
	}
	void TranslateGizmo::Update(const Camera& camera)
	{
		Gizmo::Update(camera);

		if (m_selectedEntity == entt::null)
		{
			Hide();
			return;
		}

		Show();

		Entity selectedEntity = m_selectedEntity;

		if (Engine.GetECS()->GetRegistry().all_of<Transform>(selectedEntity) == false)
		{
			Hide();
			return;
		}
		auto& selectedTransform = Engine.GetECS()->GetRegistry().get<Transform>(selectedEntity);
		selectedTransform.position.x += GetDeltaX(); // add scaling factor based on camera zoom
		selectedTransform.position.y += GetDeltaY(); // add scaling factor based on camera zoom

		SetGizmoPosition(selectedEntity);

		ExamineMousePosition();
	}

	void TranslateGizmo::Draw(const Camera& camera)
	{
		if (m_hidden || m_selectedEntity == entt::null)
			return;

		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		renderer.DrawUnbatchedSprite(m_xAxisParams->sprite, m_xAxisParams->transform, *renderer.FindOrCreateSpriteLayer(-1));

		if (!m_oneAxis)
			renderer.DrawUnbatchedSprite(m_yAxisParams->sprite, m_yAxisParams->transform, *renderer.FindOrCreateSpriteLayer(-1));
	}
}