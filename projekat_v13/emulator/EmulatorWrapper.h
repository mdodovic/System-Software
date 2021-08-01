#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>
#include <map>
//#include "EmulatorConstants.h"

using namespace std;

class EmulatorWrapper
{
    enum Register
    {
        r0 = 0,
        r1 = 1,
        r2 = 2,
        r3 = 3,
        r4 = 4,
        r5 = 5,
        r6 = 6,
        sp = r6,
        r7 = 7,
        pc = r7,
        psw = 8,
        unimportant_reg = 0xF
    };

    enum Flag
    {
        Z = 1 << 0,
        O = 1 << 1,
        C = 1 << 2,
        N = 1 << 3,
        Tr = 1 << 13,
        Tl = 1 << 14,
        I = 1 << 15
    };

    struct Segment
    {
        int virtual_address; // virtual address of segment
        int number_of_bytes; // size of data vector
        vector<char> data;   // data at offset in vector offsets
    };

    vector<Segment> segments;
    string input_file_name;
    vector<string> error_messages;

    bool running; // programm status

    // Instruction
    string instruction_mnemonic; // This is first byte of instruction
    int instruction_size;        // Instruction's number of bytes
    int destination_register_number;
    int source_register_number;
    int update_type;  // This is the way how to update the choosen tegister
    int address_type; // Way of addressing
    void instruction_fetch();
    string decode_instruction(short);

    // Memory
    vector<char> memory;         // Memory: allocatible size is 1
    short read_memory(int, int); // read value (size of falue is 1B or 2B depends on second argument) from memory at position (first argument)

    // Registers
    vector<short> registers; // Register bank; register with specific meaning r6 is sp, r7 is pc, r8 is psw
    short &rpc = registers[pc];
    short &rsp = registers[sp];

    short &rpsw = registers[psw];
    void reset_flag(short);
    void set_flag(short);

    // Timer
    void reset_timer();

    // Terminal
    void reset_terminal();

public:
    EmulatorWrapper(string);
    bool collect_data_from_relocatible_files();
    bool load_data_to_memory();
    void print_error_messages();

    bool start_emulation();

    static int MEMORY_SIZE;             // total size of memory virtual address space
    static int MEMORY_MAPPED_REGISTERS; // Start address of space where I/O controller registers are loaded to memory
    static int REGISTER_NUMBER;         // Number of processor's register
    static int BYTE;                    // Memory acceptable size of reading;
    static int WORD;                    // Memory acceptable size of reading;

    static short TERM_OUT; // data out register (data goes on displey)
    static short TERM_IN;  // data in register (data fetched from displey goes to this register)
    static short TIM_CFG;  // configure time register
};

#endif //EMULATOR_H