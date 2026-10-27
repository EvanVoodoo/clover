#pragma once
#include <cstdint>
#include <array>
#include <string_view>
#include <nlohmann/json.hpp>

namespace clvr
{
	namespace CollisionFilters
	{
		// Collision filter categories for Box2D which uses 64 bits for collision filtering. Each category is represented by a unique bit in a 64-bit integer.
		enum class CollisionCategory : uint64_t {
			None = 0,
			Player = 1ULL << 0,
			Enemy = 1ULL << 1,
			Wall = 1ULL << 2,
			Obstacle = 1ULL << 3,
			PlayerProjectile = 1ULL << 4,
			EnemyProjectile = 1ULL << 5,
			Default = 1ULL << 6,
		};

		constexpr CollisionCategory operator|(CollisionCategory a, CollisionCategory b) {
			return static_cast<CollisionCategory>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
		}

		inline constexpr std::string_view ToString(CollisionCategory c) {
			switch (c) {
				case CollisionCategory::Player:           return "Player";
				case CollisionCategory::Enemy:            return "Enemy";
				case CollisionCategory::Wall:             return "Wall";
				case CollisionCategory::Obstacle:         return "Obstacle";
				case CollisionCategory::PlayerProjectile: return "PlayerProjectile";
				case CollisionCategory::EnemyProjectile:  return "EnemyProjectile";
				case CollisionCategory::Default:          return "Default";
				default:                                  return "Default";
			}
		}

		inline constexpr CollisionCategory CategoryFromString(std::string_view s) {
			if (s == "Player")           return CollisionCategory::Player;
			if (s == "Enemy")            return CollisionCategory::Enemy;
			if (s == "Wall")             return CollisionCategory::Wall;
			if (s == "Obstacle")         return CollisionCategory::Obstacle;
			if (s == "PlayerProjectile") return CollisionCategory::PlayerProjectile;
			if (s == "EnemyProjectile")  return CollisionCategory::EnemyProjectile;
			return CollisionCategory::Default;   // unknown or missing name falls back safely
		}

		inline void to_json(nlohmann::json& j, const CollisionCategory& c) {
			j = ToString(c);
		}

		inline void from_json(const nlohmann::json& j, CollisionCategory& c) {
			c = CategoryFromString(j.get<std::string>());
		}

		constexpr uint64_t Bits(CollisionCategory c) { return static_cast<uint64_t>(c); }

		constexpr bool IsSingleBit(CollisionCategory c) {
			const uint64_t v = Bits(c);
			return v != 0 && (v & (v - 1)) == 0;
		}

		void InspectCollisionCategory(CollisionCategory& category);

		struct CollisionRule {
			CollisionCategory category;
			CollisionCategory collidesWith;
		};

		using CC = CollisionCategory;

		// The one hand-written source of truth. Each row is one row of the table.
		inline constexpr std::array<CollisionRule, 6> kCollisionTable = { {
			{ CC::Player,           CC::Enemy | CC::Wall | CC::Obstacle | CC::EnemyProjectile },
			{ CC::Enemy,            CC::Player | CC::Wall | CC::Obstacle | CC::PlayerProjectile },
			{ CC::Wall,             CC::Player | CC::Enemy | CC::PlayerProjectile | CC::EnemyProjectile },
			{ CC::Obstacle,         CC::Player | CC::Enemy | CC::PlayerProjectile | CC::EnemyProjectile },
			{ CC::PlayerProjectile, CC::Enemy | CC::Wall | CC::Obstacle },
			{ CC::EnemyProjectile,  CC::Player | CC::Wall | CC::Obstacle },
		} };

		constexpr uint64_t MaskFor(CollisionCategory category) {
			if (category == CollisionCategory::Default)
				return ~0ULL;

			for (const CollisionRule& rule : kCollisionTable)
				if (rule.category == category)
					return Bits(rule.collidesWith) | Bits(CollisionCategory::Default);
			return 0;
		}

		constexpr bool Collides(CollisionCategory a, CollisionCategory b) {
			return (MaskFor(a) & Bits(b)) != 0;
		}

		constexpr bool TableIsSymmetric() {
			for (const CollisionRule& a : kCollisionTable)
				for (const CollisionRule& b : kCollisionTable)
					if (Collides(a.category, b.category) != Collides(b.category, a.category))
						return false;
			return true;
		}

		static_assert(TableIsSymmetric(), "Collision table must be symmetric: if A hits B, B must hit A.");
	}
}