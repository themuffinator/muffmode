// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

#include <cassert>
#include <cstdio>
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
	return choices[std::forward<RandomFunc>(random_func)(choices.size())];
}

