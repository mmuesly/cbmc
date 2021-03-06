/*******************************************************************\

Module: Generates string constraints for the Java format function

Author: Romain Brenguier

Date:   May 2017

\*******************************************************************/

/// \file
/// Generates string constraints for the Java format function

#include <iomanip>
#include <string>
#include <regex>
#include <vector>

#include <util/std_expr.h>
#include <util/unicode.h>

#include "string_constraint_generator.h"

// Format specifier describes how a value should be printed.
class string_constraint_generatort::format_specifiert
{
public:
  // Constants describing the meaning of characters in format specifiers.
  static const char DECIMAL_INTEGER          ='d';
  static const char OCTAL_INTEGER            ='o';
  static const char HEXADECIMAL_INTEGER      ='x';
  static const char HEXADECIMAL_INTEGER_UPPER='X';
  static const char SCIENTIFIC               ='e';
  static const char SCIENTIFIC_UPPER         ='E';
  static const char GENERAL                  ='g';
  static const char GENERAL_UPPER            ='G';
  static const char DECIMAL_FLOAT            ='f';
  static const char HEXADECIMAL_FLOAT        ='a';
  static const char HEXADECIMAL_FLOAT_UPPER  ='A';
  static const char CHARACTER                ='c';
  static const char CHARACTER_UPPER          ='C';
  static const char DATE_TIME                ='t';
  static const char DATE_TIME_UPPER          ='T';
  static const char BOOLEAN                  ='b';
  static const char BOOLEAN_UPPER            ='B';
  static const char STRING                   ='s';
  static const char STRING_UPPER             ='S';
  static const char HASHCODE                 ='h';
  static const char HASHCODE_UPPER           ='H';
  static const char LINE_SEPARATOR           ='n';
  static const char PERCENT_SIGN             ='%';

  int index=-1;
  std::string flag;
  int width;
  int precision;
  bool dt=false;
  char conversion;

  format_specifiert(
    int _index,
    std::string _flag,
    int _width,
    int _precision,
    bool _dt,
    char c):
      index(_index),
      flag(_flag),
      width(_width),
      precision(_precision),
      dt(_dt),
      conversion(c)
  { }
};

// Format text represent a constant part of a format string.
class format_textt
{
public:
  explicit format_textt(std::string _content): content(_content) { }

  format_textt(const format_textt &fs): content(fs.content) { }

  std::string get_content() const
  {
    return content;
  }

private:
  std::string content;
};

// A format element is either a specifier or text.
class format_elementt
{
public:
  typedef enum {SPECIFIER, TEXT} format_typet;

  explicit format_elementt(format_typet _type): type(_type), fstring("")
  {
  }

  explicit format_elementt(std::string s): type(TEXT), fstring(s)
  {
  }

  explicit format_elementt(string_constraint_generatort::format_specifiert fs):
    type(SPECIFIER),
    fstring("")
  {
    fspec.push_back(fs);
  }

  bool is_format_specifier() const
  {
    return type==SPECIFIER;
  }

  bool is_format_text() const
  {
    return type==TEXT;
  }

  string_constraint_generatort::format_specifiert get_format_specifier() const
  {
    PRECONDITION(is_format_specifier());
    return fspec.back();
  }

  format_textt &get_format_text()
  {
    PRECONDITION(is_format_text());
    return fstring;
  }

  const format_textt &get_format_text() const
  {
    PRECONDITION(is_format_text());
    return fstring;
  }

private:
  format_typet type;
  format_textt fstring;
  std::vector<string_constraint_generatort::format_specifiert> fspec;
};

#if 0
// This code is deactivated as it is not used for now, but ultimalety this
// should be used to throw an exception when the format string is not correct
/// Used to check first argument of `String.format` is correct.
/// \param s: a string
/// \return True if the argument is a correct format string. Any '%' found
///   between format specifier means the string is invalid.
static bool check_format_string(std::string s)
{
  std::string format_specifier=
      "%(\\d+\\$)?([-#+ 0,(\\<]*)?(\\d+)?(\\.\\d+)?([tT])?([a-zA-Z%])";
  std::regex regex(format_specifier);
  std::smatch match;

  while(std::regex_search(s, match, regex))
  {
    if(match.position()!= 0)
      for(const auto &c : match.str())
        if(c=='%')
          return false;
    s=match.suffix();
  }

  for(const auto &c : s)
    if(c=='%')
      return false;

  return true;
}
#endif

/// Helper function for parsing format strings.
/// This follows the implementation in openJDK of the java.util.Formatter class:
/// http://hg.openjdk.java.net/jdk7/jdk7/jdk/file/9b8c96f96a0f/src/share/classes/java/util/Formatter.java#l2660
/// \param m: a match in a regular expression
/// \return Format specifier represented by the matched string. The groups in
///   the match should represent: index, flag, width, precision, date and
///   conversion type.
static string_constraint_generatort::format_specifiert
  format_specifier_of_match(std::smatch &m)
{
  int index=m[1].str().empty()?-1:std::stoi(m[1].str());
  std::string flag=m[2].str().empty()?"":m[2].str();
  int width=m[3].str().empty()?-1:std::stoi(m[3].str());
  int precision=m[4].str().empty()?-1:std::stoi(m[4].str());
  std::string tT=m[5].str();

  bool dt=(tT!="");
  if(tT=="T")
    flag.push_back(
      string_constraint_generatort::format_specifiert::DATE_TIME_UPPER);

  INVARIANT(
    m[6].str().length()==1, "format conversion should be one character");
  char conversion=m[6].str()[0];

  return string_constraint_generatort::format_specifiert(
    index, flag, width, precision, dt, conversion);
}

/// Parse the given string into format specifiers and text.
/// This follows the implementation in openJDK of the java.util.Formatter class:
/// http://hg.openjdk.java.net/jdk7/jdk7/jdk/file/9b8c96f96a0f/src/share/classes/java/util/Formatter.java#l2513
/// \param s: a string
/// \return A vector of format_elementt.
static std::vector<format_elementt> parse_format_string(std::string s)
{
  std::string format_specifier=
    "%(\\d+\\$)?([-#+ 0,(\\<]*)?(\\d+)?(\\.\\d+)?([tT])?([a-zA-Z%])";
  std::regex regex(format_specifier);
  std::vector<format_elementt> al;
  std::smatch match;

  while(std::regex_search(s, match, regex))
  {
    if(match.position()!=0)
    {
      std::string pre_match=s.substr(0, match.position());
      al.emplace_back(pre_match);
    }

    al.emplace_back(format_specifier_of_match(match));
    s=match.suffix();
  }

  al.emplace_back(s);
  return al;
}

/// Helper for add_axioms_for_format_specifier
/// \param expr: a structured expression
/// \param component_name: name of the desired component
/// \return Expression in the component of `expr` named `component_name`.
static exprt get_component_in_struct(
  const struct_exprt &expr, irep_idt component_name)
{
  const struct_typet &type=to_struct_type(expr.type());
  std::size_t number=type.component_number(component_name);
  return expr.operands()[number];
}

/// Parse `s` and add axioms ensuring the output corresponds to the output of
/// String.format. Assumes the argument is a structured expression which
/// contains the fields: string expr, int, float, char, boolean, hashcode,
/// date_time. The correct component will be fetched depending on the format
/// specifier.
/// \param fs: a format specifier
/// \param arg: a struct containing the possible value of the argument to format
/// \param index_type: type for indexes in strings
/// \param char_type: type of characters
/// \return String expression representing the output of String.format.
array_string_exprt
string_constraint_generatort::add_axioms_for_format_specifier(
  const format_specifiert &fs,
  const struct_exprt &arg,
  const typet &index_type,
  const typet &char_type)
{
  const array_string_exprt res = fresh_string(index_type, char_type);
  exprt return_code;
  switch(fs.conversion)
  {
  case format_specifiert::DECIMAL_INTEGER:
    return_code =
      add_axioms_from_int(res, get_component_in_struct(arg, ID_int));
    return res;
  case format_specifiert::HEXADECIMAL_INTEGER:
    return_code =
      add_axioms_from_int_hex(res, get_component_in_struct(arg, ID_int));
    return res;
  case format_specifiert::SCIENTIFIC:
    add_axioms_from_float_scientific_notation(
      res, get_component_in_struct(arg, ID_float));
    return res;
  case format_specifiert::DECIMAL_FLOAT:
    add_axioms_for_string_of_float(res, get_component_in_struct(arg, ID_float));
    return res;
  case format_specifiert::CHARACTER:
    return_code =
      add_axioms_from_char(res, get_component_in_struct(arg, ID_char));
    return res;
  case format_specifiert::BOOLEAN:
    return_code =
      add_axioms_from_bool(res, get_component_in_struct(arg, ID_boolean));
    return res;
  case format_specifiert::STRING:
    return get_string_expr(get_component_in_struct(arg, "string_expr"));
  case format_specifiert::HASHCODE:
    return_code =
      add_axioms_from_int(res, get_component_in_struct(arg, "hashcode"));
    return res;
  case format_specifiert::LINE_SEPARATOR:
    // TODO: the constant should depend on the system: System.lineSeparator()
    return_code = add_axioms_for_constant(res, "\n");
    return res;
  case format_specifiert::PERCENT_SIGN:
    return_code = add_axioms_for_constant(res, "%");
    return res;
  case format_specifiert::SCIENTIFIC_UPPER:
  case format_specifiert::GENERAL_UPPER:
  case format_specifiert::HEXADECIMAL_FLOAT_UPPER:
  case format_specifiert::CHARACTER_UPPER:
  case format_specifiert::DATE_TIME_UPPER:
  case format_specifiert::BOOLEAN_UPPER:
  case format_specifiert::STRING_UPPER:
  case format_specifiert::HASHCODE_UPPER:
  {
    string_constraint_generatort::format_specifiert fs_lower=fs;
    fs_lower.conversion=tolower(fs.conversion);
    const array_string_exprt lower_case =
      add_axioms_for_format_specifier(fs_lower, arg, index_type, char_type);
    add_axioms_for_to_upper_case(res, lower_case);
    return res;
  }
  case format_specifiert::OCTAL_INTEGER:
  /// \todo Conversion of octal is not implemented.
  case format_specifiert::GENERAL:
  /// \todo Conversion for format specifier general is not implemented.
  case format_specifiert::HEXADECIMAL_FLOAT:
  /// \todo Conversion of hexadecimal float is not implemented.
  case format_specifiert::DATE_TIME:
    /// \todo Conversion of date-time is not implemented
    // For all these unimplemented cases we return a non-deterministic string
    message.warning() << "unimplemented format specifier: " << fs.conversion
                        << message.eom;
    return fresh_string(index_type, char_type);
  default:
    message.error() << "invalid format specifier: " << fs.conversion
                      << message.eom;
    INVARIANT(
      false, "format specifier must belong to [bBhHsScCdoxXeEfgGaAtT%n]");
    throw 0;
  }
}

/// Parse `s` and add axioms ensuring the output corresponds to the output of
/// String.format.
/// \param res: string expression for the result of the format function
/// \param s: a format string
/// \param args: a vector of arguments
/// \return code, 0 on success
exprt string_constraint_generatort::add_axioms_for_format(
  const array_string_exprt &res,
  const std::string &s,
  const exprt::operandst &args)
{
  const std::vector<format_elementt> format_strings=parse_format_string(s);
  std::vector<array_string_exprt> intermediary_strings;
  std::size_t arg_count=0;
  const typet &char_type = res.content().type().subtype();
  const typet &index_type = res.length().type();

  for(const format_elementt &fe : format_strings)
  {
    if(fe.is_format_specifier())
    {
      const format_specifiert &fs=fe.get_format_specifier();
      struct_exprt arg;
      if(fs.conversion!=format_specifiert::PERCENT_SIGN &&
         fs.conversion!=format_specifiert::LINE_SEPARATOR)
      {
        if(fs.index==-1)
        {
          INVARIANT(
            arg_count<args.size(), "number of format must match specifiers");
          arg=to_struct_expr(args[arg_count++]);
        }
        else
        {
          INVARIANT(fs.index > 0, "index in format should be positive");
          INVARIANT(
            static_cast<std::size_t>(fs.index)<=args.size(),
            "number of format must match specifiers");

          // first argument `args[0]` corresponds to index 1
          arg=to_struct_expr(args[fs.index-1]);
        }
      }
      intermediary_strings.push_back(
        add_axioms_for_format_specifier(fs, arg, index_type, char_type));
    }
    else
    {
      const array_string_exprt str = fresh_string(index_type, char_type);
      const exprt return_code =
        add_axioms_for_constant(str, fe.get_format_text().get_content());
      intermediary_strings.push_back(str);
    }
  }

  exprt return_code = from_integer(0, get_return_code_type());

  if(intermediary_strings.empty())
  {
    lemmas.push_back(equal_exprt(res.length(), from_integer(0, index_type)));
    return return_code;
  }

  array_string_exprt str = intermediary_strings[0];

  if(intermediary_strings.size() == 1)
  {
    // Copy the first string
    return add_axioms_for_substring(
      res, str, from_integer(0, index_type), str.length());
  }

  // start after the first string and stop before the last
  for(std::size_t i = 1; i < intermediary_strings.size() - 1; ++i)
  {
    const array_string_exprt &intermediary = intermediary_strings[i];
    const array_string_exprt fresh = fresh_string(index_type, char_type);
    return_code =
      bitor_exprt(return_code, add_axioms_for_concat(fresh, str, intermediary));
    str = fresh;
  }

  return bitor_exprt(
    return_code, add_axioms_for_concat(res, str, intermediary_strings.back()));
}

/// Construct a string from a constant array.
/// \param arr: an array expression containing only constants
/// \param length: an unsigned value representing the length of the array
/// \return String of length `length` represented by the array assuming each
///   field in `arr` represents a character.
std::string
utf16_constant_array_to_java(const array_exprt &arr, std::size_t length)
{
  std::wstring out(length, '?');
  unsigned int c;
  for(std::size_t i=0; i<arr.operands().size() && i<length; i++)
  {
    PRECONDITION(arr.operands()[i].id()==ID_constant);
    bool conversion_failed=to_unsigned_integer(
      to_constant_expr(arr.operands()[i]), c);
    INVARIANT(!conversion_failed, "constant should be convertible to unsigned");
    out[i]=c;
  }
  return utf16_native_endian_to_java(out);
}

/// Formatted string using a format string and list of arguments
///
/// Add axioms to specify the Java String.format function.
/// \todo This is correct only if the first argument (ie the format string) is
/// constant or does not contain format specifiers.
/// \param f: a function application
/// \return A string expression representing the return value of the
///   String.format function on the given arguments, assuming the first argument
///   in the function application is a constant. Otherwise the first argument is
///   returned.
exprt string_constraint_generatort::add_axioms_for_format(
  const function_application_exprt &f)
{
  PRECONDITION(f.arguments().size() >= 3);
  const array_string_exprt res =
    char_array_of_pointer(f.arguments()[1], f.arguments()[0]);
  const array_string_exprt s1 = get_string_expr(f.arguments()[2]);
  unsigned int length;

  if(s1.length().id()==ID_constant &&
     s1.content().id()==ID_array &&
     !to_unsigned_integer(to_constant_expr(s1.length()), length))
  {
    std::string s=utf16_constant_array_to_java(
      to_array_expr(s1.content()), length);
    // List of arguments after s
    std::vector<exprt> args(f.arguments().begin() + 3, f.arguments().end());
    return add_axioms_for_format(res, s, args);
  }
  else
  {
    message.warning()
      << "ignoring format function with non constant first argument"
      << message.eom;
    return from_integer(1, f.type());
  }
}
