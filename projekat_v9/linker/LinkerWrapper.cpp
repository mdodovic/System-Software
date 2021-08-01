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

bool LinkerWrapper::move_sections_to_virtual_address()
{
    if (linkable_output == true)
    {
        return true;
    }
    // this is -hex option and all -place options are taken into consideration
    // all sections not mentined in -place comes after the last section from -place list
    int next_address_for_nonmentioned_sections = 0;
    for (map<string, int>::iterator it_place = mapped_section_address.begin();
         it_place != mapped_section_address.end(); it_place++)
    {
        cout << it_place->first << ":" << it_place->second << endl;

        for (map<string, SectionAdditionalData>::iterator it_filename = section_additional_helper[it_place->first].begin();
             it_filename != section_additional_helper[it_place->first].end(); it_filename++)
        {
            cout << "F: " << it_filename->first << " :" << it_filename->second.start_addres_in_aggregate_section << endl;
            it_filename->second.start_addres_in_aggregate_section += it_place->second;
            if (it_filename->second.start_addres_in_aggregate_section + it_filename->second.parent_section_size > next_address_for_nonmentioned_sections)
            {
                next_address_for_nonmentioned_sections = it_filename->second.start_addres_in_aggregate_section + it_filename->second.parent_section_size;
            }
        }
    }
    cout << "NEXT ADDRESS: " << next_address_for_nonmentioned_sections << endl;
    //section_additional_helper

    cout << "Linker output" << endl
         << endl;
    cout << "Helper info:" << endl;
    cout << hex;

    for (map<string, map<string, SectionAdditionalData>>::iterator it_section = section_additional_helper.begin();
         it_section != section_additional_helper.end(); it_section++)
    {
        cout << "Section: " << it_section->first << " :{" << endl;
        for (map<string, SectionAdditionalData>::iterator it_filename = it_section->second.begin();
             it_filename != it_section->second.end(); it_filename++)
        {
            cout << "\t"
                 << "# " << it_filename->first;
            SectionAdditionalData additional_data = it_filename->second;

            cout << "\t\t" << additional_data.section_name << "\t" << additional_data.parent_file_name;
            cout << ":\t" << additional_data.parent_section_size;
            cout << " [" << additional_data.start_addres_in_aggregate_section << ",";
            cout << additional_data.start_addres_in_aggregate_section + additional_data.parent_section_size << ")";

            cout << endl;
        }
        cout << "\t}" << endl;
        cout << endl;
    }

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
        exit(-1);
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
            if (it->second.section_name == "UNDEFINED") // || it->second.section_name == "ABSOLUTE")
            {
                continue;
            }
            SectionAdditionalData additional_data;
            additional_data.parent_file_name = filename; // this is file where this section is present!
            additional_data.parent_section_size = it->second.size;
            additional_data.section_name = it->second.section_name;
            additional_data.start_addres_in_aggregate_section = previous_end[it->second.section_name];
            previous_end[it->second.section_name] = previous_end[it->second.section_name] + additional_data.parent_section_size;

            section_additional_helper[it->second.section_name][filename] = additional_data;
        }
    }

    if (move_sections_to_virtual_address() == false)
    {
        return false;
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
    int next_symbol_id = 0;
    for (map<string, SectionTable>::iterator it = output_section_table.begin();
         it != output_section_table.end(); it++)
    {
        SymbolTable symbol;
        symbol.id_symbol = it->second.id_section;
        if (symbol.id_symbol > next_symbol_id)
        {
            next_symbol_id = symbol.id_symbol;
        }
        symbol.is_defined = true;
        symbol.is_extern = false;
        symbol.is_local = true;
        symbol.name = it->second.section_name;
        symbol.section = it->second.section_name;
        symbol.value = it->second.virtual_memory_address;
        output_symbol_table[symbol.name] = symbol;
    }
    next_symbol_id += 1;
    map<string, SymbolTable> extern_symbols_from_every_file;

    for (string filename : files_to_be_linked)
    {
        cout << "File: " << filename << endl;
        map<string, SymbolTable> symbol_table_of_file = symbol_table_per_file[filename];
        for (map<string, SymbolTable>::iterator it = symbol_table_of_file.begin(); it != symbol_table_of_file.end(); it++)
        {
            if (it->second.name == it->second.section)
            {
                // sections are already in symbol table!
                continue;
            }
            // check if the symbol is extern - put it sideward for further discussion!
            if (it->second.is_extern == true)
            {
                cout << "Extern( " << filename << ": " << it->second.name << ")" << endl;
                extern_symbols_from_every_file[it->first] = it->second;
                continue;
            }

            // check if symbol is already in symbol table
            // it can be undefined or defined
            map<string, SymbolTable>::iterator sym = output_symbol_table.find(it->first);

            if (sym != output_symbol_table.end())
            {
                // symbol exists in the symbol table
                // and it is not extern => this is multiple definition!
                string error_message = "Symbol " + sym->first + " is already defined!";
                error_messages.push_back(error_message);
                return false;
            }
            else
            {
                // symbol does not exists in the symbol table:
                // add offset of the section in output file
                // for -linkable this is just a position of the section into aggregate section
                // for -hex this is offset into aggregate section and offset of the beginning of the aggregate section
                // both increments are into section_additional_helper.start_address_in_aggregate_section
                cout << "->" << it->first << "=" << it->second.name << ":" << it->second.is_defined << it->second.is_extern << endl;
                it->second.id_symbol = next_symbol_id++;
                cout << "Should be added: " << section_additional_helper[it->second.section][filename].start_addres_in_aggregate_section << endl;
                if (it->second.section != "ABSOLUTE")
                {
                    // if this is absolute symbol, no action needs, because this symbol has constant value
                    it->second.value = it->second.value + section_additional_helper[it->second.section][filename].start_addres_in_aggregate_section;
                }
                output_symbol_table[it->first] = it->second;
            }
        }
    }
    for (map<string, SymbolTable>::iterator it = extern_symbols_from_every_file.begin();
         it != extern_symbols_from_every_file.end(); it++)
    {
        map<string, SymbolTable>::iterator sym = output_symbol_table.find(it->first);

        if (sym != output_symbol_table.end())
        {
            cout << "HERE: " << sym->first << "!" << endl;
        }
        else
        {
            cout << "EXTERNAL" << endl;
            if (linkable_output == false)
            {
                cout << "This is in progress" << endl;
                exit(-1);
            }
            else
            {
                it->second.id_symbol = next_symbol_id++;
                output_symbol_table[it->first] = it->second;
            }
        }
    }
    return true;
}

bool LinkerWrapper::create_aggregate_relocations_linkable()
{
    // Output file should be linkable with others relocatible files.
    // All necessary relocations remains in object file
    for (string filename : files_to_be_linked)
    {
        vector<RelocationTable> relocation_table = relocation_table_per_file[filename];

        for (RelocationTable rel_data : relocation_table)
        {
            cout << filename << ":" << rel_data.section_name;
            cout << "(+" << section_additional_helper[rel_data.section_name][filename].start_addres_in_aggregate_section;
            cout << "-" << output_section_table[rel_data.section_name].virtual_memory_address << ")"
                 << "\t#";
            cout << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset) << "\t" << rel_data.type << "\t" << (rel_data.is_data ? 'd' : 'i') << "\t" << rel_data.symbol_name << "\t" << rel_data.section_name << endl;
            RelocationTable output_relocation_data;
            output_relocation_data.addend = rel_data.addend;
            output_relocation_data.is_data = rel_data.is_data;
            // this offset is created:
            // offset in initial section
            // + offset of this initial section in the aggregate section
            // - offset of this aggregate section in memory
            // hence this offset is offset in aggregate section which starts at the position 0 in the memory
            // Be careful to add aggregate section offset to this saved offset to reach exact byte!
            output_relocation_data.offset = rel_data.offset + section_additional_helper[rel_data.section_name][filename].start_addres_in_aggregate_section - output_section_table[rel_data.section_name].virtual_memory_address;

            output_relocation_data.section_name = rel_data.section_name;
            output_relocation_data.symbol_name = rel_data.symbol_name;
            output_relocation_data.type = rel_data.type;
            output_relocation_data.filename = filename; // this is only useful when symbol is section name (relocation data related to local symbol)
            output_relocation_table.push_back(output_relocation_data);
        }
        cout << dec;
    }
    return true;
}

bool LinkerWrapper::create_aggregate_relocations_hex()
{
    exit(-1);
    return true;
}

bool LinkerWrapper::create_aggregate_relocations()
{
    if (linkable_output == true)
        return create_aggregate_relocations_linkable();
    else
        return create_aggregate_relocations_hex();
}

bool LinkerWrapper::create_aggregate_content_of_sections_linkable()
{
    for (map<string, SectionTable>::iterator it = output_section_table.begin(); it != output_section_table.end(); it++)
    {
        string section_name = it->first;
        cout << "Section: " << section_name << "(" << it->second.size << ")" << endl;
        if (it->second.size == 0)
        {
            // If there is no data, nothing to be done with this section in this file!
            continue;
        }

        for (string filename : files_to_be_linked)
        {
            cout << "File: " << filename << endl;
            map<string, SectionAdditionalData>::iterator it_section_additional_data = section_additional_helper[it->first].find(filename);
            if (it_section_additional_data == section_additional_helper[it->first].end())
            {
                continue;
            }

            SectionAdditionalData section_additional_data = it_section_additional_data->second;
            cout << "S:" << section_additional_data.section_name << " F:" << section_additional_data.parent_file_name << endl;
            vector<int> section_file_offsets = this->section_table_per_file[filename][section_name].offsets;
            vector<char> section_file_data = this->section_table_per_file[filename][section_name].data;
            cout << section_file_offsets.size() << "?" << section_file_data.size() << endl;

            for (int i = 0; i < section_file_offsets.size() - 1; i++)
            {
                int current_offset = section_file_offsets[i];
                int next_offset = section_file_offsets[i + 1];
                //cout << ":" << current_offset << "->" << next_offset << endl;
                cout << hex << setfill('0') << setw(4) << (0xffff & (current_offset + section_additional_helper[section_name][filename].start_addres_in_aggregate_section)) << ": ";
                it->second.offsets.push_back(current_offset + section_additional_helper[section_name][filename].start_addres_in_aggregate_section);
                for (int j = current_offset; j < next_offset; j++)
                {
                    char c = section_file_data[j];
                    cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
                    it->second.data.push_back(c);
                }
                cout << endl;
            }
            cout << dec;
            // Last directive which is in memory
            int current_offset = section_file_offsets[section_file_offsets.size() - 1];
            int next_offset = section_file_data.size();
            cout << hex << setfill('0') << setw(4) << (0xffff & (current_offset + section_additional_helper[section_name][filename].start_addres_in_aggregate_section)) << ": ";
            it->second.offsets.push_back(current_offset + section_additional_helper[section_name][filename].start_addres_in_aggregate_section);
            for (int j = current_offset; j < next_offset; j++)
            {
                char c = section_file_data[j];
                cout << hex << setfill('0') << setw(2) << (0xff & c) << " ";
                it->second.data.push_back(c);
            }
            cout << endl;
        }
    }
    return true;
}

bool LinkerWrapper::create_aggregate_content_of_sections_hex()
{
    exit(-1);
    return true;
}

bool LinkerWrapper::create_aggregate_content_of_sections()
{
    if (linkable_output == true)
        return create_aggregate_content_of_sections_linkable();
    else
        return create_aggregate_content_of_sections_hex();
}

bool LinkerWrapper::solve_relocations_on_data()
{
    // data can be accessed directly by indexing the array of data!
    // rel_data.offset always show on less byte of 2B address
    vector<int> relocation_indexes_for_removal;
    int index_of_relocation_data = 0;
    for (RelocationTable rel_data : output_relocation_table)
    {
        bool do_not_change_data = false;
        bool remove_local_pc_relative = false;
        //cout << dec;

        cout << rel_data.offset << "[" << rel_data.type << "," << rel_data.is_data << "]"
             << " <- " << rel_data.symbol_name << " (" << rel_data.section_name << ")" << endl;

        // fetch the value of symbol, to add it on the value for memory
        map<string, SectionTable>::iterator it = output_section_table.find(rel_data.symbol_name);
        int additional_value;
        if (it == output_section_table.end())
        {
            // symbol is the real symbol and it is in symbol table
            additional_value = output_symbol_table.find(rel_data.symbol_name)->second.value;
            cout << " symbol + " << additional_value << " ! ";
        }
        else
        {
            // symbol is the section (because it is local symbol in its asm file)
            // fetch symbol offset by file in helper data
            additional_value = section_additional_helper.find(rel_data.symbol_name)->second.find(rel_data.filename)->second.start_addres_in_aggregate_section;
            cout << " section + " << additional_value << " ! ";
        }

        // fetch the position of relocation data to substitute it from the value for memory
        int pc_rel_offset_data = 0;
        if (rel_data.type == "R_HYP_16_PC")
        {
            // BE VERY CAREULL!!!!!!!!!!!
            // CHECK THE OFFSET !!!

            if (output_symbol_table.find(rel_data.symbol_name)->second.section == rel_data.section_name)
            {
                // this relocation data will be deleted because this symbol will become local for the section
                // so displacement between offset and destination cannot be changed
                pc_rel_offset_data = rel_data.offset;
                if (rel_data.is_data == false)
                {
                    // this is instruction relocation data, so big endian is present
                    // need to be decremented because (-2) + offset - 1 + offset(symbol) will give exact address
                    pc_rel_offset_data -= 1;
                }
                cout << " PC REL (DELETE IT)- " << pc_rel_offset_data << " ! ";
                remove_local_pc_relative = true;
                relocation_indexes_for_removal.push_back(index_of_relocation_data);
            }
            else
            {
                if (linkable_output)
                {
                    // this relocation will remain the same
                    // because we do not know where the final address of the symbol in section will be
                    cout << " PC REL UNCHANGED  ! " << endl;
                    do_not_change_data = true;
                }
                else
                {
                    cout << "NOT COVERED YET" << endl;
                    exit(-1);
                }
            }
        }
        if (do_not_change_data)
        {
            index_of_relocation_data++;
            continue;
        }
        if (rel_data.is_data)
        {
            // this relocation data is about data where the endianinty is little
            // it is always about 2B address:
            // [L H] is the representation in memory
            int lower_value = output_section_table.find(rel_data.section_name)->second.data[rel_data.offset];
            int higher_value = output_section_table.find(rel_data.section_name)->second.data[rel_data.offset + 1];
            cout << higher_value << " " << lower_value << endl;
            int value = /*(0xffff &*/ (int)((higher_value << 8) + (0xff & lower_value));
            cout << " :" << value << " + " << additional_value << " - " << pc_rel_offset_data;
            value += additional_value - pc_rel_offset_data;
            cout << " -> " << value << endl;

            output_section_table[rel_data.section_name].data[rel_data.offset] = (0xff & (value));
            output_section_table[rel_data.section_name].data[rel_data.offset + 1] = (0xff & (value >> 8));
        }
        else
        {
            // this relocation data is about data where the endianinty is big
            // it is always about 2B address:
            // [H L] is the representation in memory
            int lower_value = output_section_table.find(rel_data.section_name)->second.data[rel_data.offset];
            int higher_value = output_section_table.find(rel_data.section_name)->second.data[rel_data.offset - 1];
            cout << higher_value << " " << lower_value << endl;
            int value = /*(0xffff &*/ (int)((higher_value << 8) + (0xff & lower_value));
            cout << " :" << value << " + " << additional_value << " - " << pc_rel_offset_data;
            value += additional_value - pc_rel_offset_data;
            cout << " -> " << value << endl;

            output_section_table[rel_data.section_name].data[rel_data.offset] = (0xff & (value));
            output_section_table[rel_data.section_name].data[rel_data.offset - 1] = (0xff & (value >> 8));
        }
        index_of_relocation_data++;
    }

    for (int index : relocation_indexes_for_removal)
    {
        output_relocation_table.erase(output_relocation_table.begin() + index);
    }

    return true;
}

// Down is output part
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

    for (map<string, map<string, SectionAdditionalData>>::iterator it_section = section_additional_helper.begin();
         it_section != section_additional_helper.end(); it_section++)
    {
        text_output_file << "Section: " << it_section->first << " :{" << endl;
        for (map<string, SectionAdditionalData>::iterator it_filename = it_section->second.begin();
             it_filename != it_section->second.end(); it_filename++)
        {
            text_output_file << "\t"
                             << "# " << it_filename->first;
            SectionAdditionalData additional_data = it_filename->second;

            text_output_file << "\t\t" << additional_data.section_name << "\t" << additional_data.parent_file_name;
            text_output_file << ":\t" << additional_data.parent_section_size;
            text_output_file << " [" << additional_data.start_addres_in_aggregate_section << ",";
            text_output_file << additional_data.start_addres_in_aggregate_section + additional_data.parent_section_size << ")";

            text_output_file << endl;
        }
        text_output_file << "\t}" << endl;
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
        text_output_file << "\t" << it->second.section << "\t\t" << it->second.name << "\t" << hex << setfill('0') << setw(4) << (0xffff & it->second.id_symbol) << endl;
    }

    text_output_file << dec;
    text_output_file << endl;
    text_output_file << "Relocation data" << endl;
    text_output_file << "Offset\tType\t\tDat/Ins\tSymbol\t\tSection name" << endl;

    for (RelocationTable rel_data : output_relocation_table)
    {
        text_output_file << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset);
        text_output_file << "\t" << rel_data.type << "\t" << (rel_data.is_data ? 'd' : 'i') << "\t";
        text_output_file << "\t" << rel_data.symbol_name << "\t\t" << rel_data.section_name << endl;
        text_output_file << "\t\t" << rel_data.filename << endl;
    }
    text_output_file << dec;
    text_output_file << endl;

    // Output section contents depends on option
    // if -linkable is set, output will be like assembly output
    // if -hex is set, output will be prepared for the memory
    if (linkable_output)
    {
        text_output_file << "Content:" << endl;
        for (map<string, SectionTable>::iterator it = output_section_table.begin(); it != output_section_table.end(); it++)
        {
            //if (it->first == "UNDEFINED")
            //    continue;
            //        if (it->first == "ABSOLUTE")
            //            continue;
            text_output_file << "Section: " << it->first << "(" << it->second.size << ")" << endl;
            if (it->second.size == 0)
            {
                continue;
            }
            SectionTable s_table = it->second;
            for (int i = 0; i < s_table.offsets.size() - 1; i++)
            {

                int current_offset = s_table.offsets[i];
                int next_offset = s_table.offsets[i + 1];
                text_output_file << hex << setfill('0') << setw(4) << (0xffff & current_offset) << ": ";
                for (int j = current_offset; j < next_offset; j++)
                {
                    char c = s_table.data[j];
                    text_output_file << hex << setfill('0') << setw(2) << (0xff & c) << " ";
                }
                text_output_file << endl;
            }
            // Last directive which is in memory
            int current_offset = s_table.offsets[s_table.offsets.size() - 1];
            int next_offset = s_table.data.size();
            text_output_file << hex << setfill('0') << setw(4) << (0xffff & current_offset) << ": ";
            for (int j = current_offset; j < next_offset; j++)
            {
                char c = s_table.data[j];
                text_output_file << hex << setfill('0') << setw(2) << (0xff & c) << " ";
            }
            text_output_file << endl;

            text_output_file << dec;
            text_output_file << endl;
        }
    }
}
