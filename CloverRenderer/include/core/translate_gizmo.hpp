#pragma once

#include "gizmo.hpp"

namespace clvr {
	class TranslateGizmo : public Gizmo {
	public:
		TranslateGizmo();
		TranslateGizmo(const GizmoAxisParams& xAxisParams, const GizmoAxisParams& yAxisParams, bool oneAxis = false);
		virtual void Update(const Camera& camera) override;
		virtual void Draw(const Camera& camera) override;
	};
}