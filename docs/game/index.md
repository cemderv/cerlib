---
title: Game
description:  Represents the central game class.
generator: doxide
---


# Game

**class Game**


Represents the central game class.

It is responsible for initializing, running and deinitializing the game instance.
Only one game instance may be alive in a process at a time.

The `Game` class cannot be used directly; it must be inherited from.
To run your custom Game-derived class, use the `cer::run_game()` function.





## Functions

| Name | Description |
| ---- | ----------- |
| [Game](#Game) | Default constructor, intended to be called by deriving classes  |
| [Game](#Game) |  The game's constructor, intended to be called by deriving classes. |
| [display_count](#display_count) |  Gets the number of displays connected to the game's system. |
| [current_display_mode](#current_display_mode) |  Gets the display mode of a specific display. |
| [display_modes](#display_modes) |  Gets a list of all supported display modes of a specific display. |

## Function Details

### Game<a name="Game"></a>
!!! function "Game()"

    Default constructor, intended to be called by deriving classes 
    

!!! function "explicit Game(bool enable_audio)"

    
    The game's constructor, intended to be called by deriving classes.
    
    
    :material-location-enter: **Parameter** `enable_audio`
    :    If true, enables the audio device. If the game does not need
        any audio capabilities, false may be specified to avoid overhead.
            
    

### current_display_mode<a name="current_display_mode"></a>
!!! function "auto current_display_mode(uint32_t display_index) -&gt; std::optional&lt;DisplayMode&gt;"

    
    Gets the display mode of a specific display.
    If no display mode was determined, an empty value is returned.
    
    
    :material-location-enter: **Parameter** `display_index`
    :    The index of the display for which to obtain the current
        display mode.
            
    

### display_count<a name="display_count"></a>
!!! function "auto display_count() -&gt; uint32_t"

    
    Gets the number of displays connected to the game's system.
        
    

### display_modes<a name="display_modes"></a>
!!! function "auto display_modes(uint32_t display_index) -&gt; List&lt;DisplayMode&gt;"

    
    Gets a list of all supported display modes of a specific display.
    If no display modes were determined, an empty list is returned.
    
    
    :material-location-enter: **Parameter** `display_index`
    :    The index of the display for which to obtain the display
        modes.
            
    

