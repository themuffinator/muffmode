# Regression Tests

## Mover collision resolution
- **Purpose:** Prevent movers from disappearing when collision traces reference freed or invalid entities during ground detection.
- **Setup:**
  - Create a simple map with a platform mover intersecting with another entity that can be removed mid-move.
  - Instrument the mover so that a collision resolution pass occurs immediately after the other entity is freed.
- **Steps:**
  1. Trigger the mover so it begins translating through the space occupied by the soon-to-be-removed entity.
  2. Remove the obstructing entity during the mover's travel to force a collision trace against an invalid reference.
  3. Allow the mover to complete its motion.
- **Expected result:** The mover finishes its path without being unlinked or lost from the world after the collision trace references an invalid entity.
