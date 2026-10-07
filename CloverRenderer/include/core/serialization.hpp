#pragma once

#include <nlohmann/json.hpp>
#include <entt/entity/fwd.hpp>
#include <DirectXMath.h>
#include "core/ecs.hpp"

using json = nlohmann::json;

namespace DirectX {
    inline void to_json(nlohmann::json& j, const XMFLOAT2& v) { j = nlohmann::json::array({ v.x, v.y }); }
    inline void from_json(const nlohmann::json& j, XMFLOAT2& v) { j.at(0).get_to(v.x); j.at(1).get_to(v.y); }
    inline void to_json(nlohmann::json& j, const XMFLOAT3& v) { j = nlohmann::json::array({ v.x, v.y, v.z }); }
    inline void from_json(const nlohmann::json& j, XMFLOAT3& v) { j.at(0).get_to(v.x); j.at(1).get_to(v.y); j.at(2).get_to(v.z); }
    inline void to_json(nlohmann::json& j, const XMFLOAT4& v) { j = nlohmann::json::array({ v.x, v.y, v.z, v.w }); }
    inline void from_json(const nlohmann::json& j, XMFLOAT4& v) { j.at(0).get_to(v.x); j.at(1).get_to(v.y); j.at(2).get_to(v.z); j.at(3).get_to(v.w); }
}

namespace clvr {

    struct ComponentSerializer {
        const char* name;
        void (*save)(const entt::registry&, entt::entity, json&, const char* key);
        void (*load)(entt::registry&, entt::entity, const json&, const char* key);
    };

    inline std::vector<ComponentSerializer>& GetSerializers() {
        static std::vector<ComponentSerializer> s;
        return s;
    }

    template <typename T>
    bool RegisterSerializer(const char* key)
    {
        auto& all = GetSerializers();
        for (auto& s : all)
            if (std::strcmp(s.name, key) == 0) return true;   // already registered from another TU

        all.push_back({ key,
            [](const entt::registry& r, entt::entity e, json& out, const char* k) {
                if (auto* c = r.try_get<T>(e)) out[k] = *c;
            },
            [](entt::registry& r, entt::entity e, const json& in, const char* k) {
                if (in.contains(k)) r.emplace_or_replace<T>(e, in.at(k).get<T>());
            } });
        return true;
    }
}

#define SAVEABLE_COMPONENT(Type, Key)                                              \
    REGISTER_COMPONENT(Type, Key)                                                  \
    namespace {                                                                    \
        const bool CONCAT(_component_serializer_, __COUNTER__) =                   \
            clvr::RegisterSerializer<Type>(Key);                                   \
    }