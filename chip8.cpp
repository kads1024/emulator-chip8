#include "chip8.h"
#include <fstream>

constexpr uint16_t ROM_START_ADDRESS = 0x200;
constexpr uint8_t FONTSET_SIZE = 0x50; // 80 total bytes (16 characters * 5 bytes each)
constexpr uint8_t FONTSET_ADDRESS = 0x50;

uint8_t fontset[FONTSET_SIZE] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

chip8::chip8() : randomNumberGenerator(std::chrono::system_clock::now().time_since_epoch().count())
{
    programCounter = ROM_START_ADDRESS;
    randomByte = std::uniform_int_distribution<uint8_t>(0, 255U);

    for (uint8_t fontByte = 0; fontByte < FONTSET_SIZE; fontByte++)
    {
        memory[FONTSET_ADDRESS + fontByte] = fontset[fontByte];
    }
}

void chip8::load_rom(const char* const fileName)
{
    std::ifstream rom(fileName, std::ios::binary | std::ios::ate);

    if(rom.is_open())
    {
        // get size of file
        std::streampos fileSize = rom.tellg();

        // go back to beginning
        rom.seekg(0);

        // fill memory
        rom.read(reinterpret_cast<char*>(memory + ROM_START_ADDRESS), fileSize);

        rom.close();
    }
}