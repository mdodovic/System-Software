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

int EmulatorWrapper::START_PROGRAM_ADDRESS_ENTRY = 0; // entry in ivt for program beginning
int EmulatorWrapper::ERROR_IN_PROGRAM_ENTRY = 1;      // entry in ivt error (invalid instruction, invalid addressing type, zero division ...) happened
int EmulatorWrapper::TIMER_ENTRY = 2;                 // entry in ivt for timer interrupts
int EmulatorWrapper::TERMINAL_ENTRY = 3;              // entry in ivt for terminal interrupts

short EmulatorWrapper::TERM_OUT = 0xFF00; // data out register (data goes on displey)
short EmulatorWrapper::TERM_IN = 0xFF02;  // data in register (data fetched from displey goes to this register)
short EmulatorWrapper::TIM_CFG = 0xFF10;  // configure time register

EmulatorWrapper::EmulatorWrapper(string input_file_name_)
    : input_file_name(input_file_name_),
      memory(MEMORY_SIZE, 0), registers(REGISTER_NUMBER, 0),
      running(false), emulator_output_file("emulator_useful_output.txt")
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
    create_memory_snapshot("memory_snapshot_begin.txt");
    return true;
}

void EmulatorWrapper::create_memory_snapshot(string filename)
{
    ofstream text_output_file(filename);
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
    text_output_file.close();
}

void EmulatorWrapper::print_error_messages()
{

    for (string err : error_messages)
    {
        cout << "Emulator error: " << err << endl;
    }
}

short EmulatorWrapper::read_memory(int address, int size, bool little_endian)
{
    if (size == 1)
    {
        // Only one address is reached because addresibile unit is 1B
        return memory[address];
    }
    else
    {
        if (little_endian)
        {
            // [L H] is the representation in memory
            char lower_value = memory[address];
            char higher_value = memory[address + 1];
            cout << " _READ:Val=> " << (int)higher_value << " " << (int)lower_value;
            // This is combination of lower and higher Byte. Lower Byte is only 8b and higher byte is expanded (its sign is expanded to the size of an int)
            short value = (short)((higher_value << 8) + (0xff & lower_value));
            cout << " :" << value << "; READ_";
            return value;
        }
        else
        {
            // [H L] is the representation in memory
            char higher_value = memory[address];
            char lower_value = memory[address + 1];
            cout << " _READ:Val=> " << (int)higher_value << " " << (int)lower_value;
            // This is combination of lower and higher Byte. Lower Byte is only 8b and higher byte is expanded (its sign is expanded to the size of an int)
            short value = (short)((higher_value << 8) + (0xff & lower_value));
            cout << " :" << value << "; READ_";
            return value;
        }
    }
}

void EmulatorWrapper::write_memory(short value, int address, int size, bool little_endian)
{
    if (size == 1)
    {
        // Only one address is reached because addresibile unit is 1B
        memory[address] = value;
    }
    else
    {
        char lower_value = value & 0xff;
        char higher_value = (value >> 8) & 0xff;
        cout << " _WRITE:Val: " << (int)higher_value << " " << (int)lower_value << "! ";
        if (little_endian)
        {
            // [L H] is the representation in memory
            cout << " [0, 1] " << (int)memory[address] << ", " << (int)memory[address + 1] << " => ";
            memory[address] = lower_value;
            memory[address + 1] = higher_value;
            cout << (int)memory[address] << ", " << (int)memory[address + 1] << "; WRITE_";
        }
        else
        {
            // [H L] is the representation in memory
            cout << " [0, 1] " << (int)memory[address] << ", " << (int)memory[address + 1] << " => ";
            memory[address] = higher_value;
            memory[address + 1] = lower_value;
            cout << (int)memory[address] << ", " << (int)memory[address + 1] << "; WRITE_";
        }
    }

    if (address == TERM_OUT)
    {
        cout << ">>" << (char)value << "<< ; WRITE_" << flush;
        cout << (char)value << flush << endl;
    }
}

bool EmulatorWrapper::start_emulation()
{
    // First 16B in memory are Interrupt Vector Table's Entries
    // 0's entry is the entry point of the programm (pc <= ivt[0])
    rpc = read_memory(START_PROGRAM_ADDRESS_ENTRY * 2, WORD);
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
        previous_pc = rpc;

        // Emulator has several phases
        if (instruction_fetch_and_decode() == false)
        {
            cout << error_messages[0] << endl;
        }
        else
        {
            cout << "OK(s:" << instruction_size << ")_I!" << endl;
        }
        if (instruction_execute() == false)
        {
            cout << error_messages[0] << endl;
        }
        else
        {
            cout << "OK_E!" << endl;
        }

        // check interrupts:
        // from timer
        // from terminal

        cout << endl;
    }
    create_memory_snapshot("memory_snapshot_end.txt");

    return true;
}

bool EmulatorWrapper::instruction_fetch_and_decode()
{
    short instruction_description = read_memory(rpc, BYTE);
    rpc += 1;

    // instruction_description is consisted of: operation_code (4b) | modificator(4b)
    cout << hex << setfill('0') << setw(2) << instruction_description << " ";
    char operation_code = (instruction_description >> 4) & 0xF;
    char modificator = instruction_description & 0xF;

    switch (operation_code)
    {
    case 0x0:
    {
        // Only instruction with that operation_code is HALT
        // it is 1B in size => no further action needed
        instruction_mnemonic = halt;
        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }
        return true;
    }
    case 0x1:
    {
        // - Not tested
        // Only instruction with that operation_code is INT
        // it is 2B in size
        instruction_mnemonic = int_i;
        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }
        // another 1B needs to be fetched: description of the registers (dest|src)
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;

        source_register_number = register_description & 0xF;

        if (destination_register_number != 0xF)
        {
            string error = "Instruction with that operation code and defined source register does not exists!";
            error_messages.push_back(error);
            return false;
        }

        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        return true;
    }
    case 0x2:
    {
        // - Not tested yet
        // Only instruction with that operation_code is IRET
        // it is 1B in size => no further action needed
        instruction_mnemonic = iret;
        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }
        return true;
    }
    case 0x3:
    {
        // This is the call instruction
        instruction_mnemonic = call;
        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }
        // we should fetch at least another 2B
        // first is the description of the registers (dest|src)
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        if (destination_register_number != 0xF)
        {
            string error = "Instruction with that operation code and defined register does not exists!";
            error_messages.push_back(error);
            return false;
        }

        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        // second is type of addressing and type of update related register (update_type|address_type)
        short update_address_type = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << update_address_type << " ";
        update_type = (update_address_type >> 4) & 0xF;
        address_type = update_address_type & 0xF;
        cout << hex << "(a:" << address_type << ";u:" << update_type << ") ";

        // Size of ldr instruction is 3 or 5B
        instruction_size = 3;
        // if the addressing type includes payload, then the size is 5B.
        if (address_type == imm || address_type == reg_ind_displ || address_type == mem_dir || address_type == reg_ind_addition)
        {
            instruction_size += 2;
            instruction_payload = read_memory(rpc, WORD, false);
            rpc += 2;
            cout << hex << setfill('0') << setw(2) << instruction_payload << " ";
        }

        return true;
    }
    case 0x4:
    {
        // Only instruction with that operation_code is RET
        // it is 1B in size => no further action needed
        instruction_mnemonic = ret;
        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }
        return true;
    }
    case 0x5:
    {
        // This is the group of the brench instuctions (jmp, jeq, jne, jgt) instructions
        switch (modificator)
        {
            // instruction mnemonic depends on modificatios.
        case 0x0:
            cout << " !" << (int)modificator << "->jmp! ";
            instruction_mnemonic = jmp;
            break;
        case 0x1:
            cout << " !" << (int)modificator << "->jeq! ";
            instruction_mnemonic = jeq;
            break;
        case 0x2:
            cout << " !" << (int)modificator << "->jne! ";
            instruction_mnemonic = jne;
            break;
        case 0x3:
            cout << " !" << (int)modificator << "->jgt! ";
            instruction_mnemonic = jgt;
            break;

        default:
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        } // end of switch that conctretize brench instruction by mnemonic
        // we should fetch at least another 2B
        // first is the description of the registers (dest|src)
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        if (destination_register_number != 0xF)
        {
            string error = "Instruction with that operation code and defined register does not exists!";
            error_messages.push_back(error);
            return false;
        }
        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        // second is type of addressing and type of update related register (update_type|address_type)
        short update_address_type = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << update_address_type << " ";
        update_type = (update_address_type >> 4) & 0xF;
        address_type = update_address_type & 0xF;
        cout << hex << "(a:" << address_type << ";u:" << update_type << ") ";

        // Size of ldr instruction is 3 or 5B
        instruction_size = 3;
        // if the addressing type includes payload, then the size is 5B.
        if (address_type == imm || address_type == reg_ind_displ || address_type == mem_dir || address_type == reg_ind_addition)
        {
            instruction_size += 2;
            instruction_payload = read_memory(rpc, WORD, false);
            rpc += 2;
            cout << hex << setfill('0') << setw(2) << instruction_payload << " ";
        }

        return true;
    }
    case 0x6:
    {
        // - Not tested yet
        // Only instruction with that operation_code is XCHG
        instruction_mnemonic = xchg;
        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }

        // This instruction is 2B size
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        return true;
    }
    case 0x7:
    {
        // This is the group of the aritmetic instuctions (add, sub, mul, div, cmp) instructions
        switch (modificator)
        {
            // instruction mnemonic depends on modificatios.
        case 0x0:
            cout << " !" << (int)modificator << "->add! ";
            instruction_mnemonic = add;
            break;
        case 0x1:
            cout << " !" << (int)modificator << "->sub! ";
            instruction_mnemonic = sub;
            break;
        case 0x2:
            cout << " !" << (int)modificator << "->mul! ";
            instruction_mnemonic = mul;
            break;
        case 0x3:
            cout << " !" << (int)modificator << "->div! ";
            instruction_mnemonic = div;
            break;
        case 0x4:
            cout << " !" << (int)modificator << "->cmp! ";
            instruction_mnemonic = cmp;
            break;

        default:
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        } // end of switch that conctretize aritmetic instruction by mnemonic
          // All aritmetic instructions have the same 2B size
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        return true;
    }
    case 0x8:
    {
        // - Not tested yet
        // This is the group of the logic instuctions (not, and, or, xor, test) instructions
        switch (modificator)
        {
            // instruction mnemonic depends on modificatios.
        case 0x0:
            cout << " !" << (int)modificator << "->not! ";
            instruction_mnemonic = not_i;
            break;
        case 0x1:
            cout << " !" << (int)modificator << "->and! ";
            instruction_mnemonic = and_i;
            break;
        case 0x2:
            cout << " !" << (int)modificator << "->or! ";
            instruction_mnemonic = or_i;
            break;
        case 0x3:
            cout << " !" << (int)modificator << "->xor! ";
            instruction_mnemonic = xor_i;
            break;
        case 0x4:
            cout << " !" << (int)modificator << "->test! ";
            instruction_mnemonic = test;
            break;

        default:
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        } // end of switch that conctretize logic instruction by mnemonic
          // All logic instructions have the same 2B size
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        return true;
    }
    case 0x9:
    {
        // - Not tested yet
        // This is the group of the shift instuctions (shl, shr) instructions
        switch (modificator)
        {
            // instruction mnemonic depends on modificatios.
        case 0x0:
            cout << " !" << (int)modificator << "->shl! ";
            instruction_mnemonic = shl;
            break;
        case 0x1:
            cout << " !" << (int)modificator << "->shr! ";
            instruction_mnemonic = shr;
            break;

        default:
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        } // end of switch that conctretize shift instruction by mnemonic
          // All shift instructions have the same 2B size
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        return true;
    }
    case 0xA:
    {
        // This is the group of the load instructions
        instruction_mnemonic = ldr;

        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }
        // we should fetch at least another 2B
        // first is the description of the registers (dest|src)
        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        // second is type of addressing and type of update related register (update_type|address_type)
        short update_address_type = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << update_address_type << " ";
        update_type = (update_address_type >> 4) & 0xF;
        address_type = update_address_type & 0xF;
        cout << hex << "(a:" << address_type << ";u:" << update_type << ") ";

        // Size of ldr instruction is 3 or 5B
        instruction_size = 3;
        // if the addressing type includes payload, then the size is 5B.
        if (address_type == imm || address_type == reg_ind_displ || address_type == mem_dir || address_type == reg_ind_addition)
        {
            instruction_size += 2;
            instruction_payload = read_memory(rpc, WORD, false);
            rpc += 2;
            cout << hex << setfill('0') << setw(2) << instruction_payload << " ";
        }

        return true;
    }
    case 0xB:
    {
        // This is the group of the load instructions
        instruction_mnemonic = str;
        if (modificator != 0x0)
        {
            string error = "Instruction with that operation code and modifier does not exists!";
            error_messages.push_back(error);
            return false;
        }
        // we should fetch at least another 2B
        // first is the description of the registers (dest|src)

        short register_description = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << register_description << " ";
        destination_register_number = (register_description >> 4) & 0xF;
        source_register_number = register_description & 0xF;
        cout << hex << "(d:" << destination_register_number << ";s:" << source_register_number << ") ";

        // second is type of addressing and type of update related register (update_type|address_type)
        short update_address_type = read_memory(rpc, BYTE);
        rpc += 1;
        cout << hex << setfill('0') << setw(2) << update_address_type << " ";
        update_type = (update_address_type >> 4) & 0xF;
        address_type = update_address_type & 0xF;
        cout << hex << "(a:" << address_type << ";u:" << update_type << ") ";

        // Size of ldr instruction is 3 or 5B
        instruction_size = 3;
        // if the addressing type includes payload, then the size is 5B.
        if (address_type == imm || address_type == reg_ind_displ || address_type == mem_dir || address_type == reg_ind_addition)
        {
            instruction_size += 2;
            instruction_payload = read_memory(rpc, WORD, false);
            rpc += 2;
            cout << hex << setfill('0') << setw(2) << instruction_payload << " ";
        }

        return true;
    }
    default:

        string error = "Instruction does not exists!";
        error_messages.push_back(error);
        return true;
    }
}

void EmulatorWrapper::update_before_address_fetch_source_register()
{
    switch (update_type)
    {
    case no_update:
        cout << " ,no pre update, ";
        return;
    case pre_auto_dec:
        cout << " ,pre update (" << registers[source_register_number] << "->";
        registers[source_register_number] -= 2;
        cout << registers[source_register_number] << "), ";

        return;
    case pre_auto_inc:
        cout << " ,pre update (" << registers[source_register_number] << "->";
        registers[source_register_number] += 2;
        cout << registers[source_register_number] << "), ";
        return;

    default:
        break;
    }
}

void EmulatorWrapper::update_after_address_fetch_source_register()
{
    switch (update_type)
    {
    case no_update:
        cout << " ,no post update, ";
        return;
    case post_auto_dec:
        cout << " ,post update (" << registers[source_register_number] << "->";
        registers[source_register_number] -= 2;
        cout << registers[source_register_number] << "), ";
        return;
    case post_auto_inc:
        cout << " ,post update (" << registers[source_register_number] << "->";
        registers[source_register_number] += 2;
        cout << registers[source_register_number] << "), ";

    default:
        break;
    }
}

short EmulatorWrapper::fetch_operand_by_addressing_type()
{
    short operand;
    switch (address_type)
    { // Fetch operand
    case imm:
    {
        operand = instruction_payload;
        cout << "imm+ (" << operand << ")";
        break;
    }
    case reg_dir:
    {
        operand = registers[source_register_number];
        cout << "reg_dir (" << operand << ")";
        break;
    }
    case reg_ind:
    {
        operand = read_memory(registers[source_register_number], WORD, true);
        cout << "reg_ind+ (" << operand << ")";
        break;
    }
    case reg_ind_displ:
    {
        operand = read_memory(registers[source_register_number] + instruction_payload, WORD, true);
        cout << "reg ind disp (" << operand << ")";
        break;
    }
    case mem_dir:
    {
        operand = read_memory(instruction_payload, WORD, true);
        cout << "mem dir (" << operand << ")";
        break;
    }
    case reg_ind_addition:
    {
        operand = registers[source_register_number] + instruction_payload;
        cout << "reg ind add  (" << operand << ")";
        break;
    }
    default:
    {
        cout << "ERROR ";
        break;
    }
    } // End of addressing type switch: operand is ready
    return operand;
}

bool EmulatorWrapper::set_operand_by_addressing_type(short value)
{

    switch (address_type)
    { // Fetch operand
    case imm:
    {
        string error = "Store in immediate value is forbidden";
        error_messages.push_back(error);
        return false;
    }
    case reg_dir:
    {
        cout << "reg_dir (" << registers[source_register_number];
        registers[source_register_number] = value;
        cout << "->" << registers[source_register_number] << ")";
        return true;
    }
    case reg_ind:
    {
        int address_as_operand = registers[source_register_number];

        cout << "reg_ind (" << read_memory(registers[source_register_number], WORD, true);
        write_memory(value, registers[source_register_number], WORD, true);
        cout << "->" << read_memory(registers[source_register_number], WORD, true) << ")";
        return true;
    }
    case reg_ind_displ:
    {
        int address_as_operand = registers[source_register_number] + instruction_payload;

        cout << "reg ind disp (" << read_memory(address_as_operand, WORD, true);
        write_memory(value, address_as_operand, WORD, true);
        cout << "->" << read_memory(address_as_operand, WORD, true) << ")";
        return true;
    }
    case mem_dir:
    {
        int address_as_operand = instruction_payload;

        cout << "mem dir (" << read_memory(address_as_operand, WORD, true);
        write_memory(value, address_as_operand, WORD, true);
        cout << "->" << read_memory(address_as_operand, WORD, true) << ")";

        return true;
    }
    case reg_ind_addition:
    {
        string error = "Store in immediate value is forbidden";
        error_messages.push_back(error);
        return false;
    }
    default:
    {
        cout << "ERROR ";
        return false;
    }
    } // End of addressing type switch: operand is set
}

bool EmulatorWrapper::instruction_execute()
{
    switch (instruction_mnemonic)
    {
    case halt:
    {
        // stop the further execution of the program
        cout << "halt! ";
        running = false;
        return true;
    } // end of case halt

    case int_i:
    {
        //* push pc
        //* push psw
        //* pc <= mem[(reg[Dest] mod 8 ) * 2];
        cout << "int r" << destination_register_number << "(" << registers[destination_register_number] << ")";

        // execute instruction:
        push_on_stack(rpc);
        push_on_stack(rpsw);
        rpc = read_memory((registers[destination_register_number] % 8) * 2, WORD, true);

        cout << " #> " << registers[pc];
        // End of int instruction, everything passed well
        return true;
    }
    case iret:
    {
        //* pop psw
        //* pop pc
        cout << "iret (psw:" << registers[psw] << ",pc:" << registers[pc] << ")";

        // execute instruction:
        rpsw = pop_from_stack();
        rpc = pop_from_stack();

        cout << " #> psw:" << registers[psw] << ",pc:" << registers[pc];
        // End of iret instruction, everything passed well
        return true;
    }

    case call:
    {
        //* push pc;
        //* pc <= operand
        cout << "call ";

        // if the update type is pre_ :
        update_before_address_fetch_source_register();

        // execute instruction:
        push_on_stack(rpc);
        short operand = fetch_operand_by_addressing_type();
        rpc = operand;

        // if the update type is post_ :
        update_after_address_fetch_source_register();
        cout << " #> " << read_memory(rsp, WORD, true) << registers[pc];
        // End of call instruction, everything passed well
        return true;
    } // end of case call
    case ret:
    {
        //* pop pc
        cout << "ret pc: " << rpc;

        // execute instruction:
        rpc = pop_from_stack();
        cout << " #> " << registers[pc];
        // End of ret instruction, everything passed well
        return true;
    }
    case jmp:
    {
        //* pc <= operand
        cout << "jmp pc = " << rpc;

        // if the update type is pre_ :
        update_before_address_fetch_source_register();

        short operand = fetch_operand_by_addressing_type();

        // execute instruction:
        bool condition = true;
        if (condition)
        {
            rpc = operand;
        }
        // if the update type is post_ :
        update_after_address_fetch_source_register();

        cout << " #> " << registers[pc];
        // End of jmp instruction, everything passed well
        return true;
    }
    case jeq:
    {
        //* if(equal) pc <= operand
        cout << "jeq pc = " << rpc << " ?psw: " << rpsw;

        // if the update type is pre_ :
        update_before_address_fetch_source_register();

        short operand = fetch_operand_by_addressing_type();

        // execute instruction:
        bool condition = calculate_condition(jeq);

        if (condition)
        {
            rpc = operand;
        }
        // if the update type is post_ :
        update_after_address_fetch_source_register();

        cout << " #> " << registers[pc];
        // End of jeq instruction, everything passed well

        return true;
    }
    case jne:
    {
        //* if(not equal) pc <= operand

        cout << "jne pc = " << rpc << " ?psw: " << rpsw;

        // if the update type is pre_ :
        update_before_address_fetch_source_register();

        short operand = fetch_operand_by_addressing_type();

        // execute instruction:
        bool condition = calculate_condition(jne);

        if (condition)
        {
            rpc = operand;
        }
        // if the update type is post_ :
        update_after_address_fetch_source_register();

        cout << " #> " << registers[pc];
        // End of jne instruction, everything passed well

        return true;
    }
    case jgt:
    {
        //* if(signed greater) pc <= operand

        cout << "jgt pc = " << rpc << " ?psw: " << rpsw;

        // if the update type is pre_ :
        update_before_address_fetch_source_register();

        short operand = fetch_operand_by_addressing_type();

        // execute instruction:
        bool condition = calculate_condition(jgt);

        if (condition)
        {
            rpc = operand;
        }
        // if the update type is post_ :
        update_after_address_fetch_source_register();

        cout << " #> " << registers[pc];
        // End of jgt instruction, everything passed well

        return true;
    }
    case xchg:
    {
        //* tmp <= reg[Dest]; reg[Dest] <= reg[Src];  reg[Src] <= tmp;

        cout << "xchg r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " += r" << source_register_number << "(" << registers[source_register_number] << ")";

        // execute intruction
        short tmp = registers[destination_register_number];
        registers[destination_register_number] = registers[source_register_number];
        registers[source_register_number] = tmp;

        cout << " #> r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " += r" << source_register_number << "(" << registers[source_register_number] << ")";

        // End of xchg instruction, everything passed well
        return true;
    }

    case add:
    {
        //* reg[Dest] <= (reg[Dest] + reg[Src])
        cout << "add r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " += r" << source_register_number << "(" << registers[source_register_number] << ")";
        registers[destination_register_number] = registers[destination_register_number] + registers[source_register_number];

        cout << " #> " << registers[destination_register_number];
        // End of add instruction, everything passed well
        return true;
    }
    case sub:
    {
        //* reg[Dest] <= (reg[Dest] - reg[Src])
        cout << "sub r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " -= r" << source_register_number << "(" << registers[source_register_number] << ")";
        registers[destination_register_number] = registers[destination_register_number] - registers[source_register_number];

        cout << " #> " << registers[destination_register_number];

        // End of sub instruction, everything passed well
        return true;
    }
    case mul:
    {
        //* reg[Dest] <= (reg[Dest] * reg[Src])
        cout << "mul r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " *= r" << source_register_number << "(" << registers[source_register_number] << ")";
        registers[destination_register_number] = registers[destination_register_number] * registers[source_register_number];

        cout << " #> " << registers[destination_register_number];

        // End of mul instruction, everything passed well
        return true;
    }
    case div:
    {
        //* reg[Dest] <= (reg[Dest] / reg[Src])
        cout << "div r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " /= r" << source_register_number << "(" << registers[source_register_number] << ")";
        if (registers[source_register_number] == 0)
        {
            cout << "DIVISION WITH ZERO! ";
            break;
        }
        registers[destination_register_number] = registers[destination_register_number] / registers[source_register_number];

        cout << " #> " << registers[destination_register_number];

        // End of div instruction, everything passed well
        return true;
    }
    case cmp:
    {
        //* temp <= (reg[Dest] - reg[Src]); update_psw(NZCO)
        cout << "cmp r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " ? r" << source_register_number << "(" << registers[source_register_number] << ")";

        cout << " #psw: " << rpsw;
        short temp = registers[destination_register_number] - registers[source_register_number];
        if (temp == 0)
        {
            set_flag(Z);
        }
        else
        {
            reset_flag(Z);
        }
        if (temp < 0)
        {
            set_flag(N);
        }
        else
        {
            reset_flag(N);
        }
        if (registers[destination_register_number] < registers[source_register_number])
        {
            set_flag(C);
        }
        else
        {
            reset_flag(C);
        }
        short a = registers[destination_register_number];
        short b = registers[source_register_number];
        if ((a > 0 && b < 0 && (a - b) < 0) || (a < 0 && b > 0 && (a - b) > 0))
        {
            set_flag(O);
        }
        else
        {
            reset_flag(O);
        }

        cout << " #> " << registers[psw];
        // End of cmp instruction, everything passed well
        return true;
    }

    case not_i:
    {
        //* reg[Dest] <= ~reg[Dest]

        cout << "not r" << destination_register_number << "(" << registers[destination_register_number] << ")";

        registers[destination_register_number] = !registers[destination_register_number];

        cout << " #> " << registers[destination_register_number];

        // End of not instruction, everything passed well
        return true;
    }
    case and_i:
    {
        //* reg[Dest] <= (reg[Dest] & reg[Src])
        cout << "and r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " &= r" << source_register_number << "(" << registers[source_register_number] << ")";

        registers[destination_register_number] = registers[destination_register_number] & registers[source_register_number];

        cout << " #> " << registers[destination_register_number];

        // End of and instruction, everything passed well
        return true;
    }
    case or_i:
    {
        //* reg[Dest] <= (reg[Dest] | reg[Src])
        cout << "and r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " |= r" << source_register_number << "(" << registers[source_register_number] << ")";

        registers[destination_register_number] = registers[destination_register_number] | registers[source_register_number];

        cout << " #> " << registers[destination_register_number];

        // End of and instruction, everything passed well
        return true;
    }
    case xor_i:
    {
        //* reg[Dest] <= (reg[Dest] ^ reg[Src])
        cout << "and r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " ^= r" << source_register_number << "(" << registers[source_register_number] << ")";

        registers[destination_register_number] = registers[destination_register_number] ^ registers[source_register_number];

        cout << " #> " << registers[destination_register_number];

        // End of and instruction, everything passed well
        return true;
    }
    case test:
    {
        //* temp <= (reg[Dest] & reg[Src]); update_psw(NZ)
        cout << "test r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " ? r" << source_register_number << "(" << registers[source_register_number] << ")";

        cout << " #psw: " << rpsw;
        short temp = registers[destination_register_number] & registers[source_register_number];
        if (temp == 0)
        {
            set_flag(Z);
        }
        else
        {
            reset_flag(Z);
        }
        if (temp < 0)
        {
            set_flag(N);
        }
        else
        {
            reset_flag(N);
        }

        cout << " #> " << registers[psw];
        // End of cmp instruction, everything passed well
        return true;
    }
    case shl:
    {
        //* reg[Dest] <= reg[Dest] << reg[Src]; update_psw(NZC)
        cout << "shl r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " << = r" << source_register_number << "(" << registers[source_register_number] << ")";

        cout << " #psw: " << rpsw;
        short prev_dest = registers[destination_register_number];
        short prev_src = registers[source_register_number];

        // execute instruction
        registers[destination_register_number] = registers[destination_register_number] << registers[source_register_number];
        // update psw (NZC)

        if (registers[destination_register_number] == 0)
        {
            set_flag(Z);
        }
        else
        {
            reset_flag(Z);
        }
        if (registers[destination_register_number] < 0)
        {
            set_flag(N);
        }
        else
        {
            reset_flag(N);
        }
        if (prev_src < 16 && ((prev_dest >> (16 - prev_src)) & 1))
        {
            set_flag(C);
        }
        else
        {
            reset_flag(C);
        }
        cout << " #> " << registers[psw] << ";" << registers[destination_register_number] << ";;";
        // End of shl instruction, everything passed well
    }
    case shr:
    {
        //* reg[Dest] <= reg[Dest] >> reg[Src]; update_psw(NZC)
        cout << "shr r" << destination_register_number << "(" << registers[destination_register_number] << ")"
             << " >> = r" << source_register_number << "(" << registers[source_register_number] << ")";

        cout << " #psw: " << rpsw;
        short prev_dest = registers[destination_register_number];
        short prev_src = registers[source_register_number];

        // execute instruction
        registers[destination_register_number] = registers[destination_register_number] >> registers[source_register_number];
        // update psw (NZC)

        if (registers[destination_register_number] == 0)
        {
            set_flag(Z);
        }
        else
        {
            reset_flag(Z);
        }
        if (registers[destination_register_number] < 0)
        {
            set_flag(N);
        }
        else
        {
            reset_flag(N);
        }
        if ((prev_dest >> (prev_src - 1)) & 1)
        {
            set_flag(C);
        }
        else
        {
            reset_flag(C);
        }
        cout << " #> " << registers[psw] << ";" << registers[destination_register_number] << ";;";
        // End of shr instruction, everything passed well
    }

    case ldr:
    {
        //* reg[Dest] <= operand
        cout << "ldr r" << destination_register_number << "=" << registers[destination_register_number];

        // if the update type is pre_ :
        update_before_address_fetch_source_register();

        short operand = fetch_operand_by_addressing_type();

        // execute instruction:
        registers[destination_register_number] = operand;

        // if the update type is post_ :
        update_after_address_fetch_source_register();

        cout << " #> " << registers[destination_register_number];
        // End of ldr instruction, everything passed well
        return true;
    } // end of case ldr
    case str:
    {
        //* operand <= reg[Dest]
        cout << "str r" << destination_register_number << "=" << registers[destination_register_number];
        // if the update type is pre_ :
        update_before_address_fetch_source_register();

        if (set_operand_by_addressing_type(registers[destination_register_number]) == false)
        {
            return false;
        }

        // if the update type is post_ :
        update_after_address_fetch_source_register();
        // End of ldr instruction, everything passed well
        return true;
    } // end of case str
    default:
    {
        {
            cout << "ERROR ";
            break;
        }
    }
    } // end of instruction switch
    return true;
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

bool EmulatorWrapper::get_flag(short flag)
{
    return rpsw & flag;
}

bool EmulatorWrapper::calculate_condition(short instruction)
{
    cout << " !condition (";
    switch (instruction)
    {
    case jeq:
    {
        cout << "jeq)? psw: " << rpsw;
        return get_flag(Z);
    }
    case jne:
    {
        cout << "jeq)? psw: " << rpsw;
        return !get_flag(Z);
    }
    case jgt:
    {
        cout << "jgt)? psw: " << rpsw;

        return !(get_flag(N) ^ get_flag(O)) & !get_flag(Z);
    }
    }
    return false;
}

void EmulatorWrapper::push_on_stack(short value)
{
    cout << " #Push " << value << " on stack: ";
    rsp -= 2;
    write_memory(value, rsp, WORD, true);
    cout << "# ";
}
short EmulatorWrapper::pop_from_stack()
{
    short value = read_memory(rsp, WORD, true);
    cout << " #Pop " << value << " from stack (sp=" << rsp << "->";
    rsp += 2;
    cout << rsp << "# ";

    return value;
}
