#pragma once

#include "core/gizmo.hpp"

namespace clvr {
	class SelectionGizmo : public Gizmo {
	public:
		SelectionGizmo();
		SelectionGizmo(const GizmoAxisParams& xAxisParams, const GizmoAxisParams& yAxisParams, bool oneAxis = true);
		virtual void Update(const Camera& camera) override;
		virtual void Draw(const Camera& camera) override;
	};
}