// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

#include <cassert>
#include <cstdio>
#include <cstddef>
#include <utility>
#include <vector>

/*
=============
G_SelectRandomTarget

Selects a random target from a list using the provided random generator.
=============
*/
template<typename T, typename RandomFunc>
T *G_SelectRandomTarget(const std::vector<T *> &choices, RandomFunc &&random_func) {
	if (choices.empty()) {
		std::fprintf(stderr, "%s: attempted random selection with no available targets.\n", __FUNCTION__);
		return nullptr;
	}

	assert(!choices.empty());

	const std::size_t max_index = choices.size() - 1;
	const std::size_t generated_index = std::forward<RandomFunc>(random_func)(max_index);
	const std::size_t clamped_index = generated_index > max_index ? max_index : generated_index;
	return choices[clamped_index];
}

