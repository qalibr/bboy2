# bboy2

After I wrote my first emulator in C# I immediately wanted to try to write one in C++. This emulator uses function pointers with a lookup table for the CPU instructions and memory paging for the MMU. It supports joypad input, saving and loading states, and loading ROMs via drag-and-drop or via CLI. Holding `space` uncaps FPS.

<img src="./img/acid2.png" alt="drawing" style="width:400px;"/>

## Building and Running

### Clone the repository

```sh
git clone https://github.com/qalibr/bboy2.git
cd bboy2
```

### Build the project

Xmake requires the user to choose which mode, *debug or release*, they are in (default is release). After selecting a mode, whenever you build and run the program it will be executed in whatever mode you chose.

To change modes, use either of these in the CLI.

```sh
xmake f -m debug
```

```sh
xmake f -m release
```

Then, build:

```sh
xmake build
```

\**Look for '.debug/.release' in the terminal output to know which mode you are in.*

### Run the emulator

```sh
xmake run
```

You can also supply an argument to load a rom instantly:

```sh
xmake run bboy2 <path-to-your-rom>
```

Example:

```sh
xmake run bboy2 roms/dmg-acid2.gb
```

\**Xmake runs in the project directory and expects to find the custom font there (assets/font/...).*

## Tracy Profiler (Windows)

Recently added [Tracy](https://github.com/wolfpld/tracy), a C++ frame profiler, to the project. If you want to try it, you can follow the setup below.

### Setup

1. Clone the Tracy repository into your project directory.

    ```sh
    git clone https://github.com/wolfpld/tracy.git
    ```

2. Download the [Tracy Profiler](https://github.com/wolfpld/tracy/releases) GUI for Windows from the releases page.

3. **Match versions**: The Tracy client you cloned must match the Tracy Profiler you downloaded.
    - Extract the contents of the zip file into an unrelated folder and run the executable `tracy-profiler.exe`. Check what version it is (e.g., v0.13.1).
    - In your terminal, check out the matching version tag in the `tracy` directory.

    ```sh
    cd tracy
    git fetch --tags
    git checkout v0.13.1
    cd ..
    ```

### Building with profiling enabled

1. To enable profiling, configure xmake with the following command:

    ```sh
    xmake f -m release --profile_trace=y
    ```

2. Build:

    ```sh
    xmake build
    ```

It's best to use release build for profiling.

### Disabling profiling

1. When you want to run without profiling simply use this command:

    ```sh
    xmake f -m debug --profile_trace=n
    ```

2. Build:

    ```sh
    xmake build
    ```

### How to profile

1. Launch `tracy-profiler.exe`.
2. Click the `Connect` button, the application is now listening for the client we cloned to begin running.
3. Now simply run the program:

    ```sh
    xmake run
    ```

### Adding code to the profile

You can easily add more functions to the profiler timeline using macros:

- `ZoneScoped`: Add to the top of a scope or function you want to measure.
- `FrameMark`: Marks the end of a frame. It is already placed in the main loop.

See `src/main.cpp` and `src/emulator/emulator.cpp` for examples.

## Keymap

| Action            | Key           |
| ----------------- | ------------- |
| D-Pad Up          | `W`           |
| D-Pad Down        | `S`           |
| D-Pad Left        | `A`           |
| D-Pad Right       | `D`           |
| A Button          | `E`           |
| B Button          | `R`           |
| Start             | `F`           |
| Select            | `Z`           |
| Toggle FPS        | `I`           |
| Save State        | `Ctrl + T`    |
| Load State        | `Ctrl + L`    |
| Uncap FPS (Hold)  | `Space`       |

## Resoures

<https://gbdev.io/pandocs/>

<https://gbdev.io/gb-opcodes/optables/>

<https://github.com/gbdev/awesome-gbdev>

### Testing

<https://github.com/robert/gameboy-doctor>

<https://github.com/retrio/gb-test-roms>

<https://github.com/mattcurrie/dmg-acid2>

## Emulators

<https://github.com/qalibr/BomberBoy/tree/main>

## Libraries

<https://github.com/raysan5/raylib/releases>

## Tools

<https://github.com/wolfpld/tracy>
