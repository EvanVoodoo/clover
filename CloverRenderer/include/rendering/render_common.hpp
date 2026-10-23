#pragma once

#include "DirectXMath.h"

namespace clvr {
	struct DebugLine {
		DirectX::XMFLOAT2 a;
		DirectX::XMFLOAT2 b;
		DirectX::XMFLOAT4 color;
	};
}