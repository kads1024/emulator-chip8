#include <chrono>
#include <cstdlib> 
#include <iostream>

#include "app.h"
#include "chip8.h"

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    int videoScale = std::atoi(argv[1]);
    int cycleDelay = std::atoi(argv[2]);
    char const *romFilename = argv[3];

    app mainApp("CHIP-8 Emulator", 64 * videoScale, 32 * videoScale, 64, 32);

    chip8 chip8Core;
    chip8Core.load_rom(romFilename);

    int videoPitch = static_cast<int>(sizeof(chip8Core.displayBuffer[0]) * 64);

    auto lastCycleTime = std::chrono::high_resolution_clock::now();
    bool quit = false;

    while (!quit)
    {
        quit = mainApp.ProcessInput(chip8Core.keypad);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(
                       currentTime - lastCycleTime)
                       .count();

        if (dt > static_cast<float>(cycleDelay))
        {
            lastCycleTime = currentTime;

            chip8Core.cycle_cpu();

            mainApp.Update(chip8Core.displayBuffer, videoPitch);
        }
    }

    return 0;
}