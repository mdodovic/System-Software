#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <regex>

#include "EmulatorWrapper.h"

using namespace std;

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        cout << "Only one file for execution needs to be passed to emulator" << endl;
        return -1;
    }
    cout << argc << ":" << argv[0] << "#" << argv[1] << endl;
    EmulatorWrapper emulator(argv[1]);

    if (emulator.collect_data_from_relocatible_files() == false)
    {
        emulator.print_error_messages();
        return -1;
    }
    if (emulator.load_data_to_memory() == false)
    {
        emulator.print_error_messages();
        return -1;
    }

    if (emulator.start_emulation() == false)
    {
        emulator.print_error_messages();
        return -1;
    }

    return 0;
}