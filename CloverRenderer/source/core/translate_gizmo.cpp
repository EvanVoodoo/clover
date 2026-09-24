#include "core/translate_gizmo.hpp"
#include "core/engine.hpp"
#include <rendering/renderer.hpp>

namespace clvr {
	TranslateGizmo::TranslateGizmo()
		: Gizmo()
	{
		Initialize(
			Engine.GetResourceManager()->Load<Texture>(Engine.GetResourceManager()->GetPath(ResourceManager::Directory::SharedAssets, "textures/DragArrow.png")),
			Engine.GetResourceManager()->Load<Texture>(Engine.GetResourceManager()->GetPath(ResourceManager::Directory::SharedAssets, "textures/DragArrow.png"))
		);
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
		renderer.DrawUnbatchedSprite(m_xAxisParams->sprite, m_xAxisParams->transform, *renderer.FindSpriteLayer(-99));

		if (!m_oneAxis)
			renderer.DrawUnbatchedSprite(m_yAxisParams->sprite, m_yAxisParams->transform, *renderer.FindSpriteLayer(-99));
	}
}