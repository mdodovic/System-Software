#ifndef EMULATOR_H
#define EMULATOR_H
#include <string>
#include <vector>
#include <map>
#include "EmulatorConstants.h"
using namespace std;

class EmulatorWrapper
{
    struct Segment
    {
        int virtual_address; // virtual address of segment
        int number_of_bytes; // size of data vector
        vector<char> data;   // data at offset in vector offsets
    };

    vector<Segment> segments;

    string input_file_name;
    vector<string> error_messages;

    vector<char> memory; // Memory: allocatible size is 1

    vector<short> registers; // Register bank; register with specific meaning r6 is sp, r7 is pc, r8 is psw
    short &rpc = registers[pc];
    short &rsp = registers[sp];
    short &rpsw = registers[psw];

public:
    EmulatorWrapper(string);
    bool collect_data_from_relocatible_files();
    bool load_data_to_memory();
    void print_error_messages();

    bool start_emulation();

    static int MEMORY_SIZE;
    static int REGISTER_NUMBER;
    static int MEMORY_MAPPED_REGISTERS;
};

#endif //EMULATOR_H