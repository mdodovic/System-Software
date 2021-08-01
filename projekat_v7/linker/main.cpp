#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <regex>
#include "LinkerWrapper.h"

using namespace std;

int main(int argc, const char *argv[])
{

    bool output_file = false;
    string output_file_name = "linker_out_generic.o";
    bool place_file = false;
    regex place_regex("^-place=([a-zA-Z_][a-zA-Z_0-9]*)@(0[xX][0-9a-fA-F]+)$");
    smatch section_address;
    map<string, int> mapped_section_address;
    bool hex_output = false;
    bool linkable_output = false;
    vector<string> files_to_be_linked;
    for (int i = 1; i < argc; i++)
    {
        string current = argv[i];
        cout << current << endl;
        if (current == "-o")
        {

            output_file = true;
            cout << "IN:" << output_file << endl;
        }
        else if (current == "-hex")
        {
            hex_output = true;
        }
        else if (current == "-linkable")
        {
            linkable_output = true;
        }
        else if (regex_search(current, section_address, place_regex))
        {
            place_file = true;
            string section = section_address.str(1);
            int address = stoi(section_address.str(2), nullptr, 16);
            mapped_section_address[section] = address;
        }
        else if (output_file)
        {
            cout << "OF:" << output_file << endl;
            output_file_name = current;
            output_file = false;
        }
        else
        {
            cout << "File for link:" << current << endl;
            files_to_be_linked.push_back(current);
        }
    }

    if (linkable_output == true && hex_output == true)
    {
        cout << "-linkable and -hex options are not allowed at the same time!" << endl;
        return -1;
    }
    if (linkable_output == false && hex_output == false)
    {
        cout << "One option -linkable or -hex has to be set!" << endl;

        return -1;
    }

    LinkerWrapper linker(output_file_name, files_to_be_linked, linkable_output, mapped_section_address);

    if (linker.collect_data_from_relocatible_files() == false)
    {
        linker.print_error_messages();
        return -1;
    }
    cout << endl
         << endl
         << endl
         << "OUTPUT:" << endl
         << endl;
    linker.print_section_table();
    linker.print_symbol_table();
    linker.print_relocation_table();
    linker.print_section_data();

    if (linker.create_aggregate_sections() == false)
    {
        linker.print_error_messages();
        return -1;
    }
    if (linker.create_aggregate_symbol_table() == false)
    {
        linker.print_error_messages();
        return -1;
    }

    linker.fill_output_file();

    // , place_file, mapped_section_address, hex_output, linkable_output different functions!
    return 0;
}