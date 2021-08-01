#ifndef LINKER_H
#define LINKER_H
#include <string>
#include <vector>
#include <map>

using namespace std;

class LinkerWrapper
{
private:
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

        //*** This is offset from the beginning of the aggregate section
        int offset; // which byte is the start byte for relocation;

        string type;        // type of relocation
        string symbol_name; // which symbol is relocated ! (for local symbol it is the section where it is defined)
        int addend;         // unused, leaved here for eventually

        //This represents input file from which this relocation belongs to.
        string filename; // used only for local symbols, when the section name is written in relocation data;
    };
    struct SectionTable
    {
        int id_section;             // sequence number of section
        string section_name;        // name of section
        int size;                   // size of section
        vector<int> offsets;        // beginning of data -> location_counter
        vector<char> data;          // data at offset in vector offsets
        int virtual_memory_address; // address where this section is going to be loaded to memory
    };
    struct SectionAdditionalData
    {
        string section_name;
        string parent_file_name;
        int parent_section_size;
        int start_addres_in_aggregate_section;
    };

    // contains symbols relocations and sections per input file
    // key is filename
    map<string, map<string, SymbolTable>> symbol_table_per_file; // inner key is symbol name
    map<string, vector<RelocationTable>> relocation_table_per_file;
    map<string, map<string, SectionTable>> section_table_per_file; // inner key is section name

    map<string, SymbolTable> output_symbol_table;
    vector<RelocationTable> output_relocation_table;
    map<string, SectionTable> output_section_table;

    string output_file_name;
    vector<string> files_to_be_linked;

    vector<string> error_messages;

    map<string, map<string, SectionAdditionalData>> section_additional_helper;
    // helper info for linker to know where are the sections from input files
    // first key is section name, and second key is the input file name

    bool linkable_output;
    map<string, int> mapped_section_address;

    bool move_sections_to_virtual_address();
    int fetch_start_address_of_section(string);
    bool create_aggregate_relocations_linkable();
    bool create_aggregate_relocations_hex();
    bool create_aggregate_content_of_sections_linkable();
    bool create_aggregate_content_of_sections_hex();
    bool check_places_of_sections();

public:
    LinkerWrapper(string, vector<string>, bool, map<string, int>);

    bool collect_data_from_relocatible_files();
    bool create_aggregate_sections();
    bool create_aggregate_symbol_table();
    bool create_aggregate_relocations();
    bool create_aggregate_content_of_sections();
    bool solve_relocations_on_data();

    void print_error_messages();

    void print_symbol_table();
    void print_relocation_table();
    void print_section_table();
    void print_section_data();

    void fill_output_file();
};
#endif //LINKER_H