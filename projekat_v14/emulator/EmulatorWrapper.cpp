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
            cout << " Val=> " << (int)higher_value << " " << (int)lower_value;
            // This is combination of lower and higher Byte. Lower Byte is only 8b and higher byte is expanded (its sign is expanded to the size of an int)
            short value = (short)((higher_value << 8) + (0xff & lower_value));
            cout << " :" << value << "; ";
            return value;
        }
        else
        {
            // [H L] is the representation in memory
            char higher_value = memory[address];
            char lower_value = memory[address + 1];
            cout << " Val=> " << (int)higher_value << " " << (int)lower_value;
            // This is combination of lower and higher Byte. Lower Byte is only 8b and higher byte is expanded (its sign is expanded to the size of an int)
            short value = (short)((higher_value << 8) + (0xff & lower_value));
            cout << " :" << value << "; ";
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
        cout << " Val: " << (int)higher_value << " " << (int)lower_value << "! ";
        if (little_endian)
        {
            // [L H] is the representation in memory
            cout << " [0, 1] " << (int)memory[address] << ", " << (int)memory[address + 1] << " => ";
            memory[address] = lower_value;
            memory[address + 1] = higher_value;
            cout << (int)memory[address] << ", " << (int)memory[address + 1] << "; ";
        }
        else
        {
            // [H L] is the representation in memory
            cout << " [0, 1] " << (int)memory[address] << ", " << (int)memory[address + 1] << " => ";
            memory[address] = higher_value;
            memory[address + 1] = lower_value;
            cout << (int)memory[address] << ", " << (int)memory[address + 1] << "; ";
        }
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
        cout << endl;
    }

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
    case 0x1:
        return true;
    case 0x2:
        return true;
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
        return true;
    case 0x5:
        return true;
    case 0x6:
        return true;
    case 0x7:
        return true;
    case 0x8:
        return true;
    case 0x9:
        return true;
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
        operand = read_memory(registers[source_register_number], WORD, true) + instruction_payload;
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

        /*case  int_i
    case          iret
    */
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
    /*    case  ret
    case  jmp
    case  jeq
    case  jne
    case  jgt
    case  push
    case  pop
    case  xchg
    case  add
    case  sub
    case  mul
    case  div
    case  cmp
    case  not_i
    case  and_i
    case  or_i
    case  xor_i
    case  test
    case          shl
    case  shr
    */
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

        set_operand_by_address_type(registers[destination_register_number]);

        // if the update type is post_ :
        update_after_address_fetch_source_register();

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
    rsp += 2;
    return value;
}
