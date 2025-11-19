#include "g_runthink.h"

#include <cassert>
#include <string>
#include <vector>

struct TestEntity {
	gtime_t nextthink{};
	void (*think)(TestEntity *) = nullptr;
	const char *classname = "test";
	bool freed = false;
};

/*
=============
DummyThink

Marks that a think function ran for testing
=============
*/
static void DummyThink(TestEntity *ent) {
	ent->classname = "ran";
}

/*
=============
main

Entry point for G_RunThinkImpl tests
=============
*/
int main() {
	TestEntity ent{};
	ent.nextthink = 1_ms;

	bool warning_called = false;
	std::vector<std::string> messages;

	auto logger = [&](TestEntity *warn_ent) {
		warning_called = true;
		messages.emplace_back(warn_ent->classname ? warn_ent->classname : "<unknown>");
	};

	auto fallback = [&](TestEntity *warn_ent) {
		warn_ent->freed = true;
	};

	bool result = G_RunThinkImpl(&ent, 1_ms, logger, fallback);

	assert(!result);
	assert(warning_called);
	assert(ent.freed);
	assert(ent.nextthink == 0_ms);
	assert(messages.size() == 1);
	assert(messages.front() == "test");

	ent.think = DummyThink;
	ent.freed = false;
	ent.nextthink = 1_ms;
	warning_called = false;
	messages.clear();

	result = G_RunThinkImpl(&ent, 1_ms, logger, fallback);

	assert(!result);
	assert(!warning_called);
	assert(!ent.freed);
	assert(ent.classname == std::string("ran"));

	return 0;
}
