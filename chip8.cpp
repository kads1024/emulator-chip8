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

void chip8::load_rom(const char *const fileName)
{
    std::ifstream rom(fileName, std::ios::binary | std::ios::ate);

    if (rom.is_open())
    {
        // get size of file
        const std::streamsize fileSize = static_cast<std::streamsize>(rom.tellg());

        // go back to beginning
        rom.seekg(0);

        // fill memory
        rom.read(reinterpret_cast<char *>(memory + ROM_START_ADDRESS), fileSize);

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
    if (delayTimer > 0)
        delayTimer--;

    // Decrement ST
    if (soundTimer > 0)
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
    // set the entire video buffer to zeroes.
    memset(displayBuffer, 0, sizeof(displayBuffer));
}

void chip8::OP_00EE()
{
    // The top of the stack has the address of one instruction past the one that called the subroutine, so we can put that back into the PC. Note that this overwrites our preemptive pc += 2 earlier.
    stackPointer--;
    if (stackPointer < 0)
    {
        std::cerr << "OP_00EE: STACK POINTER CAN'T BE NEGATIVE\n";
        stackPointer = 0;
        return;
    }
    programCounter = stack[stackPointer];
}

void chip8::OP_1nnn()
{
    // The interpreter sets the program counter to nnn.
    // A jump doesn’t remember its origin, so no stack interaction required.
    programCounter = opcode & 0x0FFFu;
}

void chip8::OP_2nnn()
{
    // When we call a subroutine, we want to return eventually, so we put the current PC onto the top of the stack. Remember that we did pc += 2 in Cycle(), so the current PC holds the next instruction after this CALL, which is correct. We don’t want to return to the CALL instruction because it would be an infinite loop of CALLs and RETs.
    stack[stackPointer++] = programCounter;
    programCounter = opcode & 0x0FFFu;
}

void chip8::OP_3xkk()
{
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment by 2 again to skip the next instruction.
    if(registers[(opcode >> 8u) & 0x000Fu] == (opcode & 0x00FFu))
    {
        programCounter += 2;
    }
 }

void chip8::OP_4xkk()
{
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment by 2 again to skip the next instruction.
    if (registers[(opcode >> 8u) & 0x000Fu] != (opcode & 0x00FFu))
    {
        programCounter += 2;
    }
}

void chip8::OP_5xy0()
{
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment by 2 again to skip the next instruction.
    if (registers[(opcode >> 8u) & 0x000Fu] == 
    (registers[(opcode >> 4u) & 0x000Fu]))
    {
        programCounter += 2;
    }
}

void chip8::OP_6xkk()
{
    registers[(opcode >> 8u) & 0x000Fu] = (opcode & 0x00FFu);
}

void chip8::OP_7xkk()
{
    registers[(opcode >> 8u) & 0x000Fu] += (opcode & 0x00FFu);
}

void chip8::OP_8xy0()
{
    registers[(opcode >> 8u) & 0x000Fu] = registers[(opcode >> 4u) & 0x000Fu];
}

void chip8::OP_8xy1()
{
    registers[(opcode >> 8u) & 0x000Fu] |= registers[(opcode >> 4u) & 0x000Fu];
}

void chip8::OP_8xy2()
{
    registers[(opcode >> 8u) & 0x000Fu] &= registers[(opcode >> 4u) & 0x000Fu];
}

void chip8::OP_8xy3()
{
    registers[(opcode >> 8u) & 0x000Fu] ^= registers[(opcode >> 4u) & 0x000Fu];
}

void chip8::OP_8xy4()
{
    // The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx.
    // This is an ADD with an overflow flag.If the sum is greater than what can fit into a byte(255), register VF will be set to 1 as a flag.
    uint16_t Vx = (opcode >> 8u) & 0x000Fu;
    uint16_t Vy = (opcode >> 4u) & 0x000Fu;
    uint16_t regVx = registers[Vx];
    uint16_t regVy = registers[Vy];

    if(regVx + regVy > 0x00FFu)
        registers[0x000Fu] = 1;
    else
        registers[0x000Fu] = 0;

    registers[Vx] += registers[Vy];
}

void chip8::OP_8xy5()
{
    // If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
    uint16_t Vx = (opcode >> 8u) & 0x000Fu;
    uint16_t Vy = (opcode >> 4u) & 0x000Fu;

    if (registers[Vx] > registers[Vy])
        registers[0x000Fu] = 1;
    else
        registers[0x000Fu] = 0;

    registers[Vx] -= registers[Vy];
}

void chip8::OP_8xy6()
{
    // If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
    // A right shift is performed(division by 2), and the least significant bit is saved in Register VF.
    uint16_t Vx = (opcode >> 8u) & 0x000Fu;


    if (registers[Vx] & 0x0001u)
        registers[0x000Fu] = 1;
    else
        registers[0x000Fu] = 0;

    registers[Vx] >>= 1;
}

void chip8::OP_8xy7()
{
    // If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
    uint16_t Vx = (opcode >> 8u) & 0x000Fu;
    uint16_t Vy = (opcode >> 4u) & 0x000Fu;

    if (registers[Vy] > registers[Vx])
        registers[0x000Fu] = 1;
    else
        registers[0x000Fu] = 0;

    registers[Vx] = registers[Vy] - registers[Vx];
}

void chip8::OP_8xyE()
{
    // If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
    // A left shift is performed(multiplication by 2), and the most significant bit is saved in Register VF.
    uint16_t Vx = (opcode >> 8u) & 0x000Fu;

    if (registers[Vx] & 0x0080u)
        registers[0x000Fu] = 1;
    else
        registers[0x000Fu] = 0;

    registers[Vx] <<= 1;
}

void chip8::OP_9xy0()
{
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment by 2 again to skip the next instruction.
    if (registers[(opcode >> 8u) & 0x000Fu] !=
        registers[(opcode >> 4u) & 0x000Fu])
    {
        programCounter += 2;
    }
}

void chip8::OP_Annn()
{
    index = opcode & 0x0FFFu;
}

void chip8::OP_Bnnn()
{
    programCounter = memory[opcode & 0x0FFFu] + registers[0];
}

void chip8::OP_Cxkk()
{
    registers[(opcode >> 8u) & 0x000Fu] = randomByte(randomNumberGenerator) & (opcode & 0x00FFu);
}

void chip8::OP_Dxyn()
{
    // DRW Vx, Vy, nibble (draw nibble-byte sprite at mem-location I at (Vx, Vy), (VF = collision)
    // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
    // We iterate over the sprite, row by row and column by column. We know there are eight columns because a sprite is guaranteed to be eight pixels wide.
    // If a sprite pixel is on then there may be a collision with what’s already being displayed, so we check if our screen pixel in the same location is set. If so we must set the VF register to express collision.
    // Then we can just XOR the screen pixel with 0xFFFFFFFF to essentially XOR it with the sprite pixel (which we now know is on). We can’t XOR directly because the sprite pixel is either 1 or 0 while our video pixel is either 0x00000000 or 0xFFFFFFFF.

    uint8_t Vx = (opcode >> 8u) & 0x000Fu;
    uint8_t Vy = (opcode >> 4u) & 0x000Fu;
    uint8_t height = opcode & 0x000Fu;

    // Wrap
    uint8_t xPos = registers[Vx] % 64;
    uint8_t yPos = registers[Vy] % 32;

    registers[0x000F] = 0;

    for (unsigned int row = 0; row < height; row++)
    {
        uint8_t spriteByte = memory[index + row];

        for (unsigned int col = 0; col < 8; ++col)
        {
            uint8_t spritePixel = spriteByte & (0x80u >> col);
            uint32_t *screenPixel = &displayBuffer[(yPos + row) * 64 + (xPos + col)];

            // Sprite pixel on
            if (spritePixel)
            {
                // Screen pixel also on - collision
                if (*screenPixel == 0xFFFFFFFF)
                {
                    registers[0xF] = 1;
                }

                // XOR with screen
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

void chip8::OP_Ex9E()
{
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment by 2 again to skip the next instruction.
    uint8_t Vx = (opcode >> 8u) & 0x000Fu;

    if (keypad[registers[Vx]])
    {
        programCounter += 2;
    }
}

void chip8::OP_ExA1()
{
    // Since our PC has already been incremented by 2 in Cycle(), we can just increment by 2 again to skip the next instruction.
    uint8_t Vx = (opcode >> 8u) & 0x000Fu;

    if (!keypad[registers[Vx]])
    {
        programCounter += 2;
    }
}

void chip8::OP_Fx07()
{
    registers[(opcode >> 8u) & 0x000Fu] = delayTimer;
}

void chip8::OP_Fx0A()
{
    // The easiest way to “wait” is to decrement the PC by 2 whenever a keypad value is not detected. This has the effect of running the same instruction repeatedly.
    uint8_t Vx = (opcode >> 8u) & 0x000Fu;

    if (keypad[0])
    {
        registers[Vx] = 0;
    }
    else if (keypad[1])
    {
        registers[Vx] = 1;
    }
    else if (keypad[2])
    {
        registers[Vx] = 2;
    }
    else if (keypad[3])
    {
        registers[Vx] = 3;
    }
    else if (keypad[4])
    {
        registers[Vx] = 4;
    }
    else if (keypad[5])
    {
        registers[Vx] = 5;
    }
    else if (keypad[6])
    {
        registers[Vx] = 6;
    }
    else if (keypad[7])
    {
        registers[Vx] = 7;
    }
    else if (keypad[8])
    {
        registers[Vx] = 8;
    }
    else if (keypad[9])
    {
        registers[Vx] = 9;
    }
    else if (keypad[10])
    {
        registers[Vx] = 10;
    }
    else if (keypad[11])
    {
        registers[Vx] = 11;
    }
    else if (keypad[12])
    {
        registers[Vx] = 12;
    }
    else if (keypad[13])
    {
        registers[Vx] = 13;
    }
    else if (keypad[14])
    {
        registers[Vx] = 14;
    }
    else if (keypad[15])
    {
        registers[Vx] = 15;
    }
    else
    {
        programCounter -= 2;
    }
}

void chip8::OP_Fx15()
{
    delayTimer = registers[(opcode >> 8u) & 0x000Fu];
}

void chip8::OP_Fx18()
{
    soundTimer = registers[(opcode >> 8u) & 0x000Fu];
}

void chip8::OP_Fx1E()
{
    index += registers[(opcode >> 8u) & 0x000Fu];
}

void chip8::OP_Fx29()
{
    // We know the font characters are located at 0x50, and we know they’re five bytes each, so we can get the address of the first byte of any character by taking an offset from the start address.
    index = FONTSET_ADDRESS + (5*registers[(opcode >> 8u) & 0x000Fu]);
}

void chip8::OP_Fx33()
{
    // Store BCD representation of Vx in memory locations I, I+1, and I+2.
    // The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
    // We can use the modulus operator to get the right-most digit of a number, and then do a division to remove that digit. A division by ten will either completely remove the digit (340 / 10 = 34), or result in a float which will be truncated (345 / 10 = 34.5 = 34).

    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];

    // Ones-place
    memory[index + 2] = value % 10;
    value /= 10;

    // Tens-place
    memory[index + 1] = value % 10;
    value /= 10;

    // Hundreds-place
    memory[index] = value % 10;
}

void chip8::OP_Fx55()
{
    uint8_t Vx = (opcode >> 8u)& 0x000Fu;

    for (uint8_t i = 0; i <= Vx; ++i)
    {
        memory[index + i] = registers[i];
    }
}

void chip8::OP_Fx65()
{
    uint8_t Vx = (opcode >> 8u) & 0x000Fu;

    for (uint8_t i = 0; i <= Vx; ++i)
    {
        registers[i] =  memory[index + i];
    }
}
