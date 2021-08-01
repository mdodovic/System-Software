#include <iostream>
#include <fstream>
#include <iomanip>

#include "EmulatorWrapper.h"
#include "EmulatorConstants.h"
using namespace std;

int EmulatorWrapper::MEMORY_SIZE = 1 << 16;
int EmulatorWrapper::MEMORY_MAPPED_REGISTERS = 0xFF00;
int EmulatorWrapper::REGISTER_NUMBER = 9;

EmulatorWrapper::EmulatorWrapper(string input_file_name_)
    : input_file_name(input_file_name_), memory(MEMORY_SIZE, 0), registers(REGISTER_NUMBER, 0)
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

bool EmulatorWrapper::start_emulation()
{
    cout << registers[7] << endl;

    registers[7] = 3;
    cout << registers[pc] << endl;
    registers[pc] = 9;
    cout << registers[7] << endl;
    rpc = 15;
    cout << rpc << endl;
    registers[7] = 1;
    cout << rpc << endl;
    rpc = 3;
    cout << rpc << endl;

    return true;
}