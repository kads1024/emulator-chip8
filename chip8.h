#include <cstdint>

struct chip8
{
    uint8_t registers[16];
    uint8_t memory[4096];
    uint16_t index;
    uint16_t programCounter;
    uint16_t stack[16];
    uint8_t stackPointer;
    uint8_t delatTimer;
    uint8_t soundTimer;
    uint8_t keypad[16];
    uint32_t displayBuffer[64 * 32];

    uint16_t opcode; // $AD22 - first byte = op, second byte = data
};