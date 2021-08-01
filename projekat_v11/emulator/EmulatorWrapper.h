#ifndef EMULATOR_H
#define EMULATOR_H
#include <string>
#include <vector>
#include <map>

using namespace std;

class EmulatorWrapper
{

    string input_file_name;
    vector<string> error_messages;

public:
    EmulatorWrapper(string);
    bool collect_data_from_relocatible_files();
    void print_error_messages();
};

#endif //EMULATOR_H