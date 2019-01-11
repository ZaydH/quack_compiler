//
// Created by Michal Young on 9/15/18.
//

#include "messages.h"
#include "location.hh"
#include "exceptions.h"

namespace report {

/* The error count is global */
  static int scanner_error_count = 0;           // How many errors so far? */
  static int parser_error_count = 0;           // How many errors so far? */
  const int error_limit = 5;           // Should be configurable

  void reset_error_count() {
    scanner_error_count = 0;
    parser_error_count = 0;
  }

  void bail() {
    std::string msg = "Too many errors, bailing";
    if (scanner_error_count > 0)
      throw ScannerException(msg);
    throw ParserException(msg);
  }

/* An error that we can locate in the input */
/* Note: An error message should look like this to work well
 * with IDEs and other tools:
 * /path/to/file:32:9: error: expression is not assignable
 */
  void error_at(const yy::location &loc, const std::string &msg) {
    unsigned begin_line = loc.begin.line;
    unsigned end_line = loc.end.line;
    std::cerr << msg << " at line " << begin_line << " column " << loc.begin.column;
    if (begin_line != end_line)
      std::cerr << " to line " << end_line << " column " << loc.end.column;
    else
      std::cerr << "-" << loc.end.column;
    std::cerr << std::endl;

    parser_error_count++;
    if ((++parser_error_count) + scanner_error_count > error_limit) {
      bail();
    }
  }

/* An error that we can't locate in the input */
  void error(const std::string &msg) {
    std::cerr << msg << std::endl;
    if (parser_error_count + (++scanner_error_count) > error_limit) {
      bail();
    }
  }

/* Additional diagnostic message, does not count against error limit */
  void note(const std::string &msg) {
    std::cerr << msg << std::endl;
  }

/* Are we ok? */
  bool ok() {
    return parser_error_count == 0 && scanner_error_count == 0;
  }

};
