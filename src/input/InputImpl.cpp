// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "InputImpl.hpp"

#include "cerlib/KeyModifier.hpp"
#include "cerlib/MouseButton.hpp"
#include "soloud.h"

#include <cassert>

#ifdef __EMSCRIPTEN__
#define SDL_SCANCODE_MEDIA_NEXT_TRACK     SDL_SCANCODE_AUDIONEXT
#define SDL_SCANCODE_MEDIA_PREVIOUS_TRACK SDL_SCANCODE_AUDIOPREV
#define SDL_SCANCODE_MEDIA_STOP           SDL_SCANCODE_AUDIOSTOP
#define SDL_SCANCODE_MEDIA_PLAY           SDL_SCANCODE_AUDIOPLAY
#define SDL_SCANCODE_MEDIA_EJECT          SDL_SCANCODE_EJECT
#define SDL_SCANCODE_MEDIA_REWIND         SDL_SCANCODE_AUDIOREWIND
#define SDL_SCANCODE_MEDIA_FAST_FORWARD   SDL_SCANCODE_AUDIOFASTFORWARD
#endif

namespace cer::details
{
int InputImpl::to_sdl_key(Key key)
{
    switch (key)
    {
        case Key::Return: return SDL_SCANCODE_RETURN;
        case Key::Escape: return SDL_SCANCODE_ESCAPE;
        case Key::Backspace: return SDL_SCANCODE_BACKSPACE;
        case Key::Tab: return SDL_SCANCODE_TAB;
        case Key::Space: return SDL_SCANCODE_SPACE;
        case Key::Comma: return SDL_SCANCODE_COMMA;
        case Key::Minus: return SDL_SCANCODE_MINUS;
        case Key::Period: return SDL_SCANCODE_PERIOD;
        case Key::Slash: return SDL_SCANCODE_SLASH;
        case Key::D0: return SDL_SCANCODE_0;
        case Key::D1: return SDL_SCANCODE_1;
        case Key::D2: return SDL_SCANCODE_2;
        case Key::D3: return SDL_SCANCODE_3;
        case Key::D4: return SDL_SCANCODE_4;
        case Key::D5: return SDL_SCANCODE_5;
        case Key::D6: return SDL_SCANCODE_6;
        case Key::D7: return SDL_SCANCODE_7;
        case Key::D8: return SDL_SCANCODE_8;
        case Key::D9: return SDL_SCANCODE_9;
        case Key::Semicolon: return SDL_SCANCODE_SEMICOLON;
        case Key::Equals: return SDL_SCANCODE_EQUALS;
        case Key::LeftBracket: return SDL_SCANCODE_LEFTBRACKET;
        case Key::Backslash: return SDL_SCANCODE_BACKSLASH;
        case Key::RightBracket: return SDL_SCANCODE_RIGHTBRACKET;
        case Key::A: return SDL_SCANCODE_A;
        case Key::B: return SDL_SCANCODE_B;
        case Key::C: return SDL_SCANCODE_C;
        case Key::D: return SDL_SCANCODE_D;
        case Key::E: return SDL_SCANCODE_E;
        case Key::F: return SDL_SCANCODE_F;
        case Key::G: return SDL_SCANCODE_G;
        case Key::H: return SDL_SCANCODE_H;
        case Key::I: return SDL_SCANCODE_I;
        case Key::J: return SDL_SCANCODE_J;
        case Key::K: return SDL_SCANCODE_K;
        case Key::L: return SDL_SCANCODE_L;
        case Key::M: return SDL_SCANCODE_M;
        case Key::N: return SDL_SCANCODE_N;
        case Key::O: return SDL_SCANCODE_O;
        case Key::P: return SDL_SCANCODE_P;
        case Key::Q: return SDL_SCANCODE_Q;
        case Key::R: return SDL_SCANCODE_R;
        case Key::S: return SDL_SCANCODE_S;
        case Key::T: return SDL_SCANCODE_T;
        case Key::U: return SDL_SCANCODE_U;
        case Key::V: return SDL_SCANCODE_V;
        case Key::W: return SDL_SCANCODE_W;
        case Key::X: return SDL_SCANCODE_X;
        case Key::Y: return SDL_SCANCODE_Y;
        case Key::Z: return SDL_SCANCODE_Z;
        case Key::CapsLock: return SDL_SCANCODE_CAPSLOCK;
        case Key::F1: return SDL_SCANCODE_F1;
        case Key::F2: return SDL_SCANCODE_F2;
        case Key::F3: return SDL_SCANCODE_F3;
        case Key::F4: return SDL_SCANCODE_F4;
        case Key::F5: return SDL_SCANCODE_F5;
        case Key::F6: return SDL_SCANCODE_F6;
        case Key::F7: return SDL_SCANCODE_F7;
        case Key::F8: return SDL_SCANCODE_F8;
        case Key::F9: return SDL_SCANCODE_F9;
        case Key::F10: return SDL_SCANCODE_F10;
        case Key::F11: return SDL_SCANCODE_F11;
        case Key::F12: return SDL_SCANCODE_F12;
        case Key::PrintScreen: return SDL_SCANCODE_PRINTSCREEN;
        case Key::ScrollLock: return SDL_SCANCODE_SCROLLLOCK;
        case Key::Pause: return SDL_SCANCODE_PAUSE;
        case Key::Insert: return SDL_SCANCODE_INSERT;
        case Key::Home: return SDL_SCANCODE_HOME;
        case Key::PageUp: return SDL_SCANCODE_PAGEUP;
        case Key::Delete: return SDL_SCANCODE_DELETE;
        case Key::End: return SDL_SCANCODE_END;
        case Key::PageDown: return SDL_SCANCODE_PAGEDOWN;
        case Key::Right: return SDL_SCANCODE_RIGHT;
        case Key::Left: return SDL_SCANCODE_LEFT;
        case Key::Down: return SDL_SCANCODE_DOWN;
        case Key::Up: return SDL_SCANCODE_UP;
        case Key::NumLockClear: return SDL_SCANCODE_NUMLOCKCLEAR;
        case Key::KeypadDivide: return SDL_SCANCODE_KP_DIVIDE;
        case Key::KeypadMultiply: return SDL_SCANCODE_KP_MULTIPLY;
        case Key::KeypadMinus: return SDL_SCANCODE_KP_MINUS;
        case Key::KeypadPlus: return SDL_SCANCODE_KP_PLUS;
        case Key::KeypadEnter: return SDL_SCANCODE_KP_ENTER;
        case Key::Keypad1: return SDL_SCANCODE_KP_1;
        case Key::Keypad2: return SDL_SCANCODE_KP_2;
        case Key::Keypad3: return SDL_SCANCODE_KP_3;
        case Key::Keypad4: return SDL_SCANCODE_KP_4;
        case Key::Keypad5: return SDL_SCANCODE_KP_5;
        case Key::Keypad6: return SDL_SCANCODE_KP_6;
        case Key::Keypad7: return SDL_SCANCODE_KP_7;
        case Key::Keypad8: return SDL_SCANCODE_KP_8;
        case Key::Keypad9: return SDL_SCANCODE_KP_9;
        case Key::Keypad0: return SDL_SCANCODE_KP_0;
        case Key::KeypadPeriod: return SDL_SCANCODE_KP_PERIOD;
        case Key::Application: return SDL_SCANCODE_APPLICATION;
        case Key::Power: return SDL_SCANCODE_POWER;
        case Key::KeypadEquals: return SDL_SCANCODE_KP_EQUALS;
        case Key::F13: return SDL_SCANCODE_F13;
        case Key::F14: return SDL_SCANCODE_F14;
        case Key::F15: return SDL_SCANCODE_F15;
        case Key::F16: return SDL_SCANCODE_F16;
        case Key::F17: return SDL_SCANCODE_F17;
        case Key::F18: return SDL_SCANCODE_F18;
        case Key::F19: return SDL_SCANCODE_F19;
        case Key::F20: return SDL_SCANCODE_F20;
        case Key::F21: return SDL_SCANCODE_F21;
        case Key::F22: return SDL_SCANCODE_F22;
        case Key::F23: return SDL_SCANCODE_F23;
        case Key::F24: return SDL_SCANCODE_F24;
        case Key::Execute: return SDL_SCANCODE_EXECUTE;
        case Key::Help: return SDL_SCANCODE_HELP;
        case Key::Menu: return SDL_SCANCODE_MENU;
        case Key::Stop: return SDL_SCANCODE_STOP;
        case Key::Again: return SDL_SCANCODE_AGAIN;
        case Key::Undo: return SDL_SCANCODE_UNDO;
        case Key::Cut: return SDL_SCANCODE_CUT;
        case Key::Copy: return SDL_SCANCODE_COPY;
        case Key::Paste: return SDL_SCANCODE_PASTE;
        case Key::Find: return SDL_SCANCODE_FIND;
        case Key::Mute: return SDL_SCANCODE_MUTE;
        case Key::VolumeUp: return SDL_SCANCODE_VOLUMEUP;
        case Key::VolumeDown: return SDL_SCANCODE_VOLUMEDOWN;
        case Key::Cancel: return SDL_SCANCODE_CANCEL;
        case Key::Clear: return SDL_SCANCODE_CLEAR;
        case Key::Return2: return SDL_SCANCODE_RETURN2;
        case Key::Separator: return SDL_SCANCODE_SEPARATOR;
        case Key::LeftControl: return SDL_SCANCODE_LCTRL;
        case Key::LeftShift: return SDL_SCANCODE_LSHIFT;
        case Key::LeftAlt: return SDL_SCANCODE_LALT;
        case Key::RightControl: return SDL_SCANCODE_RCTRL;
        case Key::RightShift: return SDL_SCANCODE_RSHIFT;
        case Key::RightAlt: return SDL_SCANCODE_RALT;
        case Key::Mode: return SDL_SCANCODE_MODE;
        case Key::AudioNext: return SDL_SCANCODE_MEDIA_NEXT_TRACK;
        case Key::AudioPrevious: return SDL_SCANCODE_MEDIA_PREVIOUS_TRACK;
        case Key::AudioStop: return SDL_SCANCODE_MEDIA_STOP;
        case Key::AudioPlay: return SDL_SCANCODE_MEDIA_PLAY;
        case Key::Eject: return SDL_SCANCODE_MEDIA_EJECT;
        case Key::Sleep: return SDL_SCANCODE_SLEEP;
        case Key::AudioRewind: return SDL_SCANCODE_MEDIA_REWIND;
        case Key::AudioFastForward: return SDL_SCANCODE_MEDIA_FAST_FORWARD;
        case Key::SoftLeft: return SDL_SCANCODE_SOFTLEFT;
        case Key::SoftRight: return SDL_SCANCODE_SOFTRIGHT;
        case Key::Call: return SDL_SCANCODE_CALL;
        case Key::EndCall: return SDL_SCANCODE_ENDCALL;
    }

    return SDL_SCANCODE_UNKNOWN;
}

Key InputImpl::from_sdl_key(SDL_Keycode sdl_key)
{
    switch (sdl_key)
    {
        case SDL_SCANCODE_RETURN: return Key::Return;
        case SDL_SCANCODE_ESCAPE: return Key::Escape;
        case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
        case SDL_SCANCODE_TAB: return Key::Tab;
        case SDL_SCANCODE_SPACE: return Key::Space;
        case SDL_SCANCODE_COMMA: return Key::Comma;
        case SDL_SCANCODE_MINUS: return Key::Minus;
        case SDL_SCANCODE_PERIOD: return Key::Period;
        case SDL_SCANCODE_SLASH: return Key::Slash;
        case SDL_SCANCODE_0: return Key::D0;
        case SDL_SCANCODE_1: return Key::D1;
        case SDL_SCANCODE_2: return Key::D2;
        case SDL_SCANCODE_3: return Key::D3;
        case SDL_SCANCODE_4: return Key::D4;
        case SDL_SCANCODE_5: return Key::D5;
        case SDL_SCANCODE_6: return Key::D6;
        case SDL_SCANCODE_7: return Key::D7;
        case SDL_SCANCODE_8: return Key::D8;
        case SDL_SCANCODE_9: return Key::D9;
        case SDL_SCANCODE_SEMICOLON: return Key::Semicolon;
        case SDL_SCANCODE_EQUALS: return Key::Equals;
        case SDL_SCANCODE_LEFTBRACKET: return Key::LeftBracket;
        case SDL_SCANCODE_BACKSLASH: return Key::Backslash;
        case SDL_SCANCODE_RIGHTBRACKET: return Key::RightBracket;
        case SDL_SCANCODE_A: return Key::A;
        case SDL_SCANCODE_B: return Key::B;
        case SDL_SCANCODE_C: return Key::C;
        case SDL_SCANCODE_D: return Key::D;
        case SDL_SCANCODE_E: return Key::E;
        case SDL_SCANCODE_F: return Key::F;
        case SDL_SCANCODE_G: return Key::G;
        case SDL_SCANCODE_H: return Key::H;
        case SDL_SCANCODE_I: return Key::I;
        case SDL_SCANCODE_J: return Key::J;
        case SDL_SCANCODE_K: return Key::K;
        case SDL_SCANCODE_L: return Key::L;
        case SDL_SCANCODE_M: return Key::M;
        case SDL_SCANCODE_N: return Key::N;
        case SDL_SCANCODE_O: return Key::O;
        case SDL_SCANCODE_P: return Key::P;
        case SDL_SCANCODE_Q: return Key::Q;
        case SDL_SCANCODE_R: return Key::R;
        case SDL_SCANCODE_S: return Key::S;
        case SDL_SCANCODE_T: return Key::T;
        case SDL_SCANCODE_U: return Key::U;
        case SDL_SCANCODE_V: return Key::V;
        case SDL_SCANCODE_W: return Key::W;
        case SDL_SCANCODE_X: return Key::X;
        case SDL_SCANCODE_Y: return Key::Y;
        case SDL_SCANCODE_Z: return Key::Z;
        case SDL_SCANCODE_CAPSLOCK: return Key::CapsLock;
        case SDL_SCANCODE_F1: return Key::F1;
        case SDL_SCANCODE_F2: return Key::F2;
        case SDL_SCANCODE_F3: return Key::F3;
        case SDL_SCANCODE_F4: return Key::F4;
        case SDL_SCANCODE_F5: return Key::F5;
        case SDL_SCANCODE_F6: return Key::F6;
        case SDL_SCANCODE_F7: return Key::F7;
        case SDL_SCANCODE_F8: return Key::F8;
        case SDL_SCANCODE_F9: return Key::F9;
        case SDL_SCANCODE_F10: return Key::F10;
        case SDL_SCANCODE_F11: return Key::F11;
        case SDL_SCANCODE_F12: return Key::F12;
        case SDL_SCANCODE_PRINTSCREEN: return Key::PrintScreen;
        case SDL_SCANCODE_SCROLLLOCK: return Key::ScrollLock;
        case SDL_SCANCODE_PAUSE: return Key::Pause;
        case SDL_SCANCODE_INSERT: return Key::Insert;
        case SDL_SCANCODE_HOME: return Key::Home;
        case SDL_SCANCODE_PAGEUP: return Key::PageUp;
        case SDL_SCANCODE_DELETE: return Key::Delete;
        case SDL_SCANCODE_END: return Key::End;
        case SDL_SCANCODE_PAGEDOWN: return Key::PageDown;
        case SDL_SCANCODE_RIGHT: return Key::Right;
        case SDL_SCANCODE_LEFT: return Key::Left;
        case SDL_SCANCODE_DOWN: return Key::Down;
        case SDL_SCANCODE_UP: return Key::Up;
        case SDL_SCANCODE_NUMLOCKCLEAR: return Key::NumLockClear;
        case SDL_SCANCODE_KP_DIVIDE: return Key::KeypadDivide;
        case SDL_SCANCODE_KP_MULTIPLY: return Key::KeypadMultiply;
        case SDL_SCANCODE_KP_MINUS: return Key::KeypadMinus;
        case SDL_SCANCODE_KP_PLUS: return Key::KeypadPlus;
        case SDL_SCANCODE_KP_ENTER: return Key::KeypadEnter;
        case SDL_SCANCODE_KP_1: return Key::Keypad1;
        case SDL_SCANCODE_KP_2: return Key::Keypad2;
        case SDL_SCANCODE_KP_3: return Key::Keypad3;
        case SDL_SCANCODE_KP_4: return Key::Keypad4;
        case SDL_SCANCODE_KP_5: return Key::Keypad5;
        case SDL_SCANCODE_KP_6: return Key::Keypad6;
        case SDL_SCANCODE_KP_7: return Key::Keypad7;
        case SDL_SCANCODE_KP_8: return Key::Keypad8;
        case SDL_SCANCODE_KP_9: return Key::Keypad9;
        case SDL_SCANCODE_KP_0: return Key::Keypad0;
        case SDL_SCANCODE_KP_PERIOD: return Key::KeypadPeriod;
        case SDL_SCANCODE_APPLICATION: return Key::Application;
        case SDL_SCANCODE_POWER: return Key::Power;
        case SDL_SCANCODE_KP_EQUALS: return Key::KeypadEquals;
        case SDL_SCANCODE_F13: return Key::F13;
        case SDL_SCANCODE_F14: return Key::F14;
        case SDL_SCANCODE_F15: return Key::F15;
        case SDL_SCANCODE_F16: return Key::F16;
        case SDL_SCANCODE_F17: return Key::F17;
        case SDL_SCANCODE_F18: return Key::F18;
        case SDL_SCANCODE_F19: return Key::F19;
        case SDL_SCANCODE_F20: return Key::F20;
        case SDL_SCANCODE_F21: return Key::F21;
        case SDL_SCANCODE_F22: return Key::F22;
        case SDL_SCANCODE_F23: return Key::F23;
        case SDL_SCANCODE_F24: return Key::F24;
        case SDL_SCANCODE_EXECUTE: return Key::Execute;
        case SDL_SCANCODE_HELP: return Key::Help;
        case SDL_SCANCODE_MENU: return Key::Menu;
        case SDL_SCANCODE_STOP: return Key::Stop;
        case SDL_SCANCODE_AGAIN: return Key::Again;
        case SDL_SCANCODE_UNDO: return Key::Undo;
        case SDL_SCANCODE_CUT: return Key::Cut;
        case SDL_SCANCODE_COPY: return Key::Copy;
        case SDL_SCANCODE_PASTE: return Key::Paste;
        case SDL_SCANCODE_FIND: return Key::Find;
        case SDL_SCANCODE_MUTE: return Key::Mute;
        case SDL_SCANCODE_VOLUMEUP: return Key::VolumeUp;
        case SDL_SCANCODE_VOLUMEDOWN: return Key::VolumeDown;
        case SDL_SCANCODE_CANCEL: return Key::Cancel;
        case SDL_SCANCODE_CLEAR: return Key::Clear;
        case SDL_SCANCODE_RETURN2: return Key::Return2;
        case SDL_SCANCODE_SEPARATOR: return Key::Separator;
        case SDL_SCANCODE_LCTRL: return Key::LeftControl;
        case SDL_SCANCODE_LSHIFT: return Key::LeftShift;
        case SDL_SCANCODE_LALT: return Key::LeftAlt;
        case SDL_SCANCODE_RCTRL: return Key::RightControl;
        case SDL_SCANCODE_RSHIFT: return Key::RightShift;
        case SDL_SCANCODE_RALT: return Key::RightAlt;
        case SDL_SCANCODE_MODE: return Key::Mode;
        case SDL_SCANCODE_MEDIA_NEXT_TRACK: return Key::AudioNext;
        case SDL_SCANCODE_MEDIA_PREVIOUS_TRACK: return Key::AudioPrevious;
        case SDL_SCANCODE_MEDIA_STOP: return Key::AudioStop;
        case SDL_SCANCODE_MEDIA_PLAY: return Key::AudioPlay;
        case SDL_SCANCODE_MEDIA_EJECT: return Key::Eject;
        case SDL_SCANCODE_SLEEP: return Key::Sleep;
        case SDL_SCANCODE_MEDIA_REWIND: return Key::AudioRewind;
        case SDL_SCANCODE_MEDIA_FAST_FORWARD: return Key::AudioFastForward;
        case SDL_SCANCODE_SOFTLEFT: return Key::SoftLeft;
        case SDL_SCANCODE_SOFTRIGHT: return Key::SoftRight;
        case SDL_SCANCODE_CALL: return Key::Call;
        case SDL_SCANCODE_ENDCALL: return Key::EndCall;
        default: return static_cast<Key>(0);
    }
}

int InputImpl::to_sdl_mouse_button(MouseButton button)
{
    switch (button)
    {
        case MouseButton::Left: return SDL_BUTTON_LEFT;
        case MouseButton::Right: return SDL_BUTTON_RIGHT;
        case MouseButton::Middle: return SDL_BUTTON_MIDDLE;
    }

    return 0;
}

MouseButton InputImpl::from_sdl_mouse_button(int sdl_button)
{
    switch (sdl_button)
    {
        case SDL_BUTTON_LEFT: return MouseButton::Left;
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        default: return static_cast<MouseButton>(0);
    }
}

#ifdef __EMSCRIPTEN__
#define CER_KMOD_LSHIFT KMOD_LSHIFT
#define CER_KMOD_RSHIFT KMOD_RSHIFT
#define CER_KMOD_LCTRL  KMOD_LCTRL
#define CER_KMOD_RCTRL  KMOD_RCTRL
#define CER_KMOD_LALT   KMOD_LALT
#define CER_KMOD_RALT   KMOD_RALT
#define CER_KMOD_NUM    KMOD_NUM
#define CER_KMOD_CAPS   KMOD_CAPS
#else
#define CER_KMOD_LSHIFT SDL_KMOD_LSHIFT
#define CER_KMOD_RSHIFT SDL_KMOD_RSHIFT
#define CER_KMOD_LCTRL  SDL_KMOD_LCTRL
#define CER_KMOD_RCTRL  SDL_KMOD_RCTRL
#define CER_KMOD_LALT   SDL_KMOD_LALT
#define CER_KMOD_RALT   SDL_KMOD_RALT
#define CER_KMOD_NUM    SDL_KMOD_NUM
#define CER_KMOD_CAPS   SDL_KMOD_CAPS
#endif

static KeyModifier operator|(KeyModifier lhs, KeyModifier rhs)
{
    return static_cast<KeyModifier>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

static KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

static KeyModifier operator&(KeyModifier lhs, KeyModifier rhs)
{
    return static_cast<KeyModifier>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

static KeyModifier from_sdl_keymods(Uint16 mods)
{
    auto result = KeyModifier::None;

    if (mods & CER_KMOD_LSHIFT)
    {
        result |= KeyModifier::LeftShift;
    }

    if (mods & CER_KMOD_RSHIFT)
    {
        result |= KeyModifier::RightShift;
    }

    if (mods & CER_KMOD_LCTRL)
    {
        result |= KeyModifier::LeftControl;
    }

    if (mods & CER_KMOD_RCTRL)
    {
        result |= KeyModifier::RightControl;
    }

    if (mods & CER_KMOD_LALT)
    {
        result |= KeyModifier::LeftAlt;
    }

    if (mods & CER_KMOD_RALT)
    {
        result |= KeyModifier::RightAlt;
    }

    if (mods & CER_KMOD_NUM)
    {
        result |= KeyModifier::Num;
    }

    if (mods & CER_KMOD_CAPS)
    {
        result |= KeyModifier::Caps;
    }

    return result;
}

#ifdef __EMSCRIPTEN__
auto InputImpl::from_sdl_keysym(const SDL_Keysym& sdl_keysym) -> std::pair<Key, KeyModifier>
{
    const auto sdl_key = sdl_keysym.sym;
    const auto sdl_mod = sdl_keysym.mod;
#else
std::pair<Key, KeyModifier> InputImpl::from_sdl_keysym(SDL_Keycode sdl_key, SDL_Keymod sdl_mod)
{
#endif
    return {
        from_sdl_key(sdl_key),
        from_sdl_keymods(sdl_mod),
    };
}

InputImpl::InputImpl()
    : m_previous_key_states()
    , m_key_states()
{
}

InputImpl& InputImpl::instance()
{
    static auto s_instance = InputImpl();
    return s_instance;
}

bool InputImpl::is_key_down(Key key) const
{
    const auto idx = static_cast<size_t>(key) - 1;
    return idx >= m_key_states.size() ? false : m_key_states[idx] == 1;
}

bool InputImpl::was_key_just_pressed(Key key) const
{
    const auto idx = static_cast<size_t>(key) - 1;
    return idx >= m_key_states.size() ? false
                                      : (m_previous_key_states[idx] == 0 && m_key_states[idx] == 1);
}

bool InputImpl::was_key_just_released(Key key) const
{
    const auto idx = static_cast<size_t>(key) - 1;
    return idx >= m_key_states.size() ? false
                                      : (m_previous_key_states[idx] == 1 && m_key_states[idx] == 0);
}

bool InputImpl::is_mouse_button_down(MouseButton button) const
{
    const auto bits = SDL_GetMouseState(nullptr, nullptr);
    return bits & SDL_BUTTON(to_sdl_mouse_button(button));
}

void InputImpl::update_key_states()
{
    int        num_keys{};
    const auto sdl_key_states = SDL_GetKeyboardState(&num_keys);

    m_previous_key_states = m_key_states;

    for (size_t i = 0; i < m_key_states.size(); ++i)
    {
        const auto key     = static_cast<Key>(i + 1);
        const auto sdl_key = to_sdl_key(key);
        assert(sdl_key < num_keys);
        m_key_states[i] = sdl_key_states[sdl_key] == 1;
    }
}

Vector2 InputImpl::mouse_position_delta() const
{
    return m_mouse_position_delta;
}

void InputImpl::set_mouse_position_delta(Vector2 value)
{
    m_mouse_position_delta = value;
}

Vector2 InputImpl::mouse_wheel_delta() const
{
    return m_mouse_wheel_delta;
}

void InputImpl::set_mouse_wheel_delta(Vector2 value)
{
    m_mouse_wheel_delta = value;
}
} // namespace cer::details
