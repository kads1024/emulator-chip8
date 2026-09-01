#include "chip8.h"
#include <fstream>
#include <iostream>

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
    randomByte = std::uniform_int_distribution<unsigned int>(0, 255u);

    for (uint8_t fontByte = 0; fontByte < FONTSET_SIZE; fontByte++)
    {
        memory[FONTSET_ADDRESS + fontByte] = fontset[fontByte];
    }

    // Fill msater table
    masterTable[0x0000u >> 12] = &chip8::Table0;
    masterTable[0x1000u >> 12] = &chip8::OP_1nnn;
    masterTable[0x2000u >> 12] = &chip8::OP_2nnn;
    masterTable[0x3000u >> 12] = &chip8::OP_3xkk;
    masterTable[0x4000u >> 12] = &chip8::OP_4xkk;
    masterTable[0x5000u >> 12] = &chip8::OP_5xy0;
    masterTable[0x6000u >> 12] = &chip8::OP_6xkk;
    masterTable[0x7000u >> 12] = &chip8::OP_7xkk;
    masterTable[0x8000u >> 12] = &chip8::Table8;
    masterTable[0x9000u >> 12] = &chip8::OP_9xy0;
    masterTable[0xA000u >> 12] = &chip8::OP_Annn;
    masterTable[0xB000u >> 12] = &chip8::OP_Bnnn;
    masterTable[0xC000u >> 12] = &chip8::OP_Cxkk;
    masterTable[0xD000u >> 12] = &chip8::OP_Dxyn;
    masterTable[0xE000u >> 12] = &chip8::TableE;
    masterTable[0xF000u >> 12] = &chip8::TableF;

    // prefill subtable with OP_NULL
    for (uint8_t table08EsubTableIndex = 0; table08EsubTableIndex < 0x000E + 1; table08EsubTableIndex++)
    {
        table0[table08EsubTableIndex] = &chip8::OP_NULL;
        table8[table08EsubTableIndex] = &chip8::OP_NULL;
        tableE[table08EsubTableIndex] = &chip8::OP_NULL;
    }
    for (uint8_t tableFSubTableIndex = 0; tableFSubTableIndex < 0x0065 + 1; tableFSubTableIndex++)
    {
        tableF[tableFSubTableIndex] = &chip8::OP_NULL;
    }

    // fill correct subtable slots
    table0[0x0000u] = &chip8::OP_00E0;
    table0[0x000Eu] = &chip8::OP_00EE;

    table8[0x0000u] = &chip8::OP_8xy0;
    table8[0x0001u] = &chip8::OP_8xy1;
    table8[0x0002u] = &chip8::OP_8xy2;
    table8[0x0003u] = &chip8::OP_8xy3;
    table8[0x0004u] = &chip8::OP_8xy4;
    table8[0x0005u] = &chip8::OP_8xy5;
    table8[0x0006u] = &chip8::OP_8xy6;
    table8[0x0007u] = &chip8::OP_8xy7;
    table8[0x000Eu] = &chip8::OP_8xyE;

    tableE[0x000Eu] = &chip8::OP_Ex9E;
    tableE[0x0001u] = &chip8::OP_ExA1;

    tableF[0x0007u] = &chip8::OP_Fx07;
    tableF[0x000Au] = &chip8::OP_Fx0A;
    tableF[0x0015u] = &chip8::OP_Fx15;
    tableF[0x0018u] = &chip8::OP_Fx18;
    tableF[0x001Eu] = &chip8::OP_Fx1E;
    tableF[0x0029u] = &chip8::OP_Fx29;
    tableF[0x0033u] = &chip8::OP_Fx33;
    tableF[0x0055u] = &chip8::OP_Fx55;
    tableF[0x0065u] = &chip8::OP_Fx65;
}

void chip8::load_rom(const char* const fileName)
{
    std::ifstream rom(fileName, std::ios::binary | std::ios::ate);

    if(rom.is_open())
    {
        // get size of file
        const std::streamsize fileSize = static_cast<std::streamsize>(rom.tellg());

        // go back to beginning
        rom.seekg(0);

        // fill memory
        rom.read(reinterpret_cast<char*>(memory + ROM_START_ADDRESS), fileSize);

        rom.close();
    }
}

void chip8::cycle_cpu()
{
    // Fetch
    opcode = memory[programCounter] << 8u | memory[programCounter + 1];

    // Increment PC
    programCounter += 2;

    // Decode + execute
    (this->*masterTable[opcode >> 12u])();

    // Decrement DT
    if(delayTimer > 0)
        delayTimer--;

    // Decrement ST
    if(soundTimer > 0)
        soundTimer--;
}


void chip8::OP_NULL()
{
}

void chip8::Table0()
{
    (this->*table0[opcode & 0x000Fu])();
}

void chip8::Table8()
{
    (this->*table8[opcode & 0x000Fu])();
}

void chip8::TableE()
{
    (this->*tableE[opcode & 0x000Fu])();
}

void chip8::TableF()
{
    (this->*tableF[opcode & 0x00FFu])();
}

void chip8::OP_00E0()
{
    std::cout << std::hex << opcode << ": OP_00E0\n";
}

void chip8::OP_00EE()
{
    std::cout << std::hex << opcode << ": OP_00EE\n";
}

void chip8::OP_1nnn()
{
    std::cout << std::hex << opcode << ": OP_1nnn\n";
}

void chip8::OP_2nnn()
{
    std::cout << std::hex << opcode << ": OP_2nnn\n";
}

void chip8::OP_3xkk()
{
    std::cout << std::hex << opcode << ": OP_3xkk\n";
}

void chip8::OP_4xkk()
{
    std::cout << std::hex << opcode << ": OP_4xkk\n";
}

void chip8::OP_5xy0()
{
    std::cout << std::hex << opcode << ": OP_5xy0\n";
}

void chip8::OP_6xkk()
{
    std::cout << std::hex << opcode << ": OP_6xkk\n";
}

void chip8::OP_7xkk()
{
    std::cout << std::hex << opcode << ": OP_7xkk\n";
}

void chip8::OP_8xy0()
{
    std::cout << std::hex << opcode << ": OP_8xy0\n";
}

void chip8::OP_8xy1()
{
    std::cout << std::hex << opcode << ": OP_8xy1\n";
}

void chip8::OP_8xy2()
{
    std::cout << std::hex << opcode << ": OP_8xy2\n";
}

void chip8::OP_8xy3()
{
    std::cout << std::hex << opcode << ": OP_8xy3\n";
}

void chip8::OP_8xy4()
{
    std::cout << std::hex << opcode << ": OP_8xy4\n";
}

void chip8::OP_8xy5()
{
    std::cout << std::hex << opcode << ": OP_8xy5\n";
}

void chip8::OP_8xy6()
{
    std::cout << std::hex << opcode << ": OP_8xy6\n";
}

void chip8::OP_8xy7()
{
    std::cout << std::hex << opcode << ": OP_8xy7\n";
}

void chip8::OP_8xyE()
{
    std::cout << std::hex << opcode << ": OP_8xyE\n";
}

void chip8::OP_9xy0()
{
    std::cout << std::hex << opcode << ": OP_9xy0\n";
}

void chip8::OP_Annn()
{
    std::cout << std::hex << opcode << ": OP_Annn\n";
}

void chip8::OP_Bnnn()
{
    std::cout << std::hex << opcode << ": OP_Bnnn\n";
}

void chip8::OP_Cxkk()
{
    std::cout << std::hex << opcode << ": OP_Cxkk\n";
}

void chip8::OP_Dxyn()
{
    std::cout << std::hex << opcode << ": OP_Dxyn\n";
}

void chip8::OP_Ex9E()
{
    std::cout << std::hex << opcode << ": OP_Ex9E\n";
}

void chip8::OP_ExA1()
{
    std::cout << std::hex << opcode << ": OP_ExA1\n";
}

void chip8::OP_Fx07()
{
    std::cout << std::hex << opcode << ": OP_Fx07\n";
}

void chip8::OP_Fx0A()
{
    std::cout << std::hex << opcode << ": OP_Fx0A\n";
}

void chip8::OP_Fx15()
{
    std::cout << std::hex << opcode << ": OP_Fx15\n";
}

void chip8::OP_Fx18()
{
    std::cout << std::hex << opcode << ": OP_Fx18\n";
}

void chip8::OP_Fx1E()
{
    std::cout << std::hex << opcode << ": OP_Fx1E\n";
}

void chip8::OP_Fx29()
{
    std::cout << std::hex << opcode << ": OP_Fx29\n";
}

void chip8::OP_Fx33()
{
    std::cout << std::hex << opcode << ": OP_Fx33\n";
}

void chip8::OP_Fx55()
{
    std::cout << std::hex << opcode << ": OP_Fx55\n";
}

void chip8::OP_Fx65()
{
    std::cout << std::hex << opcode << ": OP_Fx65\n";
}
