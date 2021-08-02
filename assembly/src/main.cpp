#include <iostream>
#include <fstream>
#include "AssemblyParser.h"
using namespace std;
#include <regex>
int main(int argc, const char *argv[])
{
    string input_file;
    string output_file;
    string current = argv[1];
    //    cout << argv[1] << "#-o" << endl;
    //    cout << argv[2] << endl;
    //    cout << argv[3] << endl;
    //    cout << strcmp(argv[1], "-o") << endl;
    if (current == "-o")
    {
        input_file = argv[3];
        output_file = argv[2];
    }
    else
    {
        cout << "Output file does not exists!" << endl;
        return -1;
    }

    AssemblyParser as(input_file, output_file);
    if (as.compile() == false)
    {
        as.print_error_messages();
        return -1;
    }

    as.print_symbol_table();
    as.print_relocation_table();
    as.print_section_table();
    as.print_section_data();

    return 0;
}