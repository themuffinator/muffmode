# Custom Gravity Push Coverage

A manual check to ensure custom gravity values survive push interactions:

1. Spawn a movable entity (e.g., a crate) and set its `gravity` to a non-default value such as `0.5`.
2. Position the entity on top of a moving platform so it will be pushed during the simulation.
3. Run a push movement tick and confirm the entity's `gravity` remains `0.5` after the push completes.
4. Verify triggers along the movement path still fire for the pushed entity.
