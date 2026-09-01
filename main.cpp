#include <chrono>
#include <iostream>

#include "chip8.h"

int main()
{
    chip8 chip8;
    chip8.load_rom("./test.ch8");

    for(int i = 0; i < 100; i++)
    {
        chip8.cycle_cpu();
    }

    std::cin.get();
    return 0;
}