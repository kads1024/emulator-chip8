#include <chrono>
#include <cstdint>
#include <random>

class chip8
{
public:
    chip8();

public:
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


    void load_rom(const char* const fileName);

private:
    std::default_random_engine randomNumberGenerator;
    std::uniform_int_distribution<uint8_t> randomByte;
};