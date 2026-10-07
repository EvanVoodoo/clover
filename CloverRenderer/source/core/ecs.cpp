#include "core/ecs.hpp"

using namespace clvr;
using namespace std;

constexpr float kMaxDeltaTime = 1.0f / 30.0f;

EntityComponentSystem::EntityComponentSystem() = default;

EntityComponentSystem::~EntityComponentSystem() = default;

void EntityComponentSystem::SetActiveRegistry(EngineMode mode)
{
	if (mode == EngineMode::Editing)
		m_active = &m_editRegistry;
	else if (mode == EngineMode::Playing)
		m_active = &m_playRegistry;
	else
		assert(false && "Invalid EditorMode");
}

void EntityComponentSystem::DeleteEntity(Entity e)
{
    assert(m_active->valid(e));

    // mark this entity for deletion
    m_active->emplace_or_replace<Delete>(e);
}

void EntityComponentSystem::UpdateSystems(float dt)
{
    dt = std::min(dt, kMaxDeltaTime);
    for (auto& s : m_systems) s->Update(dt);
}

void EntityComponentSystem::RenderSystems()
{
    for (auto& s : m_systems) s->Render();
}

void EntityComponentSystem::InspectSystems(float dt)
{
	for (auto& s : m_systems) s->Inspect(dt);
}

void EntityComponentSystem::RemoveDeleted()
{
    auto& deleteStorage = m_active->storage<Delete>();
    while (!deleteStorage.empty())
    {
        // Destroying entities may enqueue more Deletes, hence the loop
        const auto del = m_active->view<Delete>();
        m_active->destroy(del.begin(), del.end());
    }
}

