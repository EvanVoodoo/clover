#pragma once

#include <vector>
#include <string>
#include "entt/entity/registry.hpp"
#include "entt/meta/factory.hpp"

namespace clvr
{
	using Entity = entt::entity;

	class System
	{
	public:
		virtual ~System() = default;
		virtual void Update(float) {}
		virtual void Render() {}
		virtual void Inspect(float) {}
		int priority = 0;
		std::string title = {};

	};
	
	class EntityComponentSystem
	{
	public:
		EntityComponentSystem();
		~EntityComponentSystem();

		entt::registry& GetRegistry() { return m_registry; }
		Entity CreateEntity() { return m_registry.create(); }
		void DeleteEntity(Entity);
		void UpdateSystems(float);
		void RenderSystems();
		void InspectSystems(float);
		void RemoveDeleted();
		template <typename T, typename... Args>
		decltype(auto) CreateComponent(Entity entity, Args&&... args);
		template <typename T>
		static void RegisterComponent();
		template <typename T, typename... Args>
		T& CreateSystem(Args&&... args);
		template <typename T>
		T& GetSystem();
		template <typename T>
		std::vector<T*> GetSystems();

	private:
		entt::registry m_registry;

		struct Delete
		{
		};  // Tag component for entities to be deleted
		std::vector<std::unique_ptr<System>> m_systems;
	};

	template <typename T, typename... Args>
	decltype(auto) EntityComponentSystem::CreateComponent(Entity entity, Args&&... args)
	{
		return m_registry.emplace<T>(entity, args...);  // TODO: std::move this
	}

	template<typename T>
	T& GetComponent(entt::registry& registry, entt::entity entity)
	{
		return registry.get<T>(entity);
	}

	template<typename T>
	void EntityComponentSystem::RegisterComponent()
	{
		using namespace entt::literals;
		entt::meta_factory<T>{}
			.template func<&GetComponent<T>, entt::as_ref_t>("get"_hs)
			.template func<&T::Inspect>("inspect"_hs);
	}

	template <typename T, typename... Args>
	T& EntityComponentSystem::CreateSystem(Args&&... args)
	{
		T* system = new T(std::forward<Args>(args)...);
		m_systems.push_back(std::unique_ptr<System>(system));
		std::sort(m_systems.begin(),
			m_systems.end(),
			[](const std::unique_ptr<System>& sl, const std::unique_ptr<System>& sr) { return sl->priority > sr->priority; });
		return *system;
	}

	template <typename T>
	T& EntityComponentSystem::GetSystem()
	{
		for (auto& s : m_systems)
		{
			T* found = dynamic_cast<T*>(s.get());
			if (found) return *found;
		}
		assert(false);
		return *dynamic_cast<T*>(m_systems[0].get());  // This line will always fail
	}

	template <typename T>
	std::vector<T*> EntityComponentSystem::GetSystems()
	{
		std::vector<T*> systems;
		for (auto& s : m_systems)
		{
			if (T* found = dynamic_cast<T*>(s.get())) systems.push_back(found);
		}
		return systems;
	}
}

#define REGISTER_COMPONENT(Type)                                    \
    namespace {                                                     \
        const bool CONCAT(_component_registered_, __COUNTER__) =    \
            (clvr::EntityComponentSystem::RegisterComponent<Type>(), true); \
    }

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)