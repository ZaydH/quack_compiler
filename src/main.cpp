//
// Driver for scanner --- print each token
//

// #include <stdlib.h>
#include <unistd.h>  // getopt is here

#include <map>
#include <fstream>
#include <iostream>

#include "quack_compiler.h"

/* Hack alert:
 * The right way to build the following table is to write a little
 * script in awk, python, or even perl (ick) to extract it from
 * the enum in quack.tab.hxx.  That would be reusable across projects
 * and robust as the parser changes, whereas this quick hack will
 * surely break if I make small changes to the grammar.  But this
 * took me 10 minutes, and the script might take an hour to write
 * and debug, so I'm doing it this way to get the assignment out ASAP.
 */

static std::map<int, std::string> token_names =
    {{-1,  "EOF"},
     {258, "CLASS"},
     {259, "DEF"},
     {260, "EXTENDS"},
     {261, "IF"},
     {262, "ELIF"},
     {263, "ELSE"},
     {264, "WHILE"},
     {265, "RETURN"},
     {266, "TYPECASE"},
     {267, "ATLEAST"},
     {268, "ATMOST"},
     {269, "EQUALS"},
     {270, "AND"},
     {271, "OR"},
     {272, "NOT"},
     {273, "IDENT"},
     {274, "INT_LIT"},
     {275, "STRING_LIT"},
     {276, "NEG"}
    };

std::string token_name(int token) {
  if (token < 255) {
    return std::string(1, (char) token);
  } else {
    return token_names[token];
    // And presumably throw an exception if it's not defined.
  }
}

int main(int argc, char **argv) {

  Quack::Compiler compiler;
  compiler.parse_args(argc, argv);
  compiler.run();

}
