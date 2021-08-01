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
        //        as.print_errors();
        return -1;
    }

    as.print_symbol_table();
    as.print_relocation_table();
    as.print_section_table();
    as.print_section_data();
    //return 0;

    string sh_symbol1 = "[a-zA-Z][a-zA-Z0-9_]*"; // symbol can start only with letter and can contain letters, digits and _

    regex rx_global_directive1("^\\.global (" + sh_symbol1 + "(," + sh_symbol1 + ")*)$");
    string s = ".global label3,label4";
    smatch catch_parts;
    if (regex_search(s, catch_parts, rx_global_directive1))
    {
        string tmp;
        string symbols = catch_parts.str(1);
        stringstream ss(symbols);
        cout << "Global:" << endl;
        cout << "Find:#" << symbols << endl;
        while (getline(ss, tmp, ','))
        {
            cout << tmp << endl;
        }

        cout << "###" << endl;
    }
    return 0;
}