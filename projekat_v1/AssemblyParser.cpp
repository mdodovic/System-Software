#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "AssemblyParser.h"
#include "AssemblyRegexes.h"
using namespace std;

int AssemblyParser::id_symbol_in_symbol_table = 0;
int AssemblyParser::id_section_in_section_table = 0;

AssemblyParser::AssemblyParser(string path)
    : path_to_file(path), location_counter(0), current_section("")
{
    // symbol UNDEFINED represents section with id=0
    // it is used to mark all extern symbols
    SectionTable undefined_section;
    undefined_section.id_section = id_section_in_section_table++;
    undefined_section.section_name = "UNDEFINED";
    undefined_section.size = 0;
    //    undefined_section.data;
    section_table["UNDEFINED"] = undefined_section;
    // UNDEFINED is symbol so it has to be in symbol table
    SymbolTable undefined_section_symbol;
    undefined_section_symbol.id_symbol = id_symbol_in_symbol_table++;
    undefined_section_symbol.is_defined = false;
    undefined_section_symbol.is_extern = false;
    undefined_section_symbol.is_local = false;
    undefined_section_symbol.name = "UNDEFINED";
    undefined_section_symbol.section = "UNDEFINED";
    undefined_section_symbol.value = 0;
    symbol_table["UNDEFINED"] = undefined_section_symbol;
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
        // and it is undefined and unextern
        // set definition, value and section where it is defined

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
        // end previous section
        // set its size in section table
    }
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
        // if symbol exists it is now global
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
    return true;
}

bool AssemblyParser::first_assembly_pass()
{
    //cout << endl << "File" << endl;
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
            {
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
            {
                string symbols = catch_parts.str(1);
                cout << "Global:" << endl;
                cout << "Find:#" << symbols << endl;

                if (process_global_symbol(symbols) == false)
                {
                    error_happened = true;
                }
                cout << "###" << endl;
            }
            else if (regex_search(line, catch_parts, rx_extern_directive))
            {
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
            }
            else if (regex_search(line, catch_parts, rx_equ_directive))
            {
            }
            else if (regex_search(line, catch_parts, rx_end_directive))
            {
                // End of assemblu file
                break;
            }
            else if (regex_search(line, catch_parts, rx_word_directive))
            {
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
        cout << it->second.value << "\t";
        if (it->second.is_local == true)
            cout << "l\t";
        else
            cout << "g\t";
        cout << it->second.section << "\t" << it->second.name << "\t" << it->second.id_symbol << endl;
    }
}
void AssemblyParser::print_section_table()
{
    cout << "Section table:" << endl;
    cout << "Id\tName\t\tSize" << endl;
    for (map<string, SectionTable>::iterator it = section_table.begin(); it != section_table.end(); it++)
    {
        cout << it->second.id_section << "\t" << it->second.section_name << "\t" << it->second.size << endl;
    }
}

void AssemblyParser::print_relocation_table()
{
}
void AssemblyParser::print_section_data()
{
}
