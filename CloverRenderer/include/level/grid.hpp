#pragma once
#include <algorithm>
#include <cassert>   
#include <utility>   
#include <vector>

namespace clvr {
	template <typename T>
	struct Grid {
		int width = 0;
		int height = 0;

		std::vector<T> cells;

		Grid() = default;
		Grid(int w, int h, T fill = T{})
			: width(w), height(h), cells(static_cast<size_t>(w)* h, fill) {}

		bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }

		T& At(int x, int y) { assert(InBounds(x, y)); return cells[static_cast<size_t>(y) * width + x]; }
		const T& At(int x, int y) const { assert(InBounds(x, y)); return cells[static_cast<size_t>(y) * width + x]; }

		void Resize(int newW, int newH, T fill = T{})
		{
			Grid<T> resized(newW, newH, fill);
			const int copyW = (std::min)(width, newW);
			const int copyH = (std::min)(height, newH);
			for (int y = 0; y < copyH; ++y)
				for (int x = 0; x < copyW; ++x)
					resized.At(x, y) = At(x, y);
			*this = std::move(resized);
		}
	};
}