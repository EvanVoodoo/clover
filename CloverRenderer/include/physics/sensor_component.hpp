#pragma once

#include "core/ecs.hpp"
#include <algorithm>
#include <vector>
#include <memory>
#include <core/serialization.hpp>

namespace clvr
{
	struct SensorContext {
		Entity visitor;
	};

	class SensorCommand {
	public:
		virtual ~SensorCommand() = default;
		virtual void OnBegin(entt::registry& reg, const SensorContext& context) = 0;
		virtual void OnStay(entt::registry& reg, const SensorContext& context) = 0;
		virtual void OnEnd(entt::registry& reg, const SensorContext& context) = 0;

		virtual void Inspect() {}

		virtual const char* GetTypeName() const = 0;
		virtual void ToJson(nlohmann::json& j) const {}
		virtual void FromJson(const nlohmann::json& j) {}
	};

	class ColorSensorCommand : public SensorCommand {
	public:
		ColorSensorCommand() {}
		void OnBegin(entt::registry& reg, const SensorContext& context) override;
		void OnStay(entt::registry& reg, const SensorContext& context) override;
		void OnEnd(entt::registry& reg, const SensorContext& context) override;

		const char* GetTypeName() const override { return "ColorSensorCommand"; }

		void Inspect() override;

		void ToJson(nlohmann::json& j) const override {
			j["modifiedColor"] = { modifiedColor[0], modifiedColor[1], modifiedColor[2], modifiedColor[3] };
		}
		void FromJson(const nlohmann::json& j) override {
			if (j.contains("modifiedColor") && j["modifiedColor"].is_array() && j["modifiedColor"].size() == 4) {
				for (int i = 0; i < 4; ++i) {
					modifiedColor[i] = j["modifiedColor"][i].get<float>();
				}
			}
		}
	private:
		float modifiedColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // Red color
	};

	class SensorCommandFactory
	{
	public:
		using Creator = std::function<std::unique_ptr<SensorCommand>()>;

		static SensorCommandFactory& Get()
		{
			static SensorCommandFactory instance;
			return instance;
		}

		template<typename T>
		void Register(const std::string& name)
		{
			m_creators[name] = [] { return std::make_unique<T>(); };
		}

		std::unique_ptr<SensorCommand> Create(const std::string& name) const
		{
			auto it = m_creators.find(name);
			return it != m_creators.end() ? it->second() : nullptr;
		}

		std::vector<std::string> GetNames() const
		{
			std::vector<std::string> names;
			names.reserve(m_creators.size());
			for (const auto& [name, creator] : m_creators)
				names.push_back(name);
			return names;
		}

	private:
		std::map<std::string, Creator> m_creators;
	};

	struct SensorComponent
	{
		SensorComponent() = default;
		SensorComponent(const SensorComponent&) = delete;
		SensorComponent& operator=(const SensorComponent&) = delete;
		SensorComponent(SensorComponent&&) noexcept = default;
		SensorComponent& operator=(SensorComponent&&) noexcept = default;

		std::vector<std::unique_ptr<SensorCommand>> commands;
		std::vector<Entity> overlapping;

		bool active = true;

		void Inspect();

		void OnBegin(entt::registry& reg, Entity visitor);
		void OnStay(entt::registry& reg, Entity visitor);
		void OnEnd(entt::registry& reg, Entity visitor);

		template<typename T, typename... Args>
		T& AddCommand(Args&&... args)
		{
			auto& ref = commands.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
			return static_cast<T&>(*ref);
		}
		void RemoveCommand(const SensorCommand* command)
		{
			commands.erase(
				std::remove_if(commands.begin(), commands.end(),
							   [command](const std::unique_ptr<SensorCommand>& c) { return c.get() == command; }),
				commands.end());
		}
		void ClearCommands() {
			commands.clear();
		}
		void AddOverlapping(Entity entity) {
			if (std::find(overlapping.begin(), overlapping.end(), entity) == overlapping.end()) {
				overlapping.push_back(entity);
			}
		}
		void RemoveOverlapping(Entity entity) {
			overlapping.erase(std::remove(overlapping.begin(), overlapping.end(), entity), overlapping.end());
		}
		void ClearOverlapping() {
			overlapping.clear();
		}
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SensorComponent, active)

	inline void to_json(nlohmann::json& j, const SensorComponent& s) {
		j["active"] = s.active;
		j["commands"] = nlohmann::json::array();
		for (const auto& c : s.commands)
		{
			nlohmann::json cj;
			cj["type"] = c->GetTypeName();
			c->ToJson(cj);
			j["commands"].push_back(std::move(cj));
		}
	}

	inline void from_json(const nlohmann::json& j, SensorComponent& s) {
		s.active = j.value("active", true);
		s.commands.clear();
		s.overlapping.clear(); // runtime state, never loaded

		for (const auto& cj : j.value("commands", nlohmann::json::array()))
		{
			auto cmd = SensorCommandFactory::Get().Create(cj.at("type").get<std::string>());
			if (!cmd)
				continue; // unknown type: skip (or log a warning)
			cmd->FromJson(cj);
			s.commands.push_back(std::move(cmd));
		}
	}
}

SAVEABLE_COMPONENT(clvr::SensorComponent, "SensorComponent")