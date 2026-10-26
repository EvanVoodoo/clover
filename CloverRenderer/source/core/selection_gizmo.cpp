#include "core/selection_gizmo.hpp"

#include "core/engine.hpp"
#include <rendering/renderer.hpp>
#include "rendering/texture.hpp"

namespace clvr 
{
	SelectionGizmo::SelectionGizmo()
		: Gizmo()
	{
		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		auto texture = Engine.GetResourceManager()->Load<Texture>(static_cast<ID3D11Device*>(renderer.GetNativeDeviceHandle()), Engine.GetResourceManager()->GetPath(ResourceManager::Directory::SharedAssets, "textures/select_widget.png"));
		Initialize(
			texture,
			texture
		);

		m_xAxisParams->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // regular color

		m_xAxisParams->hoverColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // dont change color on hover

		float scaleFactor = 1.8f; // scale up the gizmo for better visibility
		m_xAxisParams->transform.scale = { scaleFactor, scaleFactor };
		m_xAxisParams->offset = { m_xAxisParams->sprite.size.x * scaleFactor / 2.0f, m_xAxisParams->sprite.size.y * scaleFactor / 2.0f };

		m_oneAxis = true; // only one axis for selection gizmo
	}

	SelectionGizmo::SelectionGizmo(const GizmoAxisParams& xAxisParams, const GizmoAxisParams& yAxisParams, bool oneAxis)
		: Gizmo(xAxisParams, yAxisParams, oneAxis)
	{
	}

	void SelectionGizmo::Update(const Camera& camera)
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

	void SelectionGizmo::Draw(const Camera& camera)
	{
		if (m_hidden || m_selectedEntity == entt::null)
			return;

		Gizmo::Draw(camera);

		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		const float invZoom = 1.0f / renderer.GetActiveCamera().zoom; // use however you reach the renderer

		Sprite gizmoSpriteX = m_xAxisParams->sprite;
		gizmoSpriteX.size.x *= invZoom;
		gizmoSpriteX.size.y *= invZoom;
		renderer.DrawUnbatchedSprite(gizmoSpriteX, m_xAxisParams->transform, *renderer.FindOrCreateSpriteLayer(-1), false);
	}
}