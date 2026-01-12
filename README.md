This is a working NES emulator. It is capable of playing basic cartriges ([Mapper 0](https://www.nesdev.org/wiki/NROM)). The [NesDev Wiki](https://www.nesdev.org/wiki/NES_reference_guide) was super useful for implementing this project.

Code Breakdown:

- **Run loop:** [main.c](https://github.com/sjaoudi/nesemu/blob/main/main.c) loads a ROM, sets up SDL, and runs the main “emulate one frame” loop.
- **Clock + coordination:** [system.c](https://github.com/sjaoudi/nesemu/blob/main/system.c) keeps CPU and PPU in sync (PPU runs 3 cycles per CPU cycle) and handles interrupts.
- **Bus + cartridge:** [memory.c](https://github.com/sjaoudi/nesemu/blob/main/memory.c) routes reads/writes to the right place; [cartridge.c](https://github.com/sjaoudi/nesemu/blob/main/cartridge.c) loads the ROM data and mapper info.
- **I/O:** [video.c](https://github.com/sjaoudi/nesemu/blob/main/video.c) displays frames; [controller.c](https://github.com/sjaoudi/nesemu/blob/main/controller.c) maps keyboard input to a NES controller.
- **Build:** [Makefile](https://github.com/sjaoudi/nesemu/blob/main/Makefile) compiles with gcc and links SDL2.

[CPU code](https://github.com/sjaoudi/nesemu/blob/main/cpu.c) + [cpu.h](https://github.com/sjaoudi/nesemu/blob/main/cpu.h) emulate the NES’s main processor. It:
- Reads the next instruction from memory
- Executes it (math, branching, loads/stores, stack operations)
- Keeps track of CPU state (registers + flags) and timing

This is the part that “runs the game logic.”

[PPU code](https://github.com/sjaoudi/nesemu/blob/main/ppu.c) + [ppu.h](https://github.com/sjaoudi/nesemu/blob/main/ppu.h) emulate the NES graphics chip (the 2C02 PPU). It:
- Draws the screen one line at a time (scanlines)
- Builds the background from tiles
- Draws sprites on top (characters, enemies, etc.)
- Uses palettes to turn tile/sprite data into actual colors

It also signals “vertical blank” (VBlank) each frame, which is how games know it’s safe to update graphics.
