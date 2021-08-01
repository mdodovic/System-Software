#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <regex>
#include <iterator>

#include "AssemblyParser.h"
#include "AssemblyRegexes.h"
using namespace std;

int AssemblyParser::id_symbol_in_symbol_table = 0;
int AssemblyParser::id_section_in_section_table = 0;

AssemblyParser::AssemblyParser(string path, string path_to_output_text_file)
    : path_to_file(path), path_to_output_file(path_to_output_text_file), location_counter(0), current_section("")
{
    // symbol UNDEFINED represents section with id=0
    // it is used to mark all undefined
    SectionTable undefined_section;
    undefined_section.id_section = id_section_in_section_table++;
    undefined_section.section_name = "UNDEFINED";
    undefined_section.size = 0;
    //    undefined_section.data;
    section_table["UNDEFINED"] = undefined_section;
    // UNDEFINED is symbol so it has to be in symbol table
    SymbolTable undefined_section_symbol;
    undefined_section_symbol.id_symbol = id_symbol_in_symbol_table++;
    undefined_section_symbol.is_defined = true;
    undefined_section_symbol.is_extern = false;
    undefined_section_symbol.is_local = true;
    undefined_section_symbol.name = "UNDEFINED";
    undefined_section_symbol.section = "UNDEFINED";
    undefined_section_symbol.value = 0;
    symbol_table["UNDEFINED"] = undefined_section_symbol;

    // section ABSOLUTE represents section with symbols defined by literal
    // represents constatns!
    SectionTable absolute_section;
    absolute_section.id_section = -1;
    absolute_section.section_name = "ABSOLUTE";
    absolute_section.size = 0;
    //    undefined_section.data;
    section_table["ABSOLUTE"] = absolute_section;
    // ABSOLUTE is symbol so it has to be in symbol table
    SymbolTable absolute_section_symbol;
    absolute_section_symbol.id_symbol = id_symbol_in_symbol_table++;
    absolute_section_symbol.is_defined = true;
    absolute_section_symbol.is_extern = false;
    absolute_section_symbol.is_local = true;
    absolute_section_symbol.name = "ABSOLUTE";
    absolute_section_symbol.section = "ABSOLUTE";
    absolute_section_symbol.value = 0;
    symbol_table["ABSOLUTE"] = absolute_section_symbol;

    // section ABSOLUTE represents section with symbols defined by literal
}

void AssemblyParser::create_txt_file()
{

    ofstream text_output_file(path_to_output_file);
    text_output_file << "Relocative object file" << endl
                     << endl;

    text_output_file << "Section table:" << endl;
    text_output_file << "Id\tName\t\tSize" << endl;
    for (map<string, SectionTable>::iterator it = section_table.begin(); it != section_table.end(); it++)
    {
        text_output_file << it->second.id_section << "\t" << it->second.section_name << "\t" << hex << setfill('0') << setw(4) << (0xffff & it->second.size) << endl;
    }
    text_output_file << dec;
    text_output_file << endl
                     << endl;

    text_output_file << "Symbol table:" << endl;
    text_output_file << "Value\tType\tSection\t\tName\t\tId" << endl;
    for (map<string, SymbolTable>::iterator it = symbol_table.begin(); it != symbol_table.end(); it++)
    {
        text_output_file << hex << setfill('0') << setw(4) << (0xffff & it->second.value) << "\t";
        // extern symbols?
        if (it->second.is_local == true)
            text_output_file << "l\t";
        else
        {
            if (it->second.is_defined == true)
                text_output_file << "g\t";
            else
            {
                if (it->second.is_extern)
                    text_output_file << "e\t";
                else
                    text_output_file << "u\t"; // undefined
            }
        }
        text_output_file << it->second.section << "\t" << it->second.name << "\t" << hex << setfill('0') << setw(4) << (0xffff & it->second.id_symbol) << endl;
    }
    text_output_file << dec;
    text_output_file << endl
                     << endl;
    for (map<string, SectionTable>::iterator it = section_table.begin(); it != section_table.end(); it++)
    {
        //        if (it->first == "UNDEFINED")
        //            continue;

        text_output_file << "Relocation data <" << it->first << ">:" << endl;
        text_output_file << "Offset\tType\t\tDat/Ins\tSymbol\tSection name" << endl;
        for (RelocationTable rel_data : relocation_table)
        {
            if (rel_data.section_name != it->first)
                continue;
            text_output_file << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset) << "\t" << rel_data.type << "\t" << (rel_data.is_data ? 'd' : 'i') << "\t" << rel_data.symbol_name << "\t" << rel_data.section_name << endl;
        }
        text_output_file << dec << endl;

        text_output_file << "Section data <" << it->first << ">:" << endl;

        SectionTable s_table = it->second;
        int counter = 0;

        for (int i = 0; i < s_table.data.size(); i++)
        {
            char c = s_table.data[i];
            if (counter % 8 == 0)
            {
                text_output_file << hex << setfill('0') << setw(4) << (0xffff & counter) << "   ";
            }
            text_output_file << hex << setfill('0') << setw(2) << (0xff & c) << " ";
            counter++;
            if (counter % 8 == 0)
            {
                text_output_file << endl;
            }
        }
        text_output_file << dec;
        text_output_file << endl
                         << endl;
    }
    text_output_file.close();
}
void AssemblyParser::create_binary_file()
{
    string binary_filename_output = "./linker_input_" + path_to_output_file;
    ofstream binary_output_file(binary_filename_output, ios::out | ios::binary);
    cout << "Create binary file:" << endl;

    // Relocation data
    int number_of_relocations = relocation_table.size();
    cout << number_of_relocations << endl;

    binary_output_file.write((char *)&number_of_relocations, sizeof(number_of_relocations));

    for (RelocationTable rel_data : relocation_table)
    {
        cout << sizeof(rel_data);

        binary_output_file.write((char *)(&rel_data.addend), sizeof(rel_data.addend));
        binary_output_file.write((char *)(&rel_data.is_data), sizeof(rel_data.is_data));
        binary_output_file.write((char *)(&rel_data.offset), sizeof(rel_data.offset));

        unsigned int stringLength = rel_data.section_name.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(rel_data.section_name.c_str(), rel_data.section_name.length());

        stringLength = rel_data.symbol_name.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(rel_data.symbol_name.c_str(), rel_data.symbol_name.length());

        stringLength = rel_data.type.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(rel_data.type.c_str(), rel_data.type.length());
    }

    // Symbol table
    int number_of_symbols = symbol_table.size();
    binary_output_file.write((char *)&number_of_symbols, sizeof(number_of_symbols));
    cout << number_of_symbols << endl;
    for (map<string, SymbolTable>::iterator it = symbol_table.begin(); it != symbol_table.end(); it++)
    {
        string key = it->first;
        // key for map
        unsigned int stringLength = key.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(key.c_str(), key.length());
        // values:
        binary_output_file.write((char *)(&it->second.id_symbol), sizeof(it->second.id_symbol));
        binary_output_file.write((char *)(&it->second.is_defined), sizeof(it->second.is_defined));
        binary_output_file.write((char *)(&it->second.is_extern), sizeof(it->second.is_extern));
        binary_output_file.write((char *)(&it->second.is_local), sizeof(it->second.is_local));
        binary_output_file.write((char *)(&it->second.value), sizeof(it->second.value));

        stringLength = it->second.name.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(it->second.name.c_str(), it->second.name.length());

        stringLength = it->second.section.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(it->second.section.c_str(), it->second.section.length());
    }

    // Section table:
    int number_of_sections = section_table.size();
    binary_output_file.write((char *)&number_of_sections, sizeof(number_of_sections));
    cout << number_of_sections << endl;

    for (map<string, SectionTable>::iterator it = section_table.begin(); it != section_table.end(); it++)
    {
        string key = it->first;
        // key for map
        unsigned int stringLength = key.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(key.c_str(), key.length());

        binary_output_file.write((char *)(&it->second.id_section), sizeof(it->second.id_section));
        binary_output_file.write((char *)(&it->second.size), sizeof(it->second.size));

        stringLength = it->second.section_name.length();
        binary_output_file.write((char *)(&stringLength), sizeof(stringLength));
        binary_output_file.write(it->second.section_name.c_str(), it->second.section_name.length());

        // section data
        int number_of_chars = it->second.data.size();
        binary_output_file.write((char *)&number_of_chars, sizeof(number_of_chars));
        cout << number_of_chars << " - " << it->second.data.size() << " => ";

        int x = 0;
        for (char c : it->second.data)
        {
            x++;
            binary_output_file.write((char *)&c, sizeof(c));
        }
        cout << x << endl;

        int number_of_offsets = it->second.offsets.size();
        binary_output_file.write((char *)&number_of_offsets, sizeof(number_of_offsets));
        cout << number_of_offsets << " - " << it->second.offsets.size() << endl;

        x = 0;
        for (int o : it->second.offsets)
        {
            x++;
            binary_output_file.write((char *)&o, sizeof(o));
            cout << " " << o;
        }
        cout << " ->" << x << endl;
    }
    binary_output_file.close();
}

bool AssemblyParser::compile()
{
    if (clear_input_file() == false)
    {
        cout << "File " + path_to_file + " cannot be opened!" << endl;
        return false;
    }
    if (first_assembly_pass() == false)
        return false;
    if (second_assembly_pass() == false)
        return false;
    create_txt_file();
    create_binary_file();
    return true;
}

bool AssemblyParser::clear_input_file()
{
    ifstream input_file;
    input_file.open(path_to_file);
    if (input_file.is_open() == false)
        return false;
    string line;
    int line_before_processing = 0;
    int line_after_processing = 0;
    while (getline(input_file, line))
    {
        line_before_processing++;

        //cout << "###" << endl << line << endl;
        string cleared_line;
        cleared_line = regex_replace(line, rx_remove_comments, "$1", regex_constants::format_first_only);
        cleared_line = regex_replace(cleared_line, rx_find_extra_spaces, " ");
        cleared_line = regex_replace(cleared_line, rx_find_tabs, " ");
        cleared_line = regex_replace(cleared_line, rx_remove_boundary_spaces, "$2");
        cleared_line = regex_replace(cleared_line, rx_find_comma_spaces, ",");
        cleared_line = regex_replace(cleared_line, rx_find_columns_spaces, ":");
        //cout << cleared_line << endl;
        if (cleared_line == "" || cleared_line == " ")
            continue;
        line_after_processing++;
        transfer_error_lines[line_after_processing] = line_before_processing;
        input_file_lines.push_back(cleared_line);
    }
    input_file.close();
    return true;
}

bool AssemblyParser::process_label(string label_name)
{
    // check if it is in section?
    if (current_section == "")
    {
        error_messages[current_line_number] = "Label has to be defined in section!";
        return false;
    }
    // check if it is already defined?
    map<string, SymbolTable>::iterator sym = symbol_table.find(label_name);

    if (sym != symbol_table.end())
    {
        // symbol exists in the symbol table
        if (sym->second.is_defined == true)
        {
            error_messages[current_line_number] = "Symbol is already defined in this module!";
            return false;
        }
        if (sym->second.is_extern == true)
        {
            error_messages[current_line_number] = "Symbol is already defined in another module!";
            return false;
        }
        // and it was undefined and unextern
        // so set definition, value and section where it is defined

        sym->second.is_defined = true;
        sym->second.value = location_counter;
        sym->second.section = current_section;
    }
    else
    {
        // add new symbol
        SymbolTable new_symbol;
        new_symbol.id_symbol = id_symbol_in_symbol_table++;
        new_symbol.is_defined = true;
        new_symbol.is_extern = false;
        new_symbol.is_local = true;
        new_symbol.name = label_name;
        new_symbol.section = current_section;
        new_symbol.value = location_counter;
        symbol_table[label_name] = new_symbol;
    }
    return true;
}
bool AssemblyParser::process_section(string section_name)
{

    // --- multiple definition of section?
    // --- iterate through symbol names!

    bool previous_section_exists = (current_section != "");

    if (previous_section_exists == true)
    {
        cout << "End section: " << section_table[current_section].size << "-" << location_counter << endl;
        // size of section is authomatically added dealing with other directives that changes its size
    }
    // end previous section
    location_counter = 0;
    current_section = section_name;

    SymbolTable new_section_symbol;
    new_section_symbol.id_symbol = id_symbol_in_symbol_table++;
    new_section_symbol.is_defined = true;
    new_section_symbol.is_extern = false;
    new_section_symbol.is_local = true;
    new_section_symbol.name = section_name;
    new_section_symbol.section = current_section;
    new_section_symbol.value = location_counter;
    symbol_table[section_name] = new_section_symbol;

    SectionTable new_section;
    new_section.id_section = id_section_in_section_table++;
    new_section.section_name = section_name;
    new_section.size = 0;
    //    new_section.data;
    section_table[section_name] = new_section;
    return true;
}

bool AssemblyParser::process_global_symbol(string global_symbol)
{
    // fetch symbol if it is defined already?
    map<string, SymbolTable>::iterator sym = symbol_table.find(global_symbol);
    if (sym != symbol_table.end())
    {
        // if symbol exists it becomes global
        // id, name, section, value
        sym->second.is_local = false;
        // it can be global and also both defined (is_defined = true) or nondefined (is_extern = true)
    }
    else
    {
        SymbolTable new_symbol;
        new_symbol.id_symbol = id_symbol_in_symbol_table++;
        new_symbol.is_defined = false;
        new_symbol.is_extern = false;
        new_symbol.is_local = false;
        new_symbol.name = global_symbol;
        new_symbol.section = "UNDEFINED";
        new_symbol.value = 0;
        symbol_table[global_symbol] = new_symbol;
    }
    return true;
}
bool AssemblyParser::process_extern_symbol(string external_symbol)
{
    map<string, SymbolTable>::iterator sym = symbol_table.find(external_symbol);
    if (sym != symbol_table.end())
    {
        if (sym->second.is_defined == true)
        {
            error_messages[current_line_number] = "External redefinition of defined symbol";
            return false;
        }
    }
    else
    {
        SymbolTable new_symbol;
        new_symbol.id_symbol = id_symbol_in_symbol_table++;
        new_symbol.is_defined = false;
        new_symbol.is_extern = true;
        new_symbol.is_local = false;
        new_symbol.name = external_symbol;
        new_symbol.section = "UNDEFINED";
        new_symbol.value = 0;
        symbol_table[external_symbol] = new_symbol;
    }
    return true;
}

int AssemblyParser::fetch_decimal_value_from_literal(string literal)
{
    smatch number;
    int int_number;
    //cout << "str:" << n << endl;
    if (regex_search(literal, number, rx_literal_hexadecimal))
    {
        //        cout << "0x#" << number.str(1).substr(2);
        stringstream ss;
        ss << number.str(1).substr(2); // std::string hex_value
        ss >> hex >> int_number;       //int decimal_value
    }
    else
    {
        regex_search(literal, number, rx_literal_decimal);
        int_number = stoi(number.str(1));
        /* convert to hexadecimal
        cout << "int:" << int_number << endl;
        stringstream ss;
        ss << hex << int_number; // int decimal_value
        string res(ss.str());

        cout << res;
        //cout << "hex:" << hex << int_number << endl;
        */
    }
    return int_number;
}
string AssemblyParser::get_hexadecimal_value_from_decimal(int decimal_value)
{
    stringstream ss;
    ss << hex << decimal_value; // int decimal_value
    string res(ss.str());

    return res;
}

bool AssemblyParser::process_equ_symbol(string symbol_name, string value)
{
    // test detail
    map<string, SymbolTable>::iterator sym = symbol_table.find(symbol_name);
    if (sym != symbol_table.end())
    {
        // symbol exists in symbol table
        cout << "Error?:" << endl;
        cout << sym->second.name << ":" << sym->second.value << "(" << sym->second.section << ")" << endl;
        cout << "is defined:" << sym->second.is_defined << endl;
        cout << "is extern:" << sym->second.is_extern << endl;
        cout << "is local:" << sym->second.is_local << endl;
        if (sym->second.is_extern == true)
        {
            // Error, equ defines absolute symbol
            error_messages[current_line_number] = "equ directive cannot define extern symbol";
            return false;
        }
        if (sym->second.is_defined == true)
        {
            error_messages[current_line_number] = "equ directive cannot define an absolute symbol that is already defined";
            return false;
        }
        // undefined and nonextern:
        // local or global does not metter because it belongs to ABSOLUTE section!
        sym->second.is_defined = true;
        sym->second.section = "ABSOLUTE";
        sym->second.value = fetch_decimal_value_from_literal(value);
        section_table["ABSOLUTE"].offsets.push_back(section_table["ABSOLUTE"].size);
        section_table["ABSOLUTE"].size += 2;
        section_table["ABSOLUTE"].data.push_back(0xff & fetch_decimal_value_from_literal(value));
        section_table["ABSOLUTE"].data.push_back(0xff & (fetch_decimal_value_from_literal(value) >> 8));
        return true;
    }
    else
    {
        // symbol do no exists, or it is not mentioned earlier
        SymbolTable new_equ_symbol;
        new_equ_symbol.id_symbol = id_symbol_in_symbol_table++;
        new_equ_symbol.is_defined = true;
        new_equ_symbol.is_extern = false;
        new_equ_symbol.is_local = true;
        new_equ_symbol.name = symbol_name;
        new_equ_symbol.section = "ABSOLUTE"; // it is not a section like others, just notification that this symbol has
        new_equ_symbol.value = fetch_decimal_value_from_literal(value);
        symbol_table[symbol_name] = new_equ_symbol;

        section_table["ABSOLUTE"].offsets.push_back(section_table["ABSOLUTE"].size);
        section_table["ABSOLUTE"].size += 2;
        section_table["ABSOLUTE"].data.push_back(0xff & fetch_decimal_value_from_literal(value));
        section_table["ABSOLUTE"].data.push_back(0xff & (fetch_decimal_value_from_literal(value) >> 8));
        return true;
    }
}
bool AssemblyParser::process_skip_first_pass(string value)
{
    // check if it is in section?
    if (current_section == "")
    {
        error_messages[current_line_number] = "skip has to be in section!";
        return false;
    }

    cout << "Want numeric value?!";

    int skip_value = fetch_decimal_value_from_literal(value);
    cout << skip_value << endl;

    location_counter += skip_value;
    section_table[current_section].size += skip_value;

    return true;
}

bool AssemblyParser::process_skip_second_pass(string value)
{

    int skip_value = fetch_decimal_value_from_literal(value);
    // fill data with zeros
    section_table[current_section].offsets.push_back(location_counter);
    for (int i = 0; i < skip_value; i++)
        section_table[current_section].data.push_back(0);

    for (char c : section_table[current_section].data)
    {
        cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
    }
    cout << dec;

    location_counter += skip_value;
    return true;
}

bool AssemblyParser::process_word_first_pass(string word)
{
    if (current_section == "")
    {
        error_messages[current_line_number] = "word directive has to be in a section!";
        return false;
    }

    // first pass: only collect info about size:
    location_counter += 2;
    section_table[current_section].size += 2;
    return true;
}

bool AssemblyParser::process_word_second_pass(string word_argument)
{
    // second pass: if word_argument is a number, than allocate space with this value
    // if word_argument is a label, than it is already in symbol_table
    // if it is not, raise error!
    if (regex_match(word_argument, rx_symbol))
    {
        cout << "SECOND WORD PASS: " << word_argument << "->";
        map<string, SymbolTable>::iterator sym = symbol_table.find(word_argument);
        if (sym != symbol_table.end())
        {
            SymbolTable s_table = sym->second;
            // symbol exists in symbol table
            cout << s_table.name << endl;
            if (s_table.section == "ABSOLUTE")
            {
                cout << "EQU CALCULATIONS: " << s_table.value << endl;
                section_table[current_section].offsets.push_back(location_counter);
                section_table[current_section].data.push_back(s_table.value & 0xff);
                section_table[current_section].data.push_back((s_table.value >> 8) & 0xff);
            }
            else
            {
                cout << s_table.value << endl;
                if (s_table.is_defined == true)
                {
                    if (s_table.is_local == true)
                    {
                        // add value of symbol to data
                        section_table[current_section].offsets.push_back(location_counter);
                        section_table[current_section].data.push_back(s_table.value & 0xff);
                        section_table[current_section].data.push_back((s_table.value >> 8) & 0xff);
                        // create relocation data for section
                        RelocationTable rel_data;
                        rel_data.is_data = true;
                        rel_data.type = "R_HYP_16";
                        rel_data.section_name = current_section;
                        rel_data.offset = location_counter;
                        rel_data.symbol_name = s_table.section; // this is local symbol, so only section is necessary !
                        rel_data.addend = 0;
                        relocation_table.push_back(rel_data);
                    }
                    else
                    {
                        // global symbol
                        // add zeros to data
                        section_table[current_section].offsets.push_back(location_counter);
                        section_table[current_section].data.push_back(0 & 0xff);
                        section_table[current_section].data.push_back((0 >> 8) & 0xff);
                        // create relocation data for this symbol
                        RelocationTable rel_data;
                        rel_data.is_data = true;
                        rel_data.type = "R_HYP_16";
                        rel_data.section_name = current_section;
                        rel_data.offset = location_counter;
                        rel_data.symbol_name = s_table.name;
                        rel_data.addend = 0;
                        relocation_table.push_back(rel_data);
                    }
                }
                else
                {
                    // symbol is not defined!
                    // add zeros to data
                    section_table[current_section].offsets.push_back(location_counter);
                    section_table[current_section].data.push_back(0 & 0xff);
                    section_table[current_section].data.push_back((0 >> 8) & 0xff);
                    // create relocation data for this symbol
                    RelocationTable rel_data;
                    rel_data.is_data = true;
                    rel_data.type = "R_HYP_16";
                    rel_data.section_name = current_section;
                    rel_data.offset = location_counter;
                    rel_data.symbol_name = s_table.name;
                    rel_data.addend = 0;
                    relocation_table.push_back(rel_data);
                }
            }
        }
        else
        {
            // Symbol do not exists in symbol table
            error_messages[current_line_number] = ".word used with undefined symbol!";
            return false;
        }
    }
    else
    {
        string s = get_hexadecimal_value_from_decimal(fetch_decimal_value_from_literal(word_argument));
        cout << "SECOND WORD PASS: "
             << fetch_decimal_value_from_literal(word_argument)
             << "->"
             << s
             << endl;
        char c = fetch_decimal_value_from_literal(word_argument);
        section_table[current_section].offsets.push_back(location_counter);
        section_table[current_section].data.push_back(0xff & c);
        section_table[current_section].data.push_back(0xff & (c >> 8));

        cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
        cout << hex << setfill('0') << setw(2) << (0xff & (c >> 8)) << " ";
        cout << dec;
    }
    location_counter += 2;
    return true;
}

int AssemblyParser::process_absolute_addressing_symbol(string symbol)
{
    cout << "Process absolute addressing#" << symbol << endl;
    map<string, SymbolTable>::iterator sym = symbol_table.find(symbol);
    if (sym != symbol_table.end())
    {
        SymbolTable s_from_table = sym->second;
        // symbol exists in symbol table
        cout << s_from_table.name << endl;
        if (s_from_table.section == "ABSOLUTE")
        {
            cout << "EQU CALCULATIONS: " << s_from_table.value << endl;
            return s_from_table.value;
        }
        else
        {
            // it is in regular section:
            cout << "Value: " << s_from_table.value << endl;
            cout << "Section: " << s_from_table.section << endl;
            cout << "Name: " << s_from_table.name << endl;
            cout << "Local: " << s_from_table.is_local << endl;
            cout << "Extern: " << s_from_table.is_extern << endl;
            cout << "Defined: " << s_from_table.is_defined << endl;
            cout << "Symbol: " << s_from_table.id_symbol << endl;

            // Relocation data has to be written
            RelocationTable rel_data;
            rel_data.addend = 0;
            rel_data.is_data = false;                // this is instruction, so use big endian when reloc
            rel_data.offset = location_counter + 4;  // +0 - opcode, +1 - registers, +2 - ua flags, +3 - higher bits, +4 - lower bits
            rel_data.section_name = current_section; // section for which this relocation data is about
            rel_data.type = "R_HYP_16";

            int written_symbol_value;
            // only the difference is the name of what it is.
            // if it is global/extern symbol, this very name should be in relocation table
            // but in memory, there should be zeros
            if ((s_from_table.is_local == false) || (s_from_table.is_extern == true))
            {
                rel_data.symbol_name = s_from_table.name;
                written_symbol_value = 0;
            } // else in memory should be the real value, but in relocation table there is section (symbol's, not current)
            else
            {
                rel_data.symbol_name = s_from_table.section;
                written_symbol_value = s_from_table.value;
            }
            relocation_table.push_back(rel_data); // add relocation data to relocation table
            return written_symbol_value;
        }
    }
    else
    {
        // Symbol do not exists in symbol table
        error_messages[current_line_number] = "Symbol is not in symbol table";
        return false; // THIS IS WRONG! use error_happened
    }

    return 0;
}

int AssemblyParser::process_pc_relative_addressing_symbol(string symbol)
{
    cout << "Process pc relative addressing#" << symbol << endl;
    map<string, SymbolTable>::iterator sym = symbol_table.find(symbol);
    if (sym != symbol_table.end())
    {
        SymbolTable s_from_table = sym->second;
        // symbol exists in symbol table
        cout << s_from_table.name << endl;
        int addend = -2;
        if (s_from_table.section == "ABSOLUTE")
        {
            cout << "EQU CALCULATIONS:#" << s_from_table.value << endl;
            // ? TO BE SEEN
            return addend + s_from_table.value;
        }
        else
        {
            // it is in regular section:
            cout << "Value: " << s_from_table.value << endl;
            cout << "Section: " << s_from_table.section << endl;
            cout << "Name: " << s_from_table.name << endl;
            cout << "Local: " << s_from_table.is_local << endl;
            cout << "Extern: " << s_from_table.is_extern << endl;
            cout << "Defined: " << s_from_table.is_defined << endl;
            cout << "Symbol: " << s_from_table.id_symbol << endl;
            // Relocation data has to be written
            RelocationTable rel_data;
            rel_data.addend = 0;
            rel_data.is_data = false;                // this is instruction, so use big endian when reloc
            rel_data.offset = location_counter + 4;  // ?? +0 - opcode, +1 - registers, +2 - ua flags, +3 - higher bits, +4 - lower bits
            rel_data.section_name = current_section; // section for which this relocation data is about
            rel_data.type = "R_HYP_16_PC";
            int written_symbol_value;
            // only the difference is the name of what it is.
            // if it is global/extern symbol, this very name should be in relocation table
            // but in memory, there should be zeros
            if ((s_from_table.is_local == false) || (s_from_table.is_extern == true))
            {
                rel_data.symbol_name = s_from_table.name;
                written_symbol_value = addend; // -2 is only in memory, value of symbol is left to linker
            }
            else
            { // else in memory should be the real value, but in relocation table there is section (symbol's, not current)
                if (current_section == s_from_table.section)
                {
                    // same section and pc relative address: difference between symbol and jmp instruction is always the same
                    written_symbol_value = s_from_table.value + addend - (location_counter + 3);
                    cout << "Ultimat local: s=" << s_from_table.value << ",l+5=" << location_counter + 5;
                    cout << "=" << written_symbol_value << endl;
                    return written_symbol_value;
                }
                else
                {
                    //different section
                    rel_data.symbol_name = s_from_table.section;
                    written_symbol_value = s_from_table.value + addend; // only section has to be relocated
                }
            }
            relocation_table.push_back(rel_data); // add relocation data to relocation table
            return written_symbol_value;
        }
    }
    else
    {
        // Symbol do not exists in symbol table
        error_messages[current_line_number] = "Symbol is not in symbol table";

        return false; // THIS IS WRONG! use error_happened
    }
    return 0;
}

bool AssemblyParser::process_instruction_first_pass(string line_with_instruction)
{
    cout << "FIRST PASS INSTRUCTION:" << endl;
    if (current_section == "")
    {
        error_messages[current_line_number] = "Instruction has to be in a section!";
        return false;
    }
    smatch catch_parts;
    if (regex_search(line_with_instruction, catch_parts, rx_no_operand_instruction))
    {
        // These are halt, iret, ret:
        // increment location_counter for 1 (1B is instruction size)
        cout << "One byte: " << catch_parts.str(1);
        location_counter += 1;
        section_table[current_section].size += 1;
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_one_operand_register_instruction))
    {
        // These are push r, pop r, int r, not r:
        // increment location_counter for 2 or 3 (2B is int and not instructions' size and 3B is push and pop instructions' size)
        // fill the shape of output section file with zeros
        string instruction_mnemonic = catch_parts.str(1);
        cout << "TWO bytes push/pop/int/not: " << instruction_mnemonic << " reg: " << catch_parts.str(2);

        if (instruction_mnemonic == "int")
        {
            cout << "int " << endl;
            location_counter += 2;
            section_table[current_section].size += 2;
        }
        else if (instruction_mnemonic == "push")
        {
            cout << "push " << endl;
            location_counter += 3;
            section_table[current_section].size += 3;
        }
        else if (instruction_mnemonic == "pop")
        {
            cout << "pop " << endl;
            location_counter += 3;
            section_table[current_section].size += 3;
        }
        else if (instruction_mnemonic == "not")
        {
            cout << "not " << endl;
            location_counter += 2;
            section_table[current_section].size += 2;
        }

        return true;
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_one_operand_all_kind_addressing_jumps))
    {
        string instruction_mnemonic = catch_parts.str(1);
        string operand = catch_parts.str(2);
        smatch catch_operand;
        cout << "Jump instructions: call jmp jeq jne jgt: " << instruction_mnemonic << " operand: " << operand << endl;
        if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_absolute))
        {
            // this is form: jmp 5 or jmp label
            // there is payload:
            cout << "label or literal" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_symbol_pc_relative))
        {
            // this is form: jmp %label
            // there is payload: because of pc relative addressing
            cout << "%label - pc relative" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_regdir))
        {
            // this is form: jmp *r0
            // there is not payload:
            cout << " reg dir " << endl;
            location_counter += 3;
            section_table[current_section].size += 3;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_regind))
        {

            // this is form: jmp *[r0]
            // there is not payload:
            cout << " reg ind " << endl;
            location_counter += 3;
            section_table[current_section].size += 3;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_regind_with_displacement))
        {
            // this is form: jmp *[r0 + label] or jmp *[r0 + 5]
            // there is payload:
            cout << " reg ind with displacement" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_memdir))
        {
            // this is form: jmp *5 or jmp *label
            // there is payload:
            cout << "*label or *literal" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }

        else
        {
            error_messages[current_line_number] = "Addressing type is invalid";
            return false;
        }
        return true;
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_two_operand_all_kind_addressing_load_store))
    {
        string instruction_mnemonic = catch_parts.str(1);
        string regD = catch_parts.str(2);
        string operand = catch_parts.str(3);
        smatch catch_operand;
        cout << "Ld/St: ldr str:#" << instruction_mnemonic << "#reg:#" << regD << "#operand:#" << operand << endl;
        if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_absolute))
        {
            // this is form: ld ri, $5 or ld ri, $label
            // there is payload:
            cout << "$label or $literal" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_pc_relative))
        {
            // this is form: ld ri, %label
            // there is payload:
            cout << "%label" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_regdir))
        {
            // this is form: ld ri, rj
            // there is not payload:
            cout << "rj" << endl;
            location_counter += 3;
            section_table[current_section].size += 3;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_regind))
        {
            // this is form: ld ri, [rj]
            // there is not payload:
            cout << "[rj]" << endl;
            location_counter += 3;
            section_table[current_section].size += 3;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_regind_with_displacement))
        {
            // this is form: ld ri, [rj + label / 5]
            // there is payload:
            cout << "[rj + label / 5]" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_memdir))
        {
            // this is form: ld ri, 5 or ld ri, label
            // there is payload:
            cout << "label or literal" << endl;
            location_counter += 5;
            section_table[current_section].size += 5;
        }
        else
        {
            error_messages[current_line_number] = "Addressing type is invalid";
            return false;
        }
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_two_operand_register_instruction))
    {
        string instruction_mnemonic = catch_parts.str(1);
        string regD = catch_parts.str(2);
        string regS = catch_parts.str(3);
        smatch catch_operand;
        cout << "Aritmethical/Logical:#" << instruction_mnemonic << "#regD:#" << regD << "#regS:#" << regS << endl;
        location_counter += 2;
        section_table[current_section].size += 2;
    }
    else
    {
        error_messages[current_line_number] = "Instruction does not exists";
        return false;
    }
    return true;
}
bool AssemblyParser::process_instruction_second_pass(string line_with_instruction)
{
    cout << "SECOND PASS INSTRUCTION:" << endl;
    smatch catch_parts;
    if (regex_search(line_with_instruction, catch_parts, rx_no_operand_instruction))
    {
        // These are halt, iret, ret:
        // increment location_counter for 1 (1B is instruction size), because of compatibility
        // fill the output with the actual binary representation of instruction
        string instruction_mnemonic = catch_parts.str(1);
        cout << "One byte: " << instruction_mnemonic;
        if (instruction_mnemonic == "halt")
        {
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(0x00);
        }
        else if (instruction_mnemonic == "iret")
        {
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(0x20);
        }
        else if (instruction_mnemonic == "ret")
        {
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(0x40);
        }

        location_counter += 1;
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_one_operand_register_instruction))
    {
        // These are push r, pop r, int r, not r:
        // increment location_counter for 2 or 3 (2B is int and not instructions' size and 3B is push and pop instructions' size)
        // fill the output with the actual binary representation of instruction
        string instruction_mnemonic = catch_parts.str(1);
        int register_number;
        if (catch_parts.str(2) == "psw")
        {
            register_number = 8;
        }
        else
        {
            register_number = catch_parts.str(2).at(1) - '0';
        }
        cout << "TWO bytes push/pop/int/not: " << instruction_mnemonic << " reg: " << register_number;

        if (instruction_mnemonic == "int")
        {
            cout << "int: 10 dF , d = " << register_number << endl;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(0x10);
            section_table[current_section].data.push_back((register_number << 4) + 15);
            location_counter += 2;
        }
        else if (instruction_mnemonic == "push")
        {
            cout << "push: B0 ds ua" << endl;
            cout << "d = " << register_number << ", s = 6 (sp->6), u = 1, a = 2" << endl;
            // instruction push does: sp -= 2, mem[sp] = reg
            // this means instructin that places data:
            // U bits are: 0001: dec before
            // A bits are 0010: reg indirect withou displacement
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(0xB0);
            section_table[current_section].data.push_back((register_number << 4) + 6);
            section_table[current_section].data.push_back(0x12);
            location_counter += 3;
        }
        else if (instruction_mnemonic == "pop")
        {
            cout << "pop: A0 ds ua" << endl;
            cout << "d = " << register_number << ", s = 6 (sp->6), u = 4, a = 2" << endl;
            // instruction push does: sp -= 2, mem[sp] = reg
            // this means instructin that places data:
            // U bits are: 0100: inc after
            // A bits are 0010: reg indirect withou displacement
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(0xA0);
            section_table[current_section].data.push_back((register_number << 4) + 6);
            section_table[current_section].data.push_back(0x42);
            location_counter += 3;
        }
        else if (instruction_mnemonic == "not")
        {
            cout << "not: 8m ds , M = 0, D = " << register_number << ", S = F-unused" << endl;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(0x80);
            section_table[current_section].data.push_back((register_number << 4) + 15);
            location_counter += 2;
        }
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_one_operand_all_kind_addressing_jumps))
    {
        string instruction_mnemonic = catch_parts.str(1);
        string operand = catch_parts.str(2);
        smatch catch_operand;
        cout << "Jump instructions: call jmp jeq jne jgt: " << instruction_mnemonic << " operand: " << operand << endl;
        int instr_descr;
        int reg_descr = 0xF0;
        int adr_mode;
        if (instruction_mnemonic == "call")
        {
            instr_descr = 0x30;
        }
        else if (instruction_mnemonic == "jmp")
        {
            instr_descr = 0x50;
        }
        else if (instruction_mnemonic == "jeq")
        {
            instr_descr = 0x51;
        }
        else if (instruction_mnemonic == "jne")
        {
            instr_descr = 0x52;
        }
        else if (instruction_mnemonic == "jgt")
        {
            instr_descr = 0x53;
        }
        if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_absolute))
        {
            // this is form: jmp 5 or jmp label
            // there is payload:
            if (regex_match(operand, rx_symbol))
            {
                cout << "label#" << operand << endl;
                // this is absolute addressing: Need relocation table andeverything!
                // source_reg = F - unimportant
                reg_descr += 0xF;
                // address_mode: ua = 00
                adr_mode = 0;

                int value_to_be_written = process_absolute_addressing_symbol(operand);
                // return value for memory and also create relocation data!
                section_table[current_section].offsets.push_back(location_counter);
                section_table[current_section].data.push_back(instr_descr);
                section_table[current_section].data.push_back(reg_descr);
                section_table[current_section].data.push_back(adr_mode);
                section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
                section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
                location_counter += 5;
            }
            else
            {
                int value = fetch_decimal_value_from_literal(operand);
                cout << "literal#" << value << endl;
                // this is immediate addressing:
                // source_reg = F - unimportant
                reg_descr += 0xF;
                // address_mode: ua = 00
                adr_mode = 0;
                section_table[current_section].offsets.push_back(location_counter);
                section_table[current_section].data.push_back(instr_descr);
                section_table[current_section].data.push_back(reg_descr);
                section_table[current_section].data.push_back(adr_mode);
                section_table[current_section].data.push_back(0xff & (value >> 8));
                section_table[current_section].data.push_back(0xff & (value));
                location_counter += 5;
            }
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_symbol_pc_relative))
        {
            // CHECK THIS AGAIN!!!
            operand = catch_operand.str(1);
            // this is form: jmp %label
            // there is payload: because of pc relative addressing
            cout << "%label - pc relative" << endl;
            // this is pc relative with displacement!:
            //
            // source_reg = F - unimportant
            reg_descr += 0x7;
            // address_mode: ua = 05 - regdir with displacement!

            adr_mode = 0x05;

            int value_to_be_written = process_pc_relative_addressing_symbol(operand);
            // return value for memory and also create relocation data!

            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
            section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
            location_counter += 5;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_regdir))
        {
            // this is form: jmp *r0
            // there is not payload:
            int register_number;
            if (catch_operand.str(1) == "psw")
            {
                register_number = 8;
            }
            else
            {
                register_number = catch_operand.str(1).at(1) - '0';
            }
            cout << " reg dir: " << register_number << endl;
            // source_reg: s = 0-8
            reg_descr += register_number;
            // address_mode: ua = 01 - reg dir!
            adr_mode = 0x01;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            location_counter += 3;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_regind))
        {

            // this is form: jmp *[r0]
            // there is not payload:
            cout << " reg ind " << endl;
            int register_number;
            if (catch_operand.str(1) == "psw")
            {
                register_number = 8;
            }
            else
            {
                register_number = catch_operand.str(1).at(1) - '0';
            }
            cout << " reg ind: " << register_number << endl;
            // source_reg: s = 0-8
            reg_descr += register_number;
            // address_mode: ua = 02 - reg ind!
            adr_mode = 0x02;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            location_counter += 3;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_regind_with_displacement))
        {
            // this is form: jmp *[r0 + label] or jmp *[r0 + 5]
            // there is payload:
            string displacement = catch_operand.str(2);
            int register_number;
            if (catch_operand.str(1) == "psw")
            {
                register_number = 8;
            }
            else
            {
                register_number = catch_operand.str(1).at(1) - '0';
            }
            cout << " reg ind with displacement " << register_number << " " << displacement << endl;
            // source_reg: s = 0-8
            reg_descr += register_number;
            // address_mode: ua = 03 - regind with displacement
            adr_mode = 0x03;
            int value_to_be_written;
            if (regex_match(displacement, rx_symbol))
            {
                cout << "Symbol: " << endl;
                value_to_be_written = process_absolute_addressing_symbol(displacement);
            }
            else
            {
                cout << "Literal: " << endl;
                value_to_be_written = fetch_decimal_value_from_literal(displacement);
            }
            cout << value_to_be_written << endl;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
            section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
            location_counter += 5;
        }
        else if (regex_search(operand, catch_operand, rx_jmp_address_syntax_notation_memdir))
        {

            // this is form: jmp *5 or jmp *label
            // there is payload:
            operand = catch_operand.str(1);
            cout << "*label or *literal" << operand << endl;
            // source_reg F - unimportant
            reg_descr += 0xF;
            // address_mode: ua = 04 - memory
            adr_mode = 0x04;
            int value_to_be_written;

            if (regex_match(operand, rx_symbol))
            {
                cout << "Symbol: " << endl;
                value_to_be_written = process_absolute_addressing_symbol(operand);
            }
            else
            {
                cout << "Literal: " << endl;
                value_to_be_written = fetch_decimal_value_from_literal(operand);
            }
            cout << value_to_be_written << endl;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
            section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
            location_counter += 5;
        }
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_two_operand_all_kind_addressing_load_store))
    {
        string instruction_mnemonic = catch_parts.str(1);
        string regD = catch_parts.str(2);
        string operand = catch_parts.str(3);
        smatch catch_operand;
        cout << "Ld/St: ldr str:#" << instruction_mnemonic << "#reg:#" << regD << "#operand:#" << operand << endl;

        int instr_descr;
        int reg_descr;
        int adr_mode;
        if (instruction_mnemonic == "ldr")
        {
            instr_descr = 0xA0;
        }
        else if (instruction_mnemonic == "str")
        {
            instr_descr = 0xB0;
        }
        if (regD == "psw")
        {
            reg_descr = 0x8;
        }
        else
        {
            reg_descr = regD.at(1) - '0';
        }
        reg_descr <<= 4;
        cout << "I:#" << instr_descr << "#" << reg_descr << "#" << endl;
        if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_absolute))
        {
            // this is form: ld ri, $5 or ld ri, $label
            // there is payload:
            operand = catch_operand.str(1);
            cout << "$label or $literal:" << endl;
            if (regex_match(operand, rx_symbol))
            {
                // this is immediate addressing: Need relocation table and everything!
                // source_reg = F - unimportant
                cout << "label" << operand << endl;
                reg_descr += 0xF;
                // address_mode: ua = 00
                adr_mode = 0;
                int value_to_be_written = process_absolute_addressing_symbol(operand);
                section_table[current_section].offsets.push_back(location_counter);
                section_table[current_section].data.push_back(instr_descr);
                section_table[current_section].data.push_back(reg_descr);
                section_table[current_section].data.push_back(adr_mode);
                section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
                section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
                location_counter += 5;
            }
            else
            {
                cout << "literal" << operand << endl;
                int value = fetch_decimal_value_from_literal(operand);
                cout << "literal#" << value << endl;
                // this is immediate addressing:
                // source_reg = F - unimportant
                reg_descr += 0xF;
                // address_mode: ua = 00
                adr_mode = 0;
                section_table[current_section].offsets.push_back(location_counter);
                section_table[current_section].data.push_back(instr_descr);
                section_table[current_section].data.push_back(reg_descr);
                section_table[current_section].data.push_back(adr_mode);
                section_table[current_section].data.push_back(0xff & (value >> 8));
                section_table[current_section].data.push_back(0xff & (value));
                location_counter += 5;
            }
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_pc_relative))
        {
            // this is form: ld ri, %label
            // there is payload: because of pc relative addressing
            operand = catch_operand.str(1);
            cout << "%label - pc relative" << operand << endl;
            // source_reg = pc: 7 is number of pc
            reg_descr += 0x7;
            // this is pc relative with displacement!:
            // address_mode: ua = 03 - regind with displacement!
            adr_mode = 0x03;

            // return value for memory and also create relocation data!
            int value_to_be_written = process_pc_relative_addressing_symbol(operand);

            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
            section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
            location_counter += 5;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_regdir))
        {
            // this is form: ld ri, rj
            // there is not payload:
            int register_number;
            if (catch_operand.str(1) == "psw")
            {
                register_number = 8;
            }
            else
            {
                register_number = catch_operand.str(1).at(1) - '0';
            }
            cout << " reg dir: " << register_number << endl;
            // source_reg: s = 0-8
            reg_descr += register_number;
            // address_mode: ua = 01 - reg dir!
            adr_mode = 0x01;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            location_counter += 3;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_regind))
        {
            // this is form: ld ri, [rj]
            // there is not payload:
            cout << "[rj]" << endl;
            int register_number;
            if (catch_operand.str(1) == "psw")
            {
                register_number = 8;
            }
            else
            {
                register_number = catch_operand.str(1).at(1) - '0';
            }
            cout << " reg ind: " << register_number << endl;
            // source_reg: s = 0-8
            reg_descr += register_number;
            // address_mode: ua = 02 - reg ind!
            adr_mode = 0x02;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            location_counter += 3;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_regind_with_displacement))
        {
            // this is form: ld ri, [rj + label / 5]
            // there is payload:
            cout << "[rj + label / 5]" << endl;
            string displacement = catch_operand.str(2);
            int register_number;
            if (catch_operand.str(1) == "psw")
            {
                register_number = 8;
            }
            else
            {
                register_number = catch_operand.str(1).at(1) - '0';
            }
            cout << " reg ind with displacement " << register_number << " " << displacement << endl;
            // source_reg: s = 0-8
            reg_descr += register_number;
            // address_mode: ua = 03 - regind with displacement
            adr_mode = 0x03;
            int value_to_be_written;
            if (regex_match(displacement, rx_symbol))
            {
                cout << "Symbol: " << endl;
                value_to_be_written = process_absolute_addressing_symbol(displacement);
            }
            else
            {
                cout << "Literal: " << endl;
                value_to_be_written = fetch_decimal_value_from_literal(displacement);
            }
            cout << value_to_be_written << endl;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
            section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
            location_counter += 5;
        }
        else if (regex_search(operand, catch_operand, rx_load_store_address_syntax_notation_memdir))
        {
            // this is form: ld ri, 5 or ld ri, label
            // there is payload:
            cout << "label or literal#" << operand << endl;
            // source_reg F - unimportant
            reg_descr += 0xF;
            // address_mode: ua = 04 - memory
            adr_mode = 0x04;
            int value_to_be_written;
            if (regex_match(operand, rx_symbol))
            {
                cout << "Symbol: " << endl;
                value_to_be_written = process_absolute_addressing_symbol(operand);
            }
            else
            {
                cout << "Literal: " << endl;
                value_to_be_written = fetch_decimal_value_from_literal(operand);
            }
            cout << value_to_be_written << endl;
            section_table[current_section].offsets.push_back(location_counter);
            section_table[current_section].data.push_back(instr_descr);
            section_table[current_section].data.push_back(reg_descr);
            section_table[current_section].data.push_back(adr_mode);
            section_table[current_section].data.push_back(0xff & (value_to_be_written >> 8)); // location counter + 3
            section_table[current_section].data.push_back(0xff & (value_to_be_written));      // location counter + 4
            location_counter += 5;
        }
        else
        {
            error_messages[current_line_number] = "Addressing type is invalid";
            return false;
        }
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_two_operand_register_instruction))
    {
        string instruction_mnemonic = catch_parts.str(1);
        string regD = catch_parts.str(2);
        string regS = catch_parts.str(3);
        smatch catch_operand;
        cout << "Aritmethical/Logical:#" << instruction_mnemonic << "#regD:#" << regD << "#regS:#" << regS << endl;
        int instr_descr;
        int reg_descr;

        if (instruction_mnemonic == "xchg")
        {
            instr_descr = 0x60;
        }
        else if (instruction_mnemonic == "add")
        {
            instr_descr = 0x70;
        }
        else if (instruction_mnemonic == "sub")
        {
            instr_descr = 0x71;
        }
        else if (instruction_mnemonic == "mul")
        {
            instr_descr = 0x72;
        }
        else if (instruction_mnemonic == "div")
        {
            instr_descr = 0x73;
        }
        else if (instruction_mnemonic == "cmp")
        {
            instr_descr = 0x74;
        }
        else if (instruction_mnemonic == "and")
        {
            instr_descr = 0x81;
        }
        else if (instruction_mnemonic == "or")
        {
            instr_descr = 0x82;
        }
        else if (instruction_mnemonic == "xor")
        {
            instr_descr = 0x83;
        }
        else if (instruction_mnemonic == "test")
        {
            instr_descr = 0x84;
        }
        else if (instruction_mnemonic == "shl")
        {
            instr_descr = 0x90;
        }
        else if (instruction_mnemonic == "shr")
        {
            instr_descr = 0x91;
        }

        if (regD == "psw")
        {
            reg_descr = 0x8;
        }
        else
        {
            reg_descr = regD.at(1) - '0';
        }
        reg_descr <<= 4;
        if (regS == "psw")
        {
            reg_descr += 0x8;
        }
        else
        {
            reg_descr += regS.at(1) - '0';
        }
        cout << hex << "I:#" << instr_descr << "#" << reg_descr << "#" << endl;
        cout << dec;
        section_table[current_section].offsets.push_back(location_counter);
        section_table[current_section].data.push_back(instr_descr);
        section_table[current_section].data.push_back(reg_descr);
        location_counter += 2;
    }
    else
    {
        error_messages[current_line_number] = "Instruction does not exists";
        return false;
    }
    return true;
}

bool AssemblyParser::first_assembly_pass()
{
    cout << endl
         << "FIRST PASS" << endl;
    current_line_number = 0;
    for (string line : input_file_lines)
    {
        current_line_number++;
        smatch catch_parts;
        cout << "(" << current_section << ":" << location_counter << ")" << line << endl;
        if (regex_search(line, catch_parts, rx_label_only))
        { // partially finished

            // only thing in the line is label:
            string label_name = catch_parts.str(1);

            cout << "RX_label_only:" << endl;
            cout << "Find:#" << label_name << endl;

            if (process_label(label_name) == false)
            {
                // this should put label_name to symbol_table
                error_happened = true;
            }

            cout << "###" << endl;
        }
        else
        {
            if (regex_search(line, catch_parts, rx_label_with_command))
            { // partially finished
                // there is label: and another command
                // this command is going to be further processed as a line
                string label_name = catch_parts.str(1);
                line = catch_parts.str(2);

                cout << "RX_label_with:" << endl;
                cout << "Find:#" << label_name << "#" << line << endl;

                if (process_label(label_name) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
            // continue with the line if it was after label:line
            if (regex_search(line, catch_parts, rx_global_directive))
            { // partially finished
                string symbols = catch_parts.str(1);
                stringstream ss(symbols);

                cout << "Global:" << endl;
                cout << "Find:#" << symbols << endl;
                string symbol;
                while (getline(ss, symbol, ','))
                {
                    cout << "#" << symbol << endl;

                    if (process_global_symbol(symbol) == false)
                    {
                        error_happened = true;
                    }
                }
                cout << "###" << endl;
            }
            else if (regex_search(line, catch_parts, rx_extern_directive))
            { // partially finished
                string symbols = catch_parts.str(1);
                stringstream ss(symbols);

                cout << "Extern:" << endl;
                cout << "Find:#" << symbols << endl;
                string symbol;
                while (getline(ss, symbol, ','))
                {
                    cout << "#" << symbol << endl;

                    if (process_extern_symbol(symbol) == false)
                    {
                        error_happened = true;
                    }
                }
                cout << "###" << endl;
            }
            else if (regex_search(line, catch_parts, rx_section_directive))
            { // partially finished

                // Optain, detect and process section
                string section_name = catch_parts.str(1);
                cout << "Section start found:" << endl;
                cout << "Find:#" << section_name << endl;

                if (process_section(section_name) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
            else if (regex_search(line, catch_parts, rx_skip_directive))
            {
                string value = catch_parts.str(1);
                cout << "SKIP found:" << endl;
                cout << "Find:#" << value << endl;

                if (process_skip_first_pass(value) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
            else if (regex_search(line, catch_parts, rx_equ_directive))
            { // -------------------------------------------
                // Not finished at all, wait for examples and check!
                string symbol_name = catch_parts.str(1);
                string value = catch_parts.str(2);
                cout << "EQU found:" << endl;
                cout << "Find:#" << symbol_name << "#" << value << endl;

                if (process_equ_symbol(symbol_name, value) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
            else if (regex_search(line, catch_parts, rx_end_directive))
            {
                // End of assemblu file
                break;
            }
            else if (regex_search(line, catch_parts, rx_word_directive))
            {
                // First pass only has to collect info
                // about number of directives in .word definition
                // and also to see if this .word is in some section and to raise an error
                string data = catch_parts.str(1);
                stringstream ss(data);

                cout << "WORD:" << endl;
                cout << "Find:#" << data << endl;
                string symbol_or_number;
                while (getline(ss, symbol_or_number, ','))
                {
                    cout << "#" << symbol_or_number << endl;

                    if (process_word_first_pass(symbol_or_number) == false)
                    {
                        error_happened = true;
                    }
                }
                cout << "###" << endl;
            }
            else
            {
                // If line goes there, this has to be an instruction or some undefined directive:

                cout << "This is an instruction!" << endl;
                if (process_instruction_first_pass(line) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
        }
    }
    if (error_happened == true)
        return false;
    else
        return true;
}

bool AssemblyParser::second_assembly_pass()
{
    cout << endl
         << "SECOND PASS" << endl;
    current_line_number = 0;
    current_section = "";
    location_counter = 0;
    for (string line : input_file_lines)
    {
        current_line_number++;
        smatch catch_parts;
        cout << "(" << current_section << ":" << location_counter << ")" << line << endl;
        if (regex_search(line, catch_parts, rx_label_only))
        {
            // Nothing to do in the second pass
            cout << "RX_LABEL_ONLY" << endl;
        }
        else
        {
            if (regex_search(line, catch_parts, rx_label_with_command))
            {
                // there is label: and another command
                // this command is going to be further processed as a line
                // nothing to do with the label in the second pass
                string label_name = catch_parts.str(1);
                line = catch_parts.str(2);
                cout << "RX_LABEL_WITH_SMTH" << endl;
            }

            // continue with the line if it was after label:line
            if (regex_search(line, catch_parts, rx_global_directive))
            {
                cout << "GLOBAL" << endl;
                // nothing to do with the .global directive in the second pass
            }
            else if (regex_search(line, catch_parts, rx_extern_directive))
            {
                cout << "EXTERN" << endl;
                // nothing to do with the .global directive in the second pass
            }
            else if (regex_search(line, catch_parts, rx_section_directive))
            {
                // second pass contains only constatation that new section has become,
                // everything else about sections are obtained in the first pass

                string section_name = catch_parts.str(1);

                cout << "SECTION:#" << section_name << endl;
                // second pass of section
                location_counter = 0;
                current_section = section_name;
            }
            else if (regex_search(line, catch_parts, rx_skip_directive))
            {
                // second pass contains only increment location_counter for the value in directive
                string value = catch_parts.str(1);
                cout << "SKIP found:" << endl;
                cout << "Find:#" << value << endl;

                if (process_skip_second_pass(value) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
            else if (regex_search(line, catch_parts, rx_equ_directive))
            {
                cout << "EQU" << endl;
                // nothing to do with the .global directive in the second pass
            }
            else if (regex_search(line, catch_parts, rx_end_directive))
            {
                // End of assembly file
                break;
            }
            else if (regex_search(line, catch_parts, rx_word_directive))
            {
                // First pass only has to collect info
                // about number of directives in .word definition
                // and also to see if this .word is in some section and to raise an error
                string data = catch_parts.str(1);
                stringstream ss(data);

                cout << "WORD:" << endl;
                cout << "Find:#" << data << endl;
                string symbol_or_number;
                while (getline(ss, symbol_or_number, ','))
                {
                    cout << "#" << symbol_or_number << endl;

                    if (process_word_second_pass(symbol_or_number) == false)
                    {
                        error_happened = true;
                    }
                }
                cout << "###" << endl;
            }
            else
            {
                // If line goes there, this has to be an instruction or some undefined directive:

                cout << "This is an instruction!" << endl;
                if (process_instruction_second_pass(line) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
        }
    }
    if (error_happened == true)
        return false;
    else
        return true;
}

void AssemblyParser::print_symbol_table()
{
    cout << "Symbol table:" << endl;
    cout << "Value\tType\tSection\t\tName\t\tId" << endl;
    for (map<string, SymbolTable>::iterator it = symbol_table.begin(); it != symbol_table.end(); it++)
    {
        cout << hex << setfill('0') << setw(4) << (0xffff & it->second.value) << "\t";
        // extern symbols?
        if (it->second.is_local == true)
            cout << "l\t";
        else
        {
            if (it->second.is_defined == true)
                cout << "g\t";
            else
            {
                if (it->second.is_extern)
                    cout << "e\t";
                else
                    cout << "u\t"; // undefined
            }
        }
        cout << it->second.section << "\t" << it->second.name << "\t" << hex << setfill('0') << setw(4) << (0xffff & it->second.id_symbol) << endl;
    }
    cout << dec;
}
void AssemblyParser::print_section_table()
{
    cout << "Section table:" << endl;
    cout << "Id\tName\t\tSize" << endl;
    for (map<string, SectionTable>::iterator it = section_table.begin(); it != section_table.end(); it++)
    {
        cout << it->second.id_section << "\t" << it->second.section_name << "\t" << hex << setfill('0') << setw(4) << (0xffff & it->second.size) << endl;
    }
    cout << dec;
}

void AssemblyParser::print_section_data()
{
    cout << "Data:" << endl
         << endl;
    for (map<string, SectionTable>::iterator it = section_table.begin(); it != section_table.end(); it++)
    {
        if (it->first == "UNDEFINED")
            continue;
        if (it->first == "ABSOLUTE")
            continue;
        cout << "Section: " << it->first << endl;

        SectionTable s_table = it->second;
        int counter = 0;
        cout << "?" << s_table.data.size() << "=" << s_table.offsets.size() << endl;
        for (int i = 0; i < s_table.offsets.size() - 1; i++)
        {

            int current_offset = s_table.offsets[i];
            int next_offset = s_table.offsets[i + 1];
            cout << hex << setfill('0') << setw(4) << (0xffff & current_offset) << ": ";
            for (int j = current_offset; j < next_offset; j++)
            {
                char c = s_table.data[j];
                cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
            }
            cout << endl;
        }
        // Last directive which is in memory
        int current_offset = s_table.offsets[s_table.offsets.size() - 1];
        int next_offset = s_table.data.size();
        cout << hex << setfill('0') << setw(4) << (0xffff & current_offset) << ": ";
        for (int j = current_offset; j < next_offset; j++)
        {
            char c = s_table.data[j];
            cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
        }
        cout << endl;

        cout << dec;
        cout << endl;
    }
}
void AssemblyParser::print_relocation_table()
{
    cout << "Relocation data:" << endl
         << endl;
    cout << "Offset\tType\t\tDat/Ins\tSymbol\tSection name" << endl;
    for (RelocationTable rel_data : relocation_table)
    {
        cout << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset) << "\t" << rel_data.type << "\t" << (rel_data.is_data ? 'd' : 'i') << "\t" << rel_data.symbol_name << "\t" << rel_data.section_name << endl;
    }
    cout << dec;
}
void AssemblyParser::print_error_messages()
{
    cout << "Assmebly detects some errors:" << endl;
    for (map<int, string>::iterator it = error_messages.begin(); it != error_messages.end(); it++)
    {
        cout << "Line " << transfer_error_lines[it->first] << ":" << it->second << endl;
    }
}