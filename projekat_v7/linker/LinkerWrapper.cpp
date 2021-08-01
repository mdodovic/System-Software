#include <iostream>
#include <fstream>
#include <iomanip>

#include "LinkerWrapper.h"

using namespace std;

LinkerWrapper::LinkerWrapper(string output_file_name_, vector<string> files_to_be_linked_, bool linkable_output_, map<string, int> mapped_section_address_)
    : output_file_name(output_file_name_), files_to_be_linked(files_to_be_linked_), linkable_output(linkable_output_), mapped_section_address(mapped_section_address_)
{
}

bool LinkerWrapper::collect_data_from_relocatible_files()
{

    cout << "Collect data from all files:" << endl;
    for (string filename_to_be_linked : files_to_be_linked)
    {

        cout << filename_to_be_linked << " ";
        ifstream input_file(filename_to_be_linked, ios::binary);
        if (input_file.fail())
        {
            error_messages.push_back("File " + filename_to_be_linked + " does not exists!");
            return false;
        }

        // Relocation

        int number_of_relocations = 0;
        input_file.read((char *)&number_of_relocations, sizeof(number_of_relocations));

        cout << number_of_relocations << endl;
        cout << "Relocation data:" << endl;

        cout << "Offset\tType\t\tDat/Ins\tSymbol\tSection name" << endl;
        vector<RelocationTable> relocation_table_vector;
        for (int i = 0; i < number_of_relocations; i++)
        {
            RelocationTable rel_data;
            cout << sizeof(rel_data) << endl;
            input_file.read((char *)(&rel_data.addend), sizeof(rel_data.addend));
            input_file.read((char *)(&rel_data.is_data), sizeof(rel_data.is_data));
            input_file.read((char *)(&rel_data.offset), sizeof(rel_data.offset));

            unsigned int stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            rel_data.section_name.resize(stringLength);
            input_file.read((char *)rel_data.section_name.c_str(), stringLength);

            stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            rel_data.symbol_name.resize(stringLength);
            input_file.read((char *)rel_data.symbol_name.c_str(), stringLength);

            stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            rel_data.type.resize(stringLength);
            input_file.read((char *)rel_data.type.c_str(), stringLength);

            cout << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset) << "\t" << rel_data.type << "\t" << (rel_data.is_data ? 'd' : 'i') << "\t" << rel_data.symbol_name << "\t" << rel_data.section_name << endl;
            relocation_table_vector.push_back(rel_data);
        }
        cout << dec;
        // finished with relocation
        relocation_table_per_file[filename_to_be_linked] = relocation_table_vector;
        // added relocation to map!

        map<string, SymbolTable> symbol_table_map;
        int number_of_symbols = 0;
        input_file.read((char *)&number_of_symbols, sizeof(number_of_symbols));
        cout << number_of_symbols << endl;
        cout << "Symbol table:" << endl;
        cout << "Value\tType\tSection\t\tName\t\tId" << endl;
        for (int i = 0; i < number_of_symbols; i++)
        {
            SymbolTable sym_table;
            string sym_name;
            unsigned int stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            sym_name.resize(stringLength);
            input_file.read((char *)sym_name.c_str(), stringLength);
            cout << sym_name << endl;

            input_file.read((char *)(&sym_table.id_symbol), sizeof(sym_table.id_symbol));
            input_file.read((char *)(&sym_table.is_defined), sizeof(sym_table.is_defined));
            input_file.read((char *)(&sym_table.is_extern), sizeof(sym_table.is_extern));
            input_file.read((char *)(&sym_table.is_local), sizeof(sym_table.is_local));
            input_file.read((char *)(&sym_table.value), sizeof(sym_table.value));

            stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            sym_table.name.resize(stringLength);
            input_file.read((char *)sym_table.name.c_str(), stringLength);

            stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            sym_table.section.resize(stringLength);
            input_file.read((char *)sym_table.section.c_str(), stringLength);

            cout << hex << setfill('0') << setw(4) << (0xffff & sym_table.value) << "\t";
            // extern symbols?
            if (sym_table.is_local == true)
                cout << "l\t";
            else
            {
                if (sym_table.is_defined == true)
                    cout << "g\t";
                else
                {
                    if (sym_table.is_extern)
                        cout << "e\t";
                    else
                        cout << "u\t"; // undefined
                }
            }
            cout << sym_table.section << "\t" << sym_table.name << "\t" << hex << setfill('0') << setw(4) << (0xffff & sym_table.id_symbol) << endl;
            symbol_table_map[sym_name] = sym_table;
        }
        // finished with symbol table:

        symbol_table_per_file[filename_to_be_linked] = symbol_table_map;
        // added symbol table to map

        // Section
        map<string, SectionTable> section_table_map;

        int number_of_sections = 0;
        input_file.read((char *)&number_of_sections, sizeof(number_of_sections));
        cout << number_of_sections << endl;
        cout << "Section table:" << endl;
        cout << "Id\tName\t\tSize" << endl;
        for (int i = 0; i < number_of_sections; i++)
        {
            struct SectionTable sec_table;
            string sec_name;
            unsigned int stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            sec_name.resize(stringLength);
            input_file.read((char *)sec_name.c_str(), stringLength);
            cout << sec_name << endl;

            input_file.read((char *)(&sec_table.id_section), sizeof(sec_table.id_section));
            input_file.read((char *)(&sec_table.size), sizeof(sec_table.size));

            sec_table.virtual_memory_address = 0;

            stringLength;
            input_file.read((char *)(&stringLength), sizeof(stringLength));
            sec_table.section_name.resize(stringLength);
            input_file.read((char *)sec_table.section_name.c_str(), stringLength);

            int number_of_chars;
            input_file.read((char *)(&number_of_chars), sizeof(number_of_chars));
            cout << dec << number_of_chars << hex << endl;

            for (int j = 0; j < number_of_chars; j++)
            {
                char c;
                input_file.read((char *)(&c), sizeof(c));
                sec_table.data.push_back(c);
            }
            cout << dec << sec_table.data.size() << hex << endl;
            cout << "Data:" << endl;

            int counter = 0;
            for (char c : sec_table.data)
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

            int number_of_offsets;
            input_file.read((char *)(&number_of_offsets), sizeof(number_of_offsets));
            cout << dec << number_of_offsets << hex << endl;

            for (int j = 0; j < number_of_offsets; j++)
            {
                int x;
                input_file.read((char *)(&x), sizeof(x));
                sec_table.offsets.push_back(x);
            }
            cout << dec << sec_table.offsets.size() << endl;
            for (int c : sec_table.offsets)
            {
                cout << c << " ";
            }
            cout << endl;
            cout << sec_table.id_section << "\t" << sec_table.section_name << "\t" << hex << setfill('0') << setw(4) << (0xffff & sec_table.size) << endl;
            section_table_map[sec_name] = sec_table;
        }
        // finished with section table:

        section_table_per_file[filename_to_be_linked] = section_table_map;
        // added symbol table to map

        input_file.close();
    }
    cout << endl;

    return true;
}

int LinkerWrapper::fetch_start_address_of_section(string section_name)
{
    if (linkable_output)
    {
        // This option ingores every -place options and set all sections on offset 0
        // this also means that the output of -linkable option is relocatible file which can be linked again
        return 0;
    }
    else
    {
        //    map<string, int> mapped_section_address;)
        cout << "This is in progress" << endl;
        return -1;
    }
}

bool LinkerWrapper::create_aggregate_sections()
{

    map<string, int> previous_end;
    for (string filename : files_to_be_linked)
    {
        cout << "File: " << filename << endl;
        for (map<string, SectionTable>::iterator it = section_table_per_file[filename].begin(); it != section_table_per_file[filename].end(); it++)
        {
            previous_end[it->second.section_name] = 0;
        }
    }

    for (string filename : files_to_be_linked)
    {

        cout << "File: " << filename << endl;

        for (map<string, SectionTable>::iterator it = section_table_per_file[filename].begin(); it != section_table_per_file[filename].end(); it++)
        {
            cout << it->second.id_section << "\t" << it->first << "=" << it->second.section_name << "\t" << hex << setfill('0') << setw(4) << (0xffff & it->second.size) << endl;
            if (it->second.section_name == "UNDEFINED" || it->second.section_name == "ABSOLUTE")
            {
                continue;
            }
            SectionAdditionalData additional_data;
            additional_data.parent_file_name = filename; // this is file where this section is present!
            additional_data.parent_section_size = it->second.size;
            additional_data.section_name = it->second.section_name;
            additional_data.start_addres_in_aggregate_section = previous_end[it->second.section_name];
            previous_end[it->second.section_name] = previous_end[it->second.section_name] + additional_data.parent_section_size;

            section_additional_helper[it->second.section_name].push_back(additional_data);
        }
    }

    int id_section = 0;
    for (map<string, int>::iterator it = previous_end.begin();
         it != previous_end.end(); it++)
    {
        SectionTable next_section_table;
        next_section_table.section_name = it->first;
        if (it->first == "UNDEFINED")
        {
            next_section_table.id_section = 0;
        }
        else if (it->first == "ABSOLUTE")
        {
            next_section_table.id_section = -1;
        }
        else
        {
            next_section_table.id_section = id_section++;
        }
        next_section_table.size = it->second;
        next_section_table.virtual_memory_address = fetch_start_address_of_section(next_section_table.section_name);

        this->output_section_table[it->first] = next_section_table;
    }

    return true;
}

bool LinkerWrapper::create_aggregate_symbol_table()
{
    for (map<string, SectionTable>::iterator it = output_section_table.begin();
         it != output_section_table.end(); it++)
    {
        SymbolTable symbol;
        symbol.id_symbol = it->second.id_section;
        symbol.is_defined = true;
        symbol.is_extern = false;
        symbol.is_local = true;
        symbol.name = it->second.section_name;
        symbol.section = it->second.section_name;
        symbol.value = it->second.virtual_memory_address;
        output_symbol_table[symbol.name] = symbol;
    }
    /*    for (string filename : files_to_be_linked)
    {

        cout << "File: " << filename << endl;
        map<string, SymbolTable> symbol_table = symbol_table_per_file[filename];

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
    */
    return true;
}

void LinkerWrapper::print_error_messages()
{
    for (string err : error_messages)
    {
        cout << "Linker error: " << err << endl;
    }
}

void LinkerWrapper::print_symbol_table()
{
    for (map<string, map<string, SymbolTable>>::iterator it_pf = symbol_table_per_file.begin(); it_pf != symbol_table_per_file.end(); it_pf++)
    {
        map<string, SymbolTable> symbol_table = it_pf->second;
        cout << "Symbol table:" << it_pf->first << endl;
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
}
void LinkerWrapper::print_section_table()
{
    for (map<string, map<string, SectionTable>>::iterator it_pf = section_table_per_file.begin(); it_pf != section_table_per_file.end(); it_pf++)
    {
        map<string, SectionTable> section_table = it_pf->second;
        cout << "Section table:" << it_pf->first << endl;
        cout << "Id\tName\t\tSize" << endl;
        for (map<string, SectionTable>::iterator it = section_table.begin(); it != section_table.end(); it++)
        {
            cout << it->second.id_section << "\t" << it->second.section_name << "\t" << hex << setfill('0') << setw(4) << (0xffff & it->second.size) << endl;
        }
        cout << dec;
    }
}

void LinkerWrapper::print_section_data()
{
    for (map<string, map<string, SectionTable>>::iterator it_pf = section_table_per_file.begin(); it_pf != section_table_per_file.end(); it_pf++)
    {
        map<string, SectionTable> section_table = it_pf->second;

        cout << "Data:" << it_pf->first << endl
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
}
void LinkerWrapper::print_relocation_table()
{
    for (map<string, vector<RelocationTable>>::iterator it_pf = relocation_table_per_file.begin(); it_pf != relocation_table_per_file.end(); it_pf++)
    {
        vector<RelocationTable> relocation_table = it_pf->second;
        cout << "Relocation data:" << it_pf->first << endl
             << endl;
        cout << "Offset\tType\t\tDat/Ins\tSymbol\tSection name" << endl;
        for (RelocationTable rel_data : relocation_table)
        {
            cout << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset) << "\t" << rel_data.type << "\t" << (rel_data.is_data ? 'd' : 'i') << "\t" << rel_data.symbol_name << "\t" << rel_data.section_name << endl;
        }
        cout << dec;
    }
}

void LinkerWrapper::fill_output_file()
{
    ofstream text_output_file(this->output_file_name);
    text_output_file << "Linker output" << endl
                     << endl;
    text_output_file << "Helper info:" << endl;
    text_output_file << hex;

    for (map<string, vector<SectionAdditionalData>>::iterator it = section_additional_helper.begin();
         it != section_additional_helper.end(); it++)
    {
        text_output_file << "Section: " << it->first << endl;
        for (SectionAdditionalData additional_data : it->second)
        {
            text_output_file << "\t" << additional_data.section_name << "\t" << additional_data.parent_file_name;
            text_output_file << ":\t" << additional_data.parent_section_size;
            text_output_file << " [" << additional_data.start_addres_in_aggregate_section << ",";
            text_output_file << additional_data.start_addres_in_aggregate_section + additional_data.parent_section_size << ")" << endl;
        }
        text_output_file << endl;
    }

    text_output_file << "Section table" << endl;
    text_output_file << "Id\tName\t\tSize\tVA" << endl;
    for (map<string, SectionTable>::iterator it = output_section_table.begin(); it != output_section_table.end(); it++)
    {
        text_output_file << it->second.id_section << "\t" << it->second.section_name << "\t";
        text_output_file << hex << setfill('0') << setw(4) << (0xffff & it->second.size) << "\t";
        text_output_file << hex << setfill('0') << setw(4) << (0xffff & it->second.virtual_memory_address) << endl;
    }

    text_output_file << dec;
    text_output_file << endl;
    text_output_file << "Symbol table" << endl;
    text_output_file << "Value\tType\tSection\t\tName\t\tId" << endl;
    for (map<string, SymbolTable>::iterator it = output_symbol_table.begin(); it != output_symbol_table.end(); it++)
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
}
