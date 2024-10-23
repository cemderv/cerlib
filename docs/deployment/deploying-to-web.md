# Deploying to Web

Deploying your game for web browsers is done using Emscripten.

Please follow the [Emscripten instructions](https://emscripten.org/docs/getting_started/downloads.html) and
install it on your system.

Then export an environment variable `EMSDK` that points to the Emscripten SDK location.

=== ":fontawesome-brands-windows: Windows"

    You may add the variable using either PowerShell **or** the GUI.

    1. Using PowerShell:
        ```powershell
        [Environment]::SetEnvironmentVariable("EMSDK", "path\to\emsdk", [System.EnvironmentVariableTarget]::User)
        ```

    2. Using GUI:
        1. Open the Windows start menu
        2. Search for "Environment Variables"
        3. Add the environment variable as displayed in the application

=== ":simple-apple: macOS"

    Export the variable in your shell environment, i.e. `~/.zshrc`, `~/.zshenv` or `~/.bashrc`:

    ```bash
    # ...
    export EMSDK=/Users/cem/emsdk
    # ...
    ```

=== ":material-linux: Linux"

    Export the variable in your shell environment, i.e. `~/.zshrc`, `~/.zshenv` or `~/.bashrc`:

    ```bash
    # ...
    export EMSDK=/home/cem/emsdk
    # ...
    ```

After you are done, on macOS and Linux you might need to restart the terminal or IDE / editor.

Then you can just build your game using the `wasm` CMake preset.