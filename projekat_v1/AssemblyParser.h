#ifndef ASSEMBLY_H
#define ASSEMBLY_H
#include <string>
#include <vector>
#include <map>
using namespace std;

class AssemblyParser
{
private:
    string path_to_file;
    vector<string> input_file_lines;
    int current_line_number;
    map<int, string> error_messages;
    bool error_happened;

    static int id_symbol_in_symbol_table;
    static int id_section_in_section_table;

    struct SymbolTable
    {
        int id_symbol;   // sequence number of symbol
        int value;       // this is offset within section
        bool is_local;   // is symbol local or global
        bool is_defined; // is symbol defined (if it is local it has to be)
        bool is_extern;  // is symbol extern (then it is undefined)
        string section;  // section where this symbol is defined
        string name;     // symbol name
    };
    struct RelocationTable
    {
        string section_name;
        int offset;
        string type;
        int number_in_symbol_table;
        int addend; // unused
    };
    struct SectionData
    {
        int offset;                         // offset: location counter from second pass
        vector<char> binary_representation; // data for the output file byte-per-byte
    };
    struct SectionTable
    {
        int id_section;           // sequence number of section
        string section_name;      // name of section
        int size;                 // size of section
        vector<SectionData> data; // Encapsulated data (offset, data)
    };

    int location_counter;
    string current_section;

    map<string, SymbolTable> symbol_table;
    vector<RelocationTable> relocation_table;
    map<string, SectionTable> section_table;

    bool process_label(string);
    bool process_section(string);
    bool process_global_symbol(string);
    bool process_extern_symbol(string);

    bool clear_input_file();
    bool first_assembly_pass();

public:
    AssemblyParser(string);
    bool compile();
    void print_symbol_table();
    void print_relocation_table();
    void print_section_table();
    void print_section_data();
};
#endif //ASSEMBLY_H
