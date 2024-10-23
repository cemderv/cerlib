# Logging

Due to the lack of a true cross-platform logging mechanism in C++, cerlib provides its own.

```cpp title="Example"
cer::log_info("Hello World! This is {} message and it supports {}!",
              1,
              "formatting");
```

cerlib's logging mechanism respects the target platform, for example:

* Logs to the Visual Studio output window in Visual Studio
* Logs to Android Logcat on Android
* Logs to stdout (terminal) on other platforms

The following logging functions are available at all times:

---

* [`cer::log_info(format, args...)`](../api/Misc/index.md#log_error)

Logs a normal message to the system output, indicating a status update.

---

* [`cer::log_warning(format, args...)`](../api/Misc/index.md#log_error)

Logs a warning to the system output, indicating that the developer
should pay attention to some operation that was just performed.

---

* [`cer::log_error(format, args...)`](../api/Misc/index.md#log_error)

Logs an error to the system output, indicating that the developer
should fix a bug in the game.

---

* [`cer::log_debug(format, args...)`](../api/Misc/index.md#log_debug)

Logs a normal message to the system output **only in debug mode**.
In a release build, this is optimized away by the compiler and results in a no-op.

---

* [`cer::log_verbose(format, args...)`](../api/Misc/index.md#log_debug)

Logs a normal message to the system output **only in debug mode** and
**only** if the `CERLIB_ENABLE_VERBOSE_LOGGING` CMake option was enabled.
In a release build, this is optimized away by the compiler and results in a no-op.



