# Map Transition Crash Fixes and Prevention

This document outlines the fixes applied to prevent crashes during map transitions in deathmatch/FFA servers.

## Issues Fixed

### 1. Map Queue System Memory Corruption
**Problem**: The map queue system was not properly removing empty elements, leading to potential memory corruption and crashes.

**Fix**: 
- Properly remove empty elements from the queue vector instead of just clearing them
- Added bounds checking for map queue removal operations
- Fixed `MQ_Remove_Index()` to actually remove elements from the vector

### 2. Entity Reference Cleanup
**Problem**: Dangling entity references during map transitions could cause crashes when the new map loads.

**Fix**:
- Added comprehensive entity reference cleanup in `BeginIntermission()`
- Clear follow targets, viewed entities, and bot-specific references
- Ensure all entity pointers are nullified before map transition

### 3. Bot Team Preservation Safety
**Problem**: Bot team preservation code could access null pointers during edge cases.

**Fix**:
- Added null pointer checks for bot client data
- Enhanced safety checks in team preservation logic

### 4. Map Name Validation
**Problem**: Invalid or empty map names could cause crashes during transitions.

**Fix**:
- Added validation for map names before attempting transitions
- Error handling for null or empty changemap values

## Recommended Server Configuration

### Core Settings
```
set maxclients 32
set maxentities 2048
set g_map_list_shuffle 1
set g_dm_same_level 0
```

### Bot Settings
```
set bot_name_prefix "BOT|"
set g_dm_allow_no_humans 1
```

### Memory and Performance
```
set g_lag_compensation 1
set g_weapon_respawn_time 30
set g_dm_do_warmup 1
set g_dm_do_readyup 0
```

### Map Transition Settings
```
set g_allow_mymap 1
set g_dm_allow_exit 1
```

## Additional Recommendations

### 1. Map List Management
- Keep your map list (`g_map_list`) properly formatted with space-separated map names
- Test all maps in the rotation to ensure they load properly
- Avoid having duplicate map names in the list

### 2. Bot Management
- Don't manually disconnect bots during map transitions
- Let the automatic bot management handle team assignments
- Monitor bot count to prevent excessive bot spawning

### 3. Server Monitoring
- Watch for console errors during map transitions
- Monitor memory usage, especially with large player counts
- Test map transitions under load with multiple players

### 4. Debugging Tips
If crashes still occur:
1. Check the console for "Got null changemap" or "Got empty changemap" errors
2. Verify all maps in `g_map_list` exist and are valid
3. Monitor entity count during transitions
4. Check for custom entities that might not be cleaning up properly

## Technical Details

### Memory Management
The fixes ensure proper cleanup of:
- Vector containers (map queue)
- Entity references and pointers
- Client session data
- Bot-specific data structures

### Entity Lifecycle
During map transitions, the following cleanup occurs:
1. Player trails destroyed
2. Entity references nullified
3. Bot goals and targets cleared
4. Follow targets reset
5. Memory tags freed (TAG_LEVEL)

### Safety Checks
Added multiple validation layers:
- Null pointer checks before accessing client data
- Bounds checking for array/vector access
- Map name validation before transitions
- Entity validity checks before reference access

## Version History
- v1.0: Initial crash fixes for map transitions
- Applied to muffmode fork based on Quake 2 Rerelease 