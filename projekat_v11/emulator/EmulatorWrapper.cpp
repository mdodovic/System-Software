#include <iostream>
#include <fstream>
#include <iomanip>

#include "EmulatorWrapper.h"

using namespace std;

EmulatorWrapper::EmulatorWrapper(string input_file_name_)
    : input_file_name(input_file_name_)
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
    /*    cout << "Section table:" << endl;
        cout << "Id\tName\t\tSize" << endl;
        for (int i = 0; i < number_of_sections; i++)
        {
            struct SectionTable sec_table;
            string sec_name;
            unsigned int stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            sec_name.resize(stringLength);
            input_file.read((char *)sec_name.c_str(), stringLength);
            cout << sec_name << endl;

            input_file.read((char *)(&sec_table.id_section), sizeof(sec_table.id_section));
            input_file.read((char *)(&sec_table.size), sizeof(sec_table.size));

            sec_table.virtual_memory_address = 0;

            stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            sec_table.section_name.resize(stringLength);
            input_file.read((char *)sec_table.section_name.c_str(), stringLength);

            int number_of_chars;
            input_file.read((char *)(&number_of_chars), sizeof(number_of_chars));
            cout << dec << number_of_chars << hex << endl;

            for (int j = 0; j < number_of_chars; j++)
            {
                char c;
                input_file.read((char *)(&c), sizeof(c));
                sec_table.data.push_back(c);
            }
            cout << dec << sec_table.data.size() << hex << endl;
            cout << "Data:" << endl;

            int counter = 0;
            for (char c : sec_table.data)
            {
                if (counter % 16 == 0)
                {
                    cout << hex << setfill('0') << setw(4) << (0xffff & counter) << "   ";
                }
                cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
                counter++;
                if (counter % 16 == 0)
                {
                    cout << endl;
                }
            }
            cout << dec;
            cout << endl;

            int number_of_offsets;
            input_file.read((char *)(&number_of_offsets), sizeof(number_of_offsets));
            cout << dec << number_of_offsets << hex << endl;

            for (int j = 0; j < number_of_offsets; j++)
            {
                int x;
                input_file.read((char *)(&x), sizeof(x));
                sec_table.offsets.push_back(x);
            }
            cout << dec << sec_table.offsets.size() << endl;
            for (int c : sec_table.offsets)
            {
                cout << c << " ";
            }
            cout << endl;
            cout << sec_table.id_section << "\t" << sec_table.section_name << "\t" << hex << setfill('0') << setw(4) << (0xffff & sec_table.size) << endl;
            section_table_map[sec_name] = sec_table;
        }
*/
    input_file.close();
    return true;
}
void EmulatorWrapper::print_error_messages()
{

    for (string err : error_messages)
    {
        cout << "Emulator error: " << err << endl;
    }
}