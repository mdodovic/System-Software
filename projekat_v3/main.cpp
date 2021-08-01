#include <iostream>
#include <fstream>
#include "AssemblyParser.h"
using namespace std;
#include <regex>
int main(int argc, const char *argv[])
{

    AssemblyParser as(argv[1]);
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