#include <iostream>
#include <fstream>
#include <iomanip>

#include "EmulatorWrapper.h"
//#include "EmulatorConstants.h"

using namespace std;

int EmulatorWrapper::MEMORY_SIZE = 1 << 16;
int EmulatorWrapper::MEMORY_MAPPED_REGISTERS = 0xFF00;
int EmulatorWrapper::REGISTER_NUMBER = 9;
int EmulatorWrapper::BYTE = 1; // Memory acceptable size of reading;
int EmulatorWrapper::WORD = 2; // Memory acceptable size of reading;

short EmulatorWrapper::TERM_OUT = 0xFF00; // data out register (data goes on displey)
short EmulatorWrapper::TERM_IN = 0xFF02;  // data in register (data fetched from displey goes to this register)
short EmulatorWrapper::TIM_CFG = 0xFF10;  // configure time register

EmulatorWrapper::EmulatorWrapper(string input_file_name_)
    : input_file_name(input_file_name_),
      memory(MEMORY_SIZE, 0), registers(REGISTER_NUMBER, 0),
      running(false)
{
}

bool EmulatorWrapper::collect_data_from_relocatible_files()
{

    cout << "Collect data from file:" << input_file_name << endl;
    ifstream input_file(input_file_name, ios::binary);
    if (input_file.fail())
    {
        error_messages.push_back("File " + input_file_name + " does not exists!");
        return false;
    }

    int number_of_segments = 0;
    input_file.read((char *)&number_of_segments, sizeof(number_of_segments));
    cout << number_of_segments << endl;
    cout << hex;
    for (int i = 0; i < number_of_segments; i++)
    {
        // segment data
        Segment segment;

        // This is the start address where data should be loaded in memory
        input_file.read((char *)&segment.virtual_address, sizeof(segment.virtual_address));
        cout << segment.virtual_address << endl;

        // This will represent how many 1B-data there are in memory in each segment
        input_file.read((char *)&segment.number_of_bytes, sizeof(segment.number_of_bytes));
        cout << segment.number_of_bytes << endl;
        for (int j = 0; j < segment.number_of_bytes; j++)
        {
            char c;
            input_file.read((char *)(&c), sizeof(c));
            segment.data.push_back(c);
        }
        int counter = 0;

        for (int i = 0; i < segment.data.size(); i++)
        {
            char c = segment.data[i];
            if (counter % 8 == 0)
            {
                cout << hex << setfill('0') << setw(4) << (0xffff & counter + segment.virtual_address) << "   ";
            }
            cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
            counter++;
            if (counter % 8 == 0)
            {
                cout << endl;
            }
        }
        cout << endl;
        segments.push_back(segment);
    }
    input_file.close();
    return true;
}

bool EmulatorWrapper::load_data_to_memory()
{
    for (Segment s : segments)
    {
        for (int i = 0; i < s.data.size(); i++)
        {
            if (i + s.virtual_address > MEMORY_MAPPED_REGISTERS)
            {
                error_messages.push_back("Virtual address exceeds maximal user available memory");
                return false;
            }
            memory[i + s.virtual_address] = s.data[i];
        }
    }
    cout << "Loading data to memory ..." << endl;
    ofstream text_output_file("memory_snapshot.txt");
    int counter = 0;
    for (int i = 0; i < memory.size(); i++)
    {
        char c = memory[i];
        if (counter % 8 == 0)
        {
            text_output_file << hex << setfill('0') << setw(4) << (0xffff & counter) << "   ";
        }
        text_output_file << hex << setfill('0') << setw(2) << (0xff & c) << " ";
        counter++;
        if (counter % 8 == 0)
        {
            text_output_file << endl;
        }
    }
    cout << endl;

    return true;
}

void EmulatorWrapper::print_error_messages()
{

    for (string err : error_messages)
    {
        cout << "Emulator error: " << err << endl;
    }
}

short EmulatorWrapper::read_memory(int address, int size)
{
    if (size == 1)
    {
        // Only one address is reached because addresibile unit is 1B
        return memory[address];
    }
    else
    {
        // [H L] is the representation in memory
        char lower_value = memory[address];
        char higher_value = memory[address + 1];
        cout << higher_value << " " << lower_value << endl;
        // This is combination of lower and higher Byte. Lower Byte is only 8b and higher byte is expanded (its sign is expanded to the size of an int)
        short value = (short)((higher_value << 8) + (0xff & lower_value));
        cout << ":" << value << endl;
        return value;
    }
}

bool EmulatorWrapper::start_emulation()
{
    // First 16B in memory are Interrupt Vector Table's Entries
    // 0's entry is the entry point of the programm (pc <= ivt[0])
    rpc = read_memory(0, WORD);
    cout << "PC:" << rpc << endl;

    // Stack pointer is pointed to (1) occupied location and grows toward (2) lower addresses
    // First address of the stack is right below memory mapped space
    rsp = MEMORY_MAPPED_REGISTERS;
    cout << "SP:" << rsp << endl;

    // after starting the programm (ivt[0]) all interrupts should be allowed
    reset_flag(Tr);
    reset_flag(Tl);
    reset_flag(I);
    cout << "PSW:" << rpsw << endl;

    // Timer and terminal have to be set up before the beginning;
    reset_timer();
    reset_terminal();

    running = true;

    while (running)
    {
        // Emulator has several phases
        instruction_fetch();
        //        instruction_decode();
        //        instruction_execute();
        //        interrupt_ha
        break;
    }

    return true;
}

string EmulatorWrapper::decode_instruction(short instruction_description)
{
    // instruction_description is consisted of: operation_code (4b) | modificator(4b)
    cout << hex << setfill('0') << setw(2) << instruction_description << " ";
    char operation_code = (instruction_description >> 4) & 0xF;
    char modificator = instruction_description & 0xF;

    cout << hex << setw(1) << operation_code << " ";
    cout << hex << setw(1) << modificator << endl;
    string mnemonic = "";
    switch (operation_code)
    {
    case 0x2:
        cout << "X";
        return "X";
    case 0xA:
        cout << "K";
        return "X";
    default:
        return "Y";
    }
    /*
    case 0x0:
        cout << "A";
    case 0x1:
        cout << "B";
    case 0x2:
        cout << "C";
    case 0x3:
        cout << "D";
    case 0x4:
        cout << "E";
    case 0x5:
        cout << "F";
    case 0x6:
        cout << "G";
    case 0x7:
        cout << "H";
    case 0x8:
        cout << "I";
    case 0x9:
        cout << "J";
    case 0xA:
        cout << "K";
    case 0xB:
        cout << "L";
    default:
    {
        string error = "Instruction does not exists!";
        error_messages.push_back(error);
        return nullptr;
    }
    }
*/
    return mnemonic;
}

void EmulatorWrapper::instruction_fetch()
{
    short instruction_description = read_memory(rpc, BYTE);
    rpc += 1;
    instruction_mnemonic = decode_instruction(instruction_description);
}

void EmulatorWrapper::reset_timer()
{
    //Timer is in progress
}

void EmulatorWrapper::reset_terminal()
{
    //Terminal is in progress
}

void EmulatorWrapper::reset_flag(short flag)
{
    rpsw &= ~flag;
}

void EmulatorWrapper::set_flag(short flag)
{
    rpsw |= flag;
}
