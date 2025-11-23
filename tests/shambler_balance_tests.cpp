#include "../src/g_local.h"
#include <cassert>

int ShamblerApplyExplosionResistance(gentity_t *self, int damage, const mod_t &mod);

/*
=============
main

Verifies explosion resistance halves splash damage for the shambler before pain handling.
=============
*/
int main()
{
	gentity_t shambler{};
	shambler.health = 160;
	
	mod_t splash{ MOD_R_SPLASH };
	int adjusted = ShamblerApplyExplosionResistance(&shambler, 40, splash);
	
	assert(adjusted == 20);
	assert(shambler.health == 180);
	
	shambler.health = 150;
	mod_t direct{ MOD_SHOTGUN };
	adjusted = ShamblerApplyExplosionResistance(&shambler, 40, direct);
	
	assert(adjusted == 40);
	assert(shambler.health == 150);
	
	return 0;
}
