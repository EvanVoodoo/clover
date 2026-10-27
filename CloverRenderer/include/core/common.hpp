#pragma once

namespace clvr {
	enum class EngineMode { Editing, Playing };

	class Lifetime {
	public:
		Lifetime() = default;
		Lifetime(float lifetime) : lifetime(lifetime), elapsedTime(0.0f), isAlive(true) {}
		~Lifetime() = default;

		void UpdateLife(float dt) {
			if (isAlive) {
				elapsedTime += dt;
				if (elapsedTime >= lifetime) {
					isAlive = false;
				}
			}
		}
	private:
		float lifetime = 0.0f;
		float elapsedTime = 0.0f;
		bool isAlive = true;
	};
}