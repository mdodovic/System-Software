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
    string path_to_output_file;

    vector<string> input_file_lines;
    int current_line_number;
    map<int, string> error_messages;
    // lines from input files are changed (spaces, new lines, comments etc. have been removed),
    // hence there are differences between actual file line numbers and ones that are processed.
    map<int, int> transfer_error_lines;

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
        bool is_data;        // is this relocation of data or of instruction
        string section_name; // where to relocate, current section!
        int offset;          // which byte is the start byte for relocation
        string type;         // type of relocation
        string symbol_name;  // which symbol is relocated ! (for local symbol it is the section where it is defined)
        int addend;          // unused, leaved here for eventually
    };
    /*struct SectionData
    {
        int offset;                         // offset: location counter from second pass
        vector<char> binary_representation; // data for the output file byte-per-byte
    };*/
    struct SectionTable
    {
        int id_section;      // sequence number of section
        string section_name; // name of section
        int size;            // size of section
        vector<int> offsets; // beginning of data -> location_counter
        vector<char> data;   // data at offset in vector offsets
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
    bool process_equ_symbol(string, string);
    bool process_skip_first_pass(string);
    bool process_skip_second_pass(string);
    bool process_word_first_pass(string);
    bool process_word_second_pass(string);

    bool process_instruction_first_pass(string);
    bool process_instruction_second_pass(string);

    int process_absolute_addressing_symbol(string);
    int process_pc_relative_addressing_symbol(string);

    int fetch_decimal_value_from_literal(string);
    string get_hexadecimal_value_from_decimal(int);

    bool clear_input_file();
    bool first_assembly_pass();
    bool second_assembly_pass();

    void create_txt_file();
    void create_binary_file();

public:
    AssemblyParser(string, string);
    bool compile();
    void print_symbol_table();
    void print_relocation_table();
    void print_section_table();
    void print_section_data();
    void print_error_messages();
};
#endif //ASSEMBLY_H
