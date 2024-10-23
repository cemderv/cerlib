# Playing Sounds

Audio playback in cerlib is designed to be simple and easy to use while
providing features that most 2D games would be satisfied with.

cerlib doesn’t provide any DAW experience or full-featured mixing capabilities.
Its audio engine is an optional part; you don’t have to use any of cerlib’s audio capabilities.

This is so that you’re able to choose an audio engine of your choice, such as [FMOD](https://www.fmod.com/) or [Wwise](https://www.audiokinetic.com/en/wwise/).
Those are full-fledged audio engines and frameworks that allow you to pretty much control
any aspect of your game’s audio.

The first thing to do is to add a sound file to your assets folder.
You could take the `GemCollected.wav` file from cerlib’s folder.
It’s located in `<cerlib>/applications/platformer/assets/sounds/GemCollected.wav`.

Then add a [`cer::Sound`](../api/Audio/Sound/index.md) variable to your game
and load it in the `load_content` method:

```cpp
class MyGame : public cer::Game
{
public:
  // ...
  
  void load_content() override
  {
    sound = cer::load_sound("GemCollected.wav");
  }
  
  // ...
  
private:
  cer::Sound sound;
};
```

Add a simple key press check in the `update` method and play the sound if ++space++ is pressed:

```cpp
bool update(const cer::GameTime& time) override
{
  if (cer::was_key_just_pressed(cer::Key::Space))
  {
    cer::play_sound_fire_and_forget(sound);
  }
 
  return true;
}
```

You should now be able to hear the sound play whenever you press ++space++.
The [`cer::play_sound_fire_and_forget`](../api/Audio/index.md#play_sound_fire_and_forget)
offers more parameters beside the sound, such as volume, panning and the delay after which
to start playing it.

Example:

```cpp
if (cer::was_key_just_pressed(cer::Key::Space))
{
  cer::play_sound_fire_and_forget(sound,
                                 /*volume: */ 1.0f,
                                 /*pan: */    0.0f,
                                 /*delay: */  0.8 // Start playing once 800ms have passed
                                 );
}
```

Just playing a sound is often enough, especially for actions that happen once instead of
continuously, for example explosions.

You can however obtain the virtual channel (of type [`cer::SoundChannel`](../api/Audio/SoundChannel/index.md))
that the sound is playing in after we start to play it.
This is done via the [`cer::play_sound`](../api/Audio/index.md#play_sound) function.

Modify the sound-playing code as follows:

```cpp
if (cer::was_key_just_pressed(cer::Key::Space))
{
  auto channel = cer::play_sound(sound);
  channel.set_looping(true);
}
```

Now the sound should be looping once you press ++space++.
[`cer::SoundChannel`](../api/Audio/SoundChannel/index.md) provides many more methods such as modifying
volume and panning, setting loop points and pause/resume controls.

A playing sound can always be stopped by using stop. If the sound should be stopped or paused
after a certain time, use [`stop_after`](../api/Audio/SoundChannel/index.md#stop_after) and use
[`pause_after`](../api/Audio/SoundChannel/index.md#pause_after) respectively.

Here’s an example:

```cpp
channel.stop_after(1.5);
channel.pause_after(2.6);
// ...
```

It’s possible that your game may play too many sounds at once, in which case the audio engine
will prioritize certain sounds over others (older sounds) and “cull” them.
To protect a sound channel so that it won’t get culled, use the [`set_protected`](../api/Audio/SoundChannel/index.md#set_protected) method.

Another case would be when there are so many sounds playing at once that some sounds are inaudible.
The audio engine observes this and provides you with an option of what should happen with inaudible sounds.
Use the [`set_inaudible_behavior`](../api/Audio/SoundChannel/index.md#set_inaudible_behavior)
method to specify this for a sound channel.

For music and ambient sounds, the [`cer::play_sound_in_background`](../api/Audio/index.md#play_sound_in_background) function is ideal.
It plays a continuous sound with its volume set equally to all channels, and without panning.
It also returns a channel so that you’re able to control its playback throughout the game.

---

API Reference:

* [Audio](../api/Audio/index.md)
* [cer::Sound](../api/Audio/Sound/index.md)
* [cer::SoundChannel](../api/Audio/SoundChannel/index.md)
