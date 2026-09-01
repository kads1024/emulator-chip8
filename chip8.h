#pragma once
#include <chrono>
#include <cstdint>
#include <random>

class chip8
{
public:
    chip8();

public:
    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t programCounter{};
    uint16_t stack[16]{};
    uint8_t stackPointer{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keypad[16]{};
    uint32_t displayBuffer[64 * 32]{};

    uint16_t opcode{}; // $AD22 - first byte = op, second byte = data

    
public:
    void load_rom(const char *const fileName);
    void cycle_cpu();

private:
    std::default_random_engine randomNumberGenerator;
    std::uniform_int_distribution<unsigned int> randomByte;

    using chip8func = void (chip8::*)();
    chip8func masterTable[0xF + 1];
    chip8func table0[0xE + 1];
    chip8func table8[0xE + 1];
    chip8func tableE[0xE + 1];
    chip8func tableF[0x65 + 1];

private:
    // Vx, Vy = register
    // n = nibble
    // nnn = memory addr
    // kk = byte
    // I = Index
    // {,} = optional/ignored
    // DT = Delay Timer
    // ST = Sound Timer
    // F = font sprite addr
    // B = Binary-Coded Decimal (BCD)
    // [] = memory addr itself

    // JP = JUMP
    // SE = Skip (next instruction) Equal
    // SNE = Skip Not Equal
    // LD = Load
    // SHR = Shift right
    // SUBN = reverse SUB
    // SHL = Shift Left
    // SKP = Skip key pressed
    // SKNP = Skip Key not pressed
    
    void OP_00E0(); // CLS
    void OP_00EE(); // RET
    void OP_1nnn(); // JP addr
    void OP_2nnn(); // CALL addr
    void OP_3xkk(); // SE Vx, byte
    void OP_4xkk(); // SNE Vx, byte
    void OP_5xy0(); // SE Vx, Vy
    void OP_6xkk(); // LD Vx, byte
    void OP_7xkk(); // ADD Vx, byte 
    void OP_8xy0(); // LD Vx, Vy
    void OP_8xy1(); // OR Vx, Vy
    void OP_8xy2(); // AND Vx, Vy
    void OP_8xy3(); // XOR Vx, Vy
    void OP_8xy4(); // ADD Vx, Vy (VF = carry)
    void OP_8xy5(); // SUB Vx, Vy (VF = !borrow)
    void OP_8xy6(); // SHR Vx (VF = least significant bit)
    void OP_8xy7(); // SUBN Vx, Vy (VF = !borrow)
    void OP_8xyE(); // SHL Vx {, Vy} (VF = most significant bit)
    void OP_9xy0(); // SNE Vx, Vy
    void OP_Annn(); // LD I, addr
    void OP_Bnnn(); // JP V0, addr (addr + v0)
    void OP_Cxkk(); // RND Vx, byte (Vx = Random&byte)
    void OP_Dxyn(); // DRW Vx, Vy, nibble (draw nibble-byte sprite at mem-location I at (Vx, Vy), (VF = collision)
    void OP_Ex9E(); // SKP Vx 
    void OP_ExA1(); // SKNP Vx
    void OP_Fx07(); // LD Vx, DT
    void OP_Fx0A(); // LD Vx, K
    void OP_Fx15(); // LD DT, Vx
    void OP_Fx18(); // LD ST, Vx
    void OP_Fx1E(); // ADD I, Vx
    void OP_Fx29(); // LD F, Vx 
    void OP_Fx33(); // LD B, Vx
    void OP_Fx55(); // LD [I], Vx (write to memory from I to I+X)
    void OP_Fx65(); // LD Vx, [I] (read from memory fro I to I+X)
    void OP_NULL(); // No op

    // tables
    void Table0();
    void Table8();
    void TableE();
    void TableF();
};