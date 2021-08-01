#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <iomanip>
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

    enum InstructionMnemonic
    {
        halt,
        int_i,
        iret,
        call,
        ret,
        jmp,
        jeq,
        jne,
        jgt,
        xchg,
        add,
        sub,
        mul,
        div,
        cmp,
        not_i,
        and_i,
        or_i,
        xor_i,
        test,
        shl,
        shr,
        ldr,
        str
    };

    enum AddressType
    {
        imm,
        reg_dir,
        reg_ind,
        reg_ind_displ,
        mem_dir,
        reg_ind_addition
    };

    enum UpdateType
    {
        no_update,
        pre_auto_dec,
        pre_auto_inc,
        post_auto_dec,
        post_auto_inc,
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
    InstructionMnemonic instruction_mnemonic; // This is first byte of instruction
    int instruction_size;                     // Instruction's number of bytes
    int destination_register_number;
    int source_register_number;
    int update_type;           // This is the way how to update the choosen tegister
    int address_type;          // Way of addressing
    short instruction_payload; // This is the 2B of data, ordered by [H L]

    short fetch_operand_by_addressing_type();           // depends on addressing type, fetch operand for necessary instructions
    bool set_operand_by_addressing_type(short);         // depends on addressing type, set operand on given value for necessary instructions
    void update_before_address_fetch_source_register(); // depends on update type, update source register before usage
    void update_after_address_fetch_source_register();  // depends on update type, update source register after usage

    int previous_pc; // pc before the next instruction fetching is saved

    bool instruction_fetch_and_decode();

    bool instruction_execute();

    // Memory
    vector<char> memory;                             // Memory: allocatible size is 1
    short read_memory(int, int, bool = true);        // read value (size of falue is 1B or 2B depends on second argument) from memory at position (first argument)
                                                     // third argument said that type of data is loaded by little endian (true) of big endian
                                                     // it is predefined to be true for the case that fetch only one byte. With WORD it is recommended to use this third argument
    void write_memory(short, int, int, bool = true); // write value (first argument) to memory at position (second argument) which is size equals to third argument
                                                     // forth argument said that type of data that should be written is little endian (true) of big endian
                                                     // it is predefined to be true for the case that fetch only one byte. With WORD it is recommended to use this fourth argument
    void create_memory_snapshot(string);

    // Registers
    vector<short> registers; // Register bank; register with specific meaning r6 is sp, r7 is pc, r8 is psw
    short &rpc = registers[pc];

    short &rsp = registers[sp];
    void push_on_stack(short);
    short pop_from_stack();

    short &rpsw = registers[psw];
    void reset_flag(short);
    void set_flag(short);
    bool get_flag(short);
    bool calculate_condition(short);

    // Interrupts
    vector<bool> interrupts_requests; // this simulates interrupt request lines from periferies to cpu
                                      // there are only 2 periferies: terminal and timer
                                      // priorities are terminal(0) and then timer(1) and that are the enties in this array

    void set_interrupt_request_on_line(int); // set interrupt request from device
    void interrupt_requests_handler();       // check if some i/o device send interrupt request
    void jump_on_interrupt_routine(short);   // jump on interrupt routine with given ivt entry (first argument)
                                             // push(pc); push(psw); pc = mem[(entry % 8) * 2];

    // Timer
    short timer_period_identificator; // detemine which period should the timer reach
    long long int timer_period;       // defined period by timer_period_identifier

    bool current_counting; // if the timer is currently in counding period or new counting should start

    long long int previous_time;
    long long int current_time;

    long long int fetch_duration_by_identifier(short); // return period in *ms* defermined by timer_period_identificator
    void reset_timer();
    void timer_tick();

    // Terminal
    void reset_terminal();

    ofstream emulator_output_file;

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

    static int START_PROGRAM_ADDRESS_ENTRY; // entry in ivt for program beginning
    static int ERROR_IN_PROGRAM_ENTRY;      // entry in ivt error (invalid instruction, invalid addressing type, zero division ...) happened
    static int TIMER_ENTRY;                 // entry in ivt for timer interrupts
    static int TERMINAL_ENTRY;              // entry in ivt for terminal interrupts

    static int NUMBER_OF_PERIFERIES; // number of periferies that can send interrupt request (=2 in this case)
    static int TERMINAL_LINE_NUMBER; // index of terminal line request
    static int TIMER_LINE_NUMBER;    // index of timer line request

    static short TERM_OUT; // data out register (data goes on displey)
    static short TERM_IN;  // data in register (data fetched from displey goes to this register)
    static short TIM_CFG;  // configure time register
};

#endif //EMULATOR_H