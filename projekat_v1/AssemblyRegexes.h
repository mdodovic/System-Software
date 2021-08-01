#ifndef ASSEMBLY_REGEXES_H
#define ASSEMBLY_REGEXES_H
#include <regex>
#include <string>
using namespace std;

regex any_symbol("(.*)");

// regexes for cleaning input file
regex rx_remove_comments("([^#]*)#.*");
regex rx_find_extra_spaces(" {2,}");
regex rx_find_tabs("\\t");
//regex rx_find_empty_strings(""); // this lines will be skipped manually
regex rx_remove_boundary_spaces("^( *)([^ ].*[^ ])( *)$");
regex rx_find_comma_spaces(" ?, ?");
regex rx_find_columns_spaces(" ?: ?");

// Odavde treba proveriti nadole:

// helper strings
string sh_literal_decimal = "-?[0-9]+";     // this is a decimal number (positive or negative)
string sh_literal_hexadecimal = "0x[0-9]+"; // this is a hexadecimal number
string sh_symbol = "[a-zA-Z][a-zA-Z0-9_]*"; // symbol can start only with letter and can contain letters, digits and _
string sh_register_range = "[0-7]";         // there are 8 registers: r0 - r7

// combined helper strings
string sh_symbol_or_literal = sh_symbol + "|" + sh_literal_decimal + "|" + sh_literal_hexadecimal;

// regexes for assembly directives
regex rx_global_directive("^\\.global (" + sh_symbol + "(," + sh_symbol + ")*)$");
regex rx_extern_directive("^\\.extern (" + sh_symbol + "(," + sh_symbol + ")*)$");
regex rx_section_directive("^\\.section (" + sh_symbol + ")$");
regex rx_word_directive("^\\.word (" + sh_symbol_or_literal + "(," + sh_symbol_or_literal + ")*)$");
regex rx_skip_directive("^\\.skip (" + sh_literal_decimal + "|" + sh_literal_hexadecimal + ")$");
regex rx_equ_directive("^\\.equ (" + sh_symbol + "),(" + sh_literal_decimal + "|" + sh_literal_hexadecimal + ")$");
regex rx_end_directive("^\\.end$");

// labels are in form -> 'symbol:'
// label has to be at the beginning of the line
// if there is not anything, label is associated with next command
regex rx_label_only("^(" + sh_symbol + "):$");             // nothing is after label
regex rx_label_with_command("^(" + sh_symbol + "):(.*)$"); // something is after label

#endif //ASSEMBLY_REGEXES_H