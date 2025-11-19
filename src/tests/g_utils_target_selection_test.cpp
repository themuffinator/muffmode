#include <cassert>
#include <cstdint>
#include <vector>

#include "../g_utils_target_selection.h"

struct DummyTarget {
};

/*
=============
main

Validates that the target selection helper can pick entries beyond the first eight options.
=============
*/
int main() {
	DummyTarget targets[10];
	std::vector<DummyTarget *> references;
	references.reserve(std::size(targets));

	for (auto &target : targets)
		references.push_back(&target);

	auto cycling_random = [index = static_cast<int32_t>(0)](size_t max) mutable {
		return (index++) % static_cast<int32_t>(max);
	};

	bool saw_ninth_entry = false;

	for (size_t i = 0; i < references.size(); i++) {
		DummyTarget *choice = G_SelectRandomTarget(references, cycling_random);
		if (choice == references[9])
			saw_ninth_entry = true;
	}

	assert(saw_ninth_entry);
	return 0;
}
