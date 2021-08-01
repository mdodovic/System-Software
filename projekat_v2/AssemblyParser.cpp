#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <iomanip>

#include "AssemblyParser.h"
#include "AssemblyRegexes.h"
using namespace std;

int AssemblyParser::id_symbol_in_symbol_table = 0;
int AssemblyParser::id_section_in_section_table = 0;

AssemblyParser::AssemblyParser(string path)
    : path_to_file(path), location_counter(0), current_section("")
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
    return true;
}

bool AssemblyParser::clear_input_file()
{
    ifstream input_file;
    input_file.open(path_to_file);
    if (input_file.is_open() == false)
        return false;
    string line;
    while (getline(input_file, line))
    {
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
        // Error, equ defines absolute symbol
        error_messages[current_line_number] = "equ directive cannot define an absolute symbol that is already defined";
        return false;
    }
    /*
    TEST THIS - SEEMS WRONG
    */

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
bool AssemblyParser::process_skip_first_pass(string value)
{
    // check if it is in section?
    if (current_section == "")
    {
        error_messages[current_line_number] = "skip has to be in section!";
        return false;
    }

    cout << "Want numeric value?!" << endl;

    int skip_value = fetch_decimal_value_from_literal(value);
    // fill data with zeros
    section_table[current_section].offsets.push_back(location_counter);
    for (int i = 0; i < skip_value; i++)
        section_table[current_section].data.push_back(0);

    location_counter += skip_value;
    section_table[current_section].size += skip_value;

    for (char c : section_table[current_section].data)
    {
        cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
    }
    cout << dec;
    return true;
}

bool AssemblyParser::process_skip_second_pass(string value)
{

    int skip_value = fetch_decimal_value_from_literal(value);
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
                        rel_data.type = "R_X86_64_16";
                        rel_data.section_name = current_section;
                        rel_data.offset = location_counter;
                        rel_data.symbol_name = s_table.section; // this is local symbol, so only section is necessary !
                        rel_data.addend = 0;
                        relocation_table.push_back(rel_data);
                    }
                    else
                    {
                        // add zeros to data
                        section_table[current_section].offsets.push_back(location_counter);
                        section_table[current_section].data.push_back(0 & 0xff);
                        section_table[current_section].data.push_back((0 >> 8) & 0xff);
                        // create relocation data for this symbol
                        RelocationTable rel_data;
                        rel_data.type = "R_X86_64_16";
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
                    rel_data.type = "R_X86_64_16";
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
            location_counter += 2;
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
        // fill the shape of outut section file with zeros
        cout << "One byte: " << catch_parts.str(1);
        section_table[current_section].offsets.push_back(location_counter);
        section_table[current_section].data.push_back(0);
        location_counter += 1;
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_one_operand_data_instruction))
    {
        return true;
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
            section_table[current_section].data[location_counter] = 0x00;
        }
        else if (instruction_mnemonic == "iret")
        {
            section_table[current_section].data[location_counter] = 0x20;
        }
        else if (instruction_mnemonic == "ret")
        {
            section_table[current_section].data[location_counter] = 0x40;
        }

        location_counter += 1;
    }
    else if (regex_search(line_with_instruction, catch_parts, rx_one_operand_data_instruction))
    {
        return true;
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
        //        if (it->first == "UNDEFINED")
        //            continue;
        cout << "Section: " << it->first << endl;

        SectionTable s_table = it->second;
        int counter = 0;
        for (char c : s_table.data)
        {
            if (counter % 16 == 0)
            {
                cout << hex << setfill('0') << setw(4) << (0xffff & counter) << "   ";
            }
            cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
            counter++;
            if (counter % 16 == 0)
            {
                cout << endl;
            }
        }
        cout << dec;
        cout << endl;
    }
}
void AssemblyParser::print_relocation_table()
{
    cout << "Relocation data:" << endl
         << endl;
    cout << "Offset\tType\t\tSymbol\tSection name" << endl;
    for (RelocationTable rel_data : relocation_table)
    {
        cout << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset) << "\t" << rel_data.type << "\t" << rel_data.symbol_name << "\t" << rel_data.section_name << endl;
    }
    cout << dec;
}
void AssemblyParser::print_error_messages()
{
    cout << "Assmebly detects some errors:" << endl;
    for (map<int, string>::iterator it = error_messages.begin(); it != error_messages.end(); it++)
    {
        cout << "Line " << it->first << ":" << it->second << endl;
    }
}