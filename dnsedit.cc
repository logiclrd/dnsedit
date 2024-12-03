#define for if (false) ; else for

#ifndef WIN32
# define stricmp(a,b) strcasecmp((char *)(a), (char *)(b))
#endif

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

template <typename T>
class safe_vector : public vector<T>
{
  T _dummy;
public:
  T &dummy()
  {
    return _dummy;
  }

  T &operator [](size_t index)
  {
    if (index >= size())
      return _dummy;
    else
      return vector<T>::operator [](index);
  }

  const T &operator [](size_t index) const
  {
    if (index >= size())
      return _dummy;
    else
      return vector<T>::operator [](index);
  }
};

class safe_string : public string
{
public:
  char &operator [](size_t index)
  {
    if (index > size())
      throw "Crash";

    if (index == size())
      append(1, ' ');

    return string::operator [](index);
  }

  const char &operator [](size_t index) const
  {
    if (index >= size())
      throw "Crash";

    return string::operator [](index);
  }
};

string operator +(const string &left, int value)
{
  char buf[30];

  sprintf(buf, "%d", value);

  return left + buf;
}

string operator +(const string &left, size_t value)
{
  char buf[30];

  sprintf(buf, "%u", value);

  return left + buf;
}

class autoinit_bool
{
  bool _value;
public:
  autoinit_bool() { _value = false; }
  autoinit_bool(bool value) { _value = value; }
  operator bool &() { return _value; }
  operator bool() const { return _value; }
  autoinit_bool &operator =(bool value) { _value = value; return *this; }
  autoinit_bool &operator =(autoinit_bool value) { _value = value._value; return *this; }
};

void try_variable(const char **slot, char *name)
{
  char *value = getenv(name);

  if (value != NULL)
    *slot = value;
}

const char *request_method  = "GET";
const char *script_name     = "/cgi-bin/dnsedit.exe";
const char *query_string    = "";

void load_environment_variables()
{
  try_variable(&request_method, "REQUEST_METHOD");
  try_variable(&script_name,    "SCRIPT_NAME");
  try_variable(&query_string,   "QUERY_STRING");
}

class ConfigFileException
{
  char *message;
  int line, column;
  bool end_of_file;
  bool partial_record;

  void operator =(const ConfigFileException &other);
public:
  ConfigFileException(const char *message, int line, int column, bool end_of_file = false, bool partial_record = false)
  {
    this->message = strdup(message);
    this->line = line;
    this->column = column;
    this->end_of_file = end_of_file;
    this->partial_record = partial_record;
  }

  ConfigFileException(const ConfigFileException &other)
  {
    this->message = strdup(other.message);
    this->line = other.line;
    this->column = other.column;
    this->end_of_file = other.end_of_file;
    this->partial_record = partial_record;
  }

  ~ConfigFileException()
  {
    free(message);
  }

  string GetMessage() const
  {
    return message;
  }

  int GetLine() const
  {
    return line;
  }

  int GetColumn() const
  {
    return column;
  }

  bool EndOfFile() const
  {
    return end_of_file;
  }

  bool &PartialRecord()
  {
    return partial_record;
  }
};

class ConfigFileSource
{
  istream *source;
  int line, column;
public:
  ConfigFileSource(istream *source)
  {
    this->source = source;

    line = 1;
    column = 0;
  }

  void Throw(const char *message) const
  {
    throw ConfigFileException(message, line, column);
  }

  void Throw(const string &message) const
  {
    Throw(message.c_str());
  }

  void ThrowEOF(const char *message = "Unexpected end of file", bool partial = true) const
  {
    throw ConfigFileException(message, line, column, true, partial);
  }

  void ThrowEOF(const string &message, bool partial = true) const
  {
    ThrowEOF(message.c_str(), partial);
  }

  string ReadAtom()
  {
    #define ATOM_CHARSET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:1234567890.-[]"
    #define ATOM_START_CHARS 62

    string ret;
    int ch;

    ret.reserve(25);

    while ((ch = source->get()) != -1)
    {
      if (ch == '\n')
        line++, column = 1;
      else
        column++;

      if (isspace(ch))
        if (ret.size() == 0)
          continue;
        else
          break;

      if (ch == ';')
      {
        while ((ch = source->get()) != -1)
        {
          if (ch == '\n')
          {
            line++, column = 1;
            break;
          }
          else
            column++;
        }

        if ((ch < 0) || (ret.size() > 0))
          break;

        continue;
      }

      char *match = strchr(ATOM_CHARSET, ch);

      if (match == NULL)
      {
        if ((ret.size() == 0) && (ch == '@'))
          return "@";

        if ((ret.size() == 0) && (ch == '$'))
        {
          ret.append(1, (char)ch);
          continue;
        }

        Throw("Invalid character while parsing an ATOM");
      }

      int match_index = (int)(match - ATOM_CHARSET);

      int chars = sizeof(ATOM_CHARSET);

      if (ret.size() == 0)
        chars = ATOM_START_CHARS;

      if (match_index >= chars)
        Throw("Invalid character while parsing an ATOM");

      ret.append(1, (char)ch);
    }

    if (ret.size() == 0)
      ThrowEOF("End of file while parsing an ATOM", false);

    return ret;

    #undef ATOM_START_CHARS
    #undef ATOM_CHARSET
  }

  string ReadNumber()
  {
    // TODO: need more than positive integers?
    
    string ret;
    int ch;

    ret.reserve(10);

    while ((ch = source->get()) != -1)
    {
      if (ch == '\n')
        line++, column = 1;
      else
        column++;

      if (isspace(ch))
        if (ret.size() == 0)
          continue;
        else
          break;

      if (ch == ';')
      {
        while ((ch = source->get()) != -1)
        {
          if (ch == '\n')
          {
            line++, column = 1;
            break;
          }
          else
            column++;
        }

        if ((ch < 0) || (ret.size() > 0))
          break;

        continue;
      }

      if (!isdigit(ch))
        Throw("Invalid character while parsing a NUMBER");

      ret.append(1, (char)ch);
    }

    if (ret.size() == 0)
      ThrowEOF("End of file while parsing a NUMBER", false);

    return ret;
  }

  string ReadString()
  {
    string ret;
    int ch;

    ret.reserve(40);

    while ((ch = source->get()) != -1)
    {
      if (ch == '\n')
        line++, column = 1;
      else
        column++;

      if (isspace(ch))
        continue;

      if (ch == ';')
      {
        while ((ch = source->get()) != -1)
        {
          if (ch == '\n')
          {
            line++, column = 1;
            break;
          }
          else
            column++;
        }

        if (ch < 0)
          break;

        continue;
      }

      if (ch == '"')
        break;

      Throw("Invalid character while parsing a STRING");
    }

    if (ch < 0)
      ThrowEOF("End of file while parsing a STRING", false);

    while ((ch = source->get()) != -1)
    {
      if (ch == '\n')
        line++, column = 1;
      else
        column++;

      if (ch == ';')
      {
        while ((ch = source->get()) != -1)
        {
          if (ch == '\n')
          {
            line++, column = 1;
            break;
          }
          else
            column++;
        }

        if ((ch < 0) || (ret.size() > 0))
          break;

        continue;
      }

      if (ch == '"')
        break;

      ret.append(1, (char)ch);
    }

    if (ch != '"')
      ThrowEOF("End of file while parsing a STRING", true);

    return ret;
  }

  string ReadPunctuation()
  {
    #define PUNCTUATION_CHARSET "()"

    int ch;
    
    while ((ch = source->get()) != -1)
    {
      if (ch == '\n')
        line++, column = 1;
      else
        column++;

      if (ch == ';')
      {
        while ((ch = source->get()) != -1)
        {
          if (ch == '\n')
          {
            line++, column = 1;
            break;
          }
          else
            column++;
        }

        if (ch < 0)
          ThrowEOF("End of file while parsing PUNCTUATION", false);

        continue;
      }
                                    
      if (strchr(PUNCTUATION_CHARSET, ch) != NULL)
        break;

      if (!isspace(ch))
        Throw("Invalid character when parsing PUNCTUATION");
    }

    return string(1, ch);

    #undef PUNCTUATION_CHARSET
  }
};

struct ConfigFileRecordTemplate
{
  string NetworkClass, RecordType, Pattern;

  ConfigFileRecordTemplate(char *network_class, char *record_type, char *pattern)
    : NetworkClass(network_class),
      RecordType(record_type),
      Pattern(pattern)
  {
  }

  bool operator ==(const ConfigFileRecordTemplate &other) const
  {
    return (NetworkClass == other.NetworkClass)
        && (RecordType == other.RecordType)
        && (Pattern == other.Pattern);
  }

  string ClassAndType() const
  {
    return NetworkClass + ' ' + RecordType;
  }
};

#define ATOM            "A"
#define ATOM_CH         'A'
#define IP              "i"
#define IP_CH           'i'
#define IP6             "I"
#define IP6_CH          'I'
#define NUMBER          "N"
#define NUMBER_CH       'N'
#define STRING          "S"
#define STRING_CH       'S'
#define GROUP_START     "("
#define GROUP_START_CH  '('
#define GROUP_END       ")"
#define GROUP_END_CH    ')'
#define REMARK          "#"
#define REMARK_CH       '#'
#define NL              "\n"
#define NL_CH           '\n'

ConfigFileRecordTemplate END_OF_LIST("END", "OF", "LIST");

ConfigFileRecordTemplate grammar[] =
{
  ConfigFileRecordTemplate("IN", "A",       IP),
  ConfigFileRecordTemplate("IN", "AAAA",    IP6),
  ConfigFileRecordTemplate("IN", "CNAME",   ATOM),
  ConfigFileRecordTemplate("IN", "MX",      NUMBER ATOM),
  ConfigFileRecordTemplate("IN", "NS",      ATOM),
  ConfigFileRecordTemplate("IN", "SOA",     ATOM ATOM GROUP_START NL
                                              NUMBER REMARK "Serial number" NL
                                              NUMBER REMARK "Soft cache expiry" NL
                                              NUMBER REMARK "Failed cache refresh retry interval" NL
                                              NUMBER REMARK "Hard cache expiry" NL
                                              NUMBER REMARK "Minimum cached entry time-to-live" NL
                                            GROUP_END),
  ConfigFileRecordTemplate("IN", "TXT",     STRING),

  END_OF_LIST
};

string operator +(const char *left, const string &right)
{
  return string(left) + right;
}

bool is_valid_dns_label_character(char ch, int position, int field_size)
{
  if (isalnum(ch))
    return true;

  if (ch != '-')
    return false;

  return (position > 0) && (position + 1 < field_size);
}

bool validate_atom(const string &atom)
{
  if (atom == "@")
    return true;

  string::size_type last_dot = (string::size_type)-1;
  string::size_type dot = atom.find('.');

  while (dot != string::npos)
  {
    int field_size = (int)(dot - last_dot - 1);

    if ((field_size < 1) || (field_size > 63))
      return false;

    for (int i=0; i < field_size; i++)
      if (!is_valid_dns_label_character(atom[last_dot + i + 1], i, field_size))
        return false;

    last_dot = dot;
    dot = atom.find('.', dot + 1);
  }

  int last_field_size = (int)(atom.size() - last_dot - 1);

  if (last_field_size > 63)
    return false;

  for (int i=0; i < last_field_size; i++)
    if (!is_valid_dns_label_character(atom[last_dot + i + 1], i, last_field_size))
      return false;

  return true;
}

bool validate_ip(const string &address)
{
  int bytes_remaining = 3;
  int accumulator = 0;
  int digits = 0;

  for (string::size_type i=0; i < address.size(); i++)
  {
    int ch = address[i];

    if (ch == '.')
    {
      if (digits == 0)
        return false;

      bytes_remaining--;

      if (bytes_remaining < 0)
        return false;

      accumulator = 0;
      digits = 0;
    }

    if ((ch >= '0') && (ch <= '9'))
    {
      accumulator = (accumulator * 10) + (ch - '0');
      digits++;

      if (accumulator > 255)
        return false;
    }
  }

  if ((bytes_remaining > 0) || (digits == 0))
    return false;

  return true;
}

bool validate_ip6(const string &address)
{
  // TODO
  return false;
}

bool validate_number(const string &number)
{
  enum { Before, Number, After } position = Before;

  for (string::size_type i=0; i < number.size(); i++)
  {
    int ch = number[i];

    if (isspace(ch))
    {
      if (position == Number)
        position = After;
    }
    else if (isdigit(ch))
    {
      if (position == Before)
        position = Number;
      else if (position == After)
        return false;
    }
    else
      return false;
  }

  return (position >= Number);
}

struct EscapableCharacter
{
  int Character;
  char *Entity;
} escapable_characters[] =
{
  { '&', "&amp;" },
  { '<', "&lt;" },
  { '>', "&gt;" },
  { '"', "&quot;" },

  { 0, 0 }
};

string escape(string text)
{
  string::size_type index = 0;

  while (index < text.size())
  {
    string::size_type next_escape = string::npos;
    int escape_index = -1;

    for (int i=0; escapable_characters[i].Entity != NULL; i++)
    {
      string::size_type offset = text.find(escapable_characters[i].Character, index);

      if (offset < next_escape)
      {
        next_escape = offset;
        escape_index = i;
      }
    }

    if (escape_index < 0)
      break;

    text = text.substr(0, next_escape) + escapable_characters[escape_index].Entity + text.substr(next_escape + 1);
    index = next_escape + strlen(escapable_characters[escape_index].Entity);
  }

  return text;
}

struct ConfigFileRecord
{
  bool IsDirective;
  string Describee;
  autoinit_bool DescribeeError;
  safe_vector<string> Arguments;
  safe_vector<autoinit_bool> ArgumentError;
  ConfigFileRecordTemplate *Template;
  bool Transient, Old, Deleted;

  ConfigFileRecord()
  {
    Transient = false;
    Old = false;
    Deleted = false;
    IsDirective = false;
    Template = NULL;
  }

  bool HasErrors() const
  {
    if (DescribeeError)
      return true;

    for (size_t i=0; i < ArgumentError.size(); i++)
      if (ArgumentError[i])
        return true;

    return false;
  }

  int AddArgument(const string &value)
  {
    Arguments.push_back(value);
    ArgumentError.push_back(false);

    return (int)(Arguments.size() - 1);
  }

private:
  static string translate_quotes(string line)
  {
    for (size_t i=0; i < line.size(); i++)
    {
      if (isspace(line[i]) && (line[i] != '\t'))
        line[i] = ' ';
      else if (line[i] == '"')
      {
        line[i] = 0["'"];
        line.insert(i, "'");
      }
    }

    return line;
  }
public:

  string GenerateConfigFileRecord() const
  {
    if (Deleted)
      return "";

    stringstream ret;

    if (IsDirective)
    {
      ret << Describee << ' ' << Arguments[0] << endl;

      return ret.str();
    }

    ret << Describee << ' ' << Template->NetworkClass << ' ' << Template->RecordType;

    int indent_chars = (int)(ret.str().size() + 1);
    bool at_nl = false;

    size_t argument_index = 0;

    string &pattern = Template->Pattern;

    for (size_t pattern_index=0; pattern_index < pattern.size(); pattern_index++)
    {
      switch (pattern[pattern_index])
      {
        case REMARK_CH:
        {
          string::size_type remark_start = pattern_index + 1;
          string::size_type remark_end = remark_start;

          while ((remark_end < pattern.size()) &&  (pattern[remark_end] != NL_CH))
            remark_end++;

          if (at_nl)
            ret << string(indent_chars, ' ');
          else
            ret << ' ';

          ret << "; " << pattern.substr(remark_start, remark_end - remark_start) << endl;
          at_nl = true;

          pattern_index = remark_end;

          break;
        }
        case NL_CH:
        {
          ret << endl;
          at_nl = true;

          break;
        }

        case ATOM_CH:
        case IP_CH:
        case IP6_CH:
        case NUMBER_CH:
        {
          if (at_nl)
            ret << string(indent_chars, ' ');
          else
            ret << ' ';

          ret << Arguments[argument_index];
          at_nl = false;

          argument_index++;
          break;
        }
        case STRING_CH:
        {
          if (at_nl)
            ret << string(indent_chars, ' ');
          else
            ret << ' ';

          ret << '"' << translate_quotes(Arguments[argument_index]) << '"';
          at_nl = false;

          argument_index++;
          break;
        }
        case GROUP_START_CH:
        {
          if (at_nl)
            ret << string(indent_chars, ' ');
          else
            ret << ' ';

          ret << '(';
          at_nl = false;
          indent_chars += 2;

          break;
        }
        case GROUP_END_CH:
        {
          indent_chars -= 2;

          if (at_nl)
            ret << string(indent_chars, ' ');
          else
            ret << ' ';

          ret << ')';
          at_nl = false;

          break;
        }
      }
    }

    ret << endl;

    return ret.str();
  }

  string GetSignature() const
  {
    if (saved_signature.size() > 0)
      return saved_signature;

    stringstream sig;

    if (Template != NULL)
      sig << Template->ClassAndType() << ' ' << Describee;
    else

      sig << Describee;

    for (size_t i=0; i < Arguments.size(); i++)
      sig << ' ' << Arguments[i];

    return sig.str();
  }

  void LockSignature()
  {
    saved_signature = GetSignature();
  }

private:
  string saved_signature;

  static char * colour(bool error, bool deleted)
  {
    if (deleted)
      return " style=\"color: #d8d8d8;\"";
    else if (error)
      return " style=\"background: #ffd8d8;\"";
    else
      return "";
  }
public:

  string GenerateHTML(const string &row_id, const char *extra_td_property = "") const
  {
    bool suppress_nl = false;

    stringstream ret;

    ret << "<tr>" << endl
        << " <td valign=\"top\" align=\"center\"" << extra_td_property << ">" << endl;

    if (Transient)
      ret << "  <input type=\"hidden\" name=\"" << row_id << ".type\" value=\"" << Template->ClassAndType() << "\" />" << endl;
    else
      ret << "  <input type=\"hidden\" name=\"" << row_id << ".sig\" value=\"" << escape(GetSignature()) << "\" />" << endl;

    if ((!IsDirective) && (!Transient || Old))
      ret << "  <input type=\"checkbox\" name=\"" << row_id << ".delete\" " << (Deleted ? "checked " : "") << "/>" << endl;

    if (IsDirective)
      ret << " </td>" << endl
          << " <td valign\"top\"" << extra_td_property << ">" << endl
          << "  " << escape(Describee) << endl
          << " </td>" << endl;
    else
      ret << " </td>" << endl
          << " <td valign=\"top\"" << extra_td_property << ">" << endl
          << "  <input type=\"text\" name=\"" << row_id << ".describee\" value=\"" << escape(Describee) << "\"" << colour(DescribeeError, Deleted) << " />" << endl
          << " </td>" << endl;

    if (IsDirective)
    {
      ret << " <td valign=\"top\" colspan=\"2\"" << extra_td_property << "><hr /></td>" << endl
          << " <td valign=\"top\"" << extra_td_property << ">" << endl
          << "  <input type=\"text\" name=\"" << row_id << ".arg0\" value=\"" << escape(Arguments[0]) << "\"" << colour(ArgumentError[0], Deleted) << " />" << endl
          << " </td>" << endl
          << "</tr>" << endl;

      return ret.str();
    }

    ret << " <td valign=\"top\"" << extra_td_property << colour(false, Deleted) << ">" << Template->NetworkClass << "</td>" << endl
        << " <td valign=\"top\"" << extra_td_property << colour(false, Deleted) << ">" << Template->RecordType << "</td>" << endl
        << " <td valign=\"top\"" << extra_td_property << colour(false, Deleted) << ">" << endl;

    int argument_index = 0;

    string &pattern = Template->Pattern;

    for (string::size_type pattern_index = 0; pattern_index < pattern.size(); pattern_index++)
    {
      switch (pattern[pattern_index])
      {
        case REMARK_CH:
        {
          string::size_type remark_start = pattern_index + 1;
          string::size_type remark_end = remark_start;

          while ((remark_end < pattern.size()) &&  (pattern[remark_end] != NL_CH))
            remark_end++;

          ret << " &nbsp; ; " << escape(pattern.substr(remark_start, remark_end - remark_start)) << "<br />" << endl;

          pattern_index = remark_end;

          break;
        }
        case NL_CH:
        {
          if (suppress_nl)
            suppress_nl = false;
          else
            ret << "<br />" << endl;
          break;
        }

        case ATOM_CH:
        case IP_CH:
        case IP6_CH:
        case NUMBER_CH:
        case STRING_CH:
        {
          ret << "<input type=\"text\" name=\"" << row_id << ".arg" << argument_index << "\" value=\"" << escape(Arguments[argument_index]) << "\"" << colour(ArgumentError[argument_index], Deleted) << " />" << endl;
          argument_index++;
          break;
        }
        case GROUP_START_CH:
        {
          if (suppress_nl)
            suppress_nl = false;
          else
            ret << "<br />" << endl;

          ret << "(<br />" << endl
              << "<div style=\"margin-left: 2em\">" << endl;
          suppress_nl = true;
          break;
        }
        case GROUP_END_CH:
        {
          ret << "</div>" << endl
              << ")<br />" << endl;
        }
      }
    }

    ret << " </td>" << endl
        << "</tr>" << endl;

    return ret.str();
  }

  static ConfigFileRecord Parse(ConfigFileSource &source)
  {
    ConfigFileRecord ret;

    try
    {
      ret.Describee = source.ReadAtom();
    }
    catch (ConfigFileException ex)
    {
      if (ex.EndOfFile())
      {
        ret.Template = &END_OF_LIST;
        return ret;
      }
      else
        throw;
    }

    if (ret.Describee[0] == '$')
    {
      ret.IsDirective = true;

      ret.AddArgument(source.ReadAtom());

      return ret;
    }

    string network_class = source.ReadAtom();
    string record_type = source.ReadAtom();

    ConfigFileRecordTemplate *record_template = NULL;

    for (int i=0; grammar[i].NetworkClass != "END"; i++)
      if ((grammar[i].NetworkClass == network_class)
       && (grammar[i].RecordType == record_type))
      {
        record_template = &grammar[i];
        break;
      }

    if (record_template == NULL)
      source.Throw("Unrecognized zone file record");

    ret.Template = record_template;

    try
    {
      string &pattern = record_template->Pattern;

      for (string::size_type i=0; i < pattern.size(); i++)
      {
        switch (pattern[i])
        {
          case REMARK_CH:
          {
            while ((i < pattern.size()) &&  (pattern[i] != NL_CH))
              i++;
            break;
          }
          case NL_CH:
            break;

          case ATOM_CH:
          case IP_CH:
          case IP6_CH:
          {
            string atom = source.ReadAtom();

            switch (pattern[i])
            {
              case IP_CH:
              {
                if (!validate_ip(atom))
                  source.Throw("Syntax error in config file (expected an IP address, got: " + atom + ")");
                break;
              }
              case IP6_CH:
              {
                if (!validate_ip6(atom))
                  source.Throw("Syntax error in config file (expected an IPv6 address, got: " + atom + ")");
                break;
              }
            }

            ret.AddArgument(atom);
            break;
          }
          case NUMBER_CH:
          {
            ret.AddArgument(source.ReadNumber());
            break;
          }
          case STRING_CH:
          {
            ret.AddArgument(source.ReadString());
            break;
          }
          case GROUP_START_CH:
          case GROUP_END_CH:
          {
            string punctuation = source.ReadPunctuation();

            if (punctuation[0] != pattern[i])
              source.Throw("Syntax error in config file (expected '" + pattern.substr(i, 1) + "', got: '" + punctuation + "')");

            break;
          }
        }
      }
    }
    catch (ConfigFileException ex)
    {
      if (ex.EndOfFile())
        ex.PartialRecord() = true;
      throw;
    }

    return ret;
  }
};

struct ConfigFile
{
  safe_vector<ConfigFileRecord> Records;

  static ConfigFile Load(istream &stream)
  {
    if (!stream)
      throw ConfigFileException("Unable to open zone file", 0, 0);

    ConfigFileSource source(&stream);

    ConfigFile ret;

    while (true)
    {
      ConfigFileRecord record = ConfigFileRecord::Parse(source);

      if (record.Template == &END_OF_LIST)
        break;

      record.LockSignature();

      ret.Records.push_back(record);
    }

    return ret;
  }

  int Save(ostream &stream) const
  {
    for (size_t i=0; i < Records.size(); i++)
    {
      stream << Records[i].GenerateConfigFileRecord();
      if (!stream)
        return errno ? errno : 88; // EDOOFUS on FreeBSD :-) "Programming error"

      stream << endl;
      if (!stream)
        return errno ? errno : 88;
    }

    return 0;
  }

  bool HasErrors() const
  {
    for (size_t i=0; i < Records.size(); i++)
      if (Records[i].HasErrors())
        return true;

    return false;
  }
};

ConfigFileRecordTemplate *lookup_template(const string &network_class, const string &record_type)
{
  for (int i=0; grammar[i].NetworkClass != "END"; i++)
    if ((grammar[i].NetworkClass == network_class)
     && (grammar[i].RecordType == record_type))
      return &grammar[i];

  return NULL;
}

ConfigFileRecordTemplate *lookup_template(const string &class_and_type)
{
  string::size_type space_ofs = class_and_type.find(' ');

  if (space_ofs != string::npos)
  {
    string network_class = class_and_type.substr(0, space_ofs);
    string record_type = class_and_type.substr(space_ofs + 1);

    return lookup_template(network_class, record_type);
  }

  return NULL;
}

void display_zone(const ConfigFile &zone, bool add_failure)
{
  cout << "<html>" << endl
       << " <body>" << endl
       << "  <form action=\"" << escape(script_name) << "\" method=\"POST\">" << endl
       << "   <table border=\"1\">" << endl
       << "    <tr>" << endl
       << "     <td><i>Delete?</i></td>" << endl
       << "     <td><i>Name</i></td>" << endl
       << "     <td><i>Class</i></td>" << endl
       << "     <td><i>Type</i></td>" << endl
       << "     <td><i>Data</i></td>" << endl
       << "    </tr>" << endl;

  for (size_t i=0, transient_count=0; i < zone.Records.size(); i++)
  {
    string row_id;

    if (zone.Records[i].Transient)
    {
      row_id = string("transient") + (transient_count++);

      if (zone.Records[i].Old)
        cout << zone.Records[i].GenerateHTML(row_id, " bgcolor=\"#d0ffff\"");
      else
        cout << zone.Records[i].GenerateHTML(row_id, " bgcolor=\"#d0ffd0\"");
    }
    else
    {
      row_id = string("row") + i;

      cout << zone.Records[i].GenerateHTML(row_id);
    }
  }

  if (add_failure)
    cout << "    <tr><td bgcolor=\"#ffd0d0\" colspan=\"5\"><i>Couldn't create new record template</i></td></tr>" << endl;

  cout << "   </table>" << endl
       << "   <br />" << endl
       << "   <select name=\"new_record_type\">" << endl;

  for (int i=0; grammar[i].NetworkClass != "END"; i++)
    cout << "    <option value=\"" << grammar[i].NetworkClass << " " << grammar[i].RecordType << "\">"
                                  << grammar[i].NetworkClass << " " << grammar[i].RecordType << "</option>" << endl;

  cout << "   </select>" << endl
       << "   <input type=\"submit\" name=\"add\" value=\"Add Record\" />" << endl
       << "   <input type=\"submit\" name=\"revert\" value=\"Revert To Last Successful Update\" />" << endl
       << "   <input type=\"submit\" name=\"update\" value=\"Update File\" />" << endl
       << "   <input type=\"submit\" name=\"refresh\" value=\"Refresh DNS Server\" />" << endl
       << "  </form>" << endl
       << " </body>" << endl
       << "</html>" << endl;
}

void emit_zone(const ConfigFile &zone, const string &name)
{
  ofstream out(name.c_str());

  for (size_t i=0; i < zone.Records.size(); i++)
    if (zone.Records[i].Transient == false)
      out << zone.Records[i].GenerateConfigFileRecord();
}

class CGIParameters
{
  map<string, string> params;

  static int dehex(int nybble)
  {
    if ((nybble >= '0') && (nybble <= '9'))
      return (nybble - '0');
    if ((nybble >= 'A') && (nybble <= 'F'))
      return (nybble - 'A') + 10;
    if ((nybble >= 'a') && (nybble <= 'f'))
      return (nybble - 'a') + 10;

    return -1;
  }

  static int dehex(int high_nybble, int low_nybble)
  {
    return (dehex(high_nybble) << 4) | dehex(low_nybble);
  }

  void load_parameters_from_stream(istream &source)
  {
    safe_string name, value;
    size_t name_len, value_len;
    bool have_divider = false;

    name_len = value_len = 0;

    while (true)
    {
      int ch = source.get();

      if (!source)
        break;

      if (ch == '+')
        ch = ' ';
      if (ch == '%')
      {
        int high_nybble, low_nybble;

        high_nybble = source.get();
        low_nybble = source.get();

        ch = dehex(high_nybble, low_nybble);
      }

      if (ch < 0)
        continue;

      if (ch == '&')
      {
        params[name.substr(0, name_len)] = value.substr(0, value_len);

        name_len = 0;
        value_len = 0;
        have_divider = false;
        continue;
      }

      if (!have_divider)
      {
        if (ch == '=')
        {
          have_divider = true;
          continue;
        }

        name[name_len++] = ch;
      }
      else
        value[value_len++] = ch;
    }

    if (name_len)
      params[name.substr(0, name_len)] = value.substr(0, value_len);
  }

  void load_query_string_parameters()
  {
    if ((query_string != NULL) && (query_string[0] != '\0'))
    {
      stringstream stream(query_string);

      load_parameters_from_stream(stream);
    }
  }

  void load_stdin_parameters()
  {
    if (0 == stricmp(request_method, "POST"))
      load_parameters_from_stream(cin);
  }

  void load_cgi_parameters()
  {
    load_query_string_parameters();
    load_stdin_parameters();
  }
public:
  CGIParameters()
  {
    load_cgi_parameters();
  }

  bool ParameterExists(const string &name) const
  {
    typedef map<string, string>::const_iterator iter;

    iter location = params.find(name);

    return (location != params.end());
  }

  string GetParameter(const string &name) const
  {
    static string empty;

    typedef map<string, string>::const_iterator iter;

    iter location = params.find(name);

    if (location != params.end())
      return location->second;
    else
      return empty;
  }
};

void apply_edits(const CGIParameters &cgi_params, ConfigFile &zone)
{
  map<string, int> records_by_sig;

  typedef map<string, int>::iterator iter;

  for (size_t i=0; i < zone.Records.size(); i++)
    records_by_sig[zone.Records[i].GetSignature()] = (int)i;

  vector<int> to_delete;

  for (int i=0; true; i++)
  {
    string row_id = string("row") + i;

    string sig_field = row_id + ".sig";
    string delete_field = row_id + ".delete";

    if (!cgi_params.ParameterExists(sig_field))
      break;

    bool delete_record = cgi_params.ParameterExists(delete_field);

    iter have_record = records_by_sig.find(cgi_params.GetParameter(sig_field));

    if (have_record == records_by_sig.end())
    {
      if (delete_record)
        continue; // record didn't exist, but the user doesn't want it anyway :-)

      // TODO: something more appropriate, perhaps?
      cout << "<font color=\"red\">ERROR: Stale form data detected!</font>" << endl;
      exit(0);
    }
    else
    {
      int index = have_record->second;

      ConfigFileRecord &record = zone.Records[index];

      if (delete_record)
      {
        if (record.Transient)
          to_delete.push_back(index);
        else
          record.Deleted = true;
      }
      else
      {
        ConfigFileRecordTemplate *record_template = record.Template;

        if (record_template == NULL)
        {
          // for now, directive-type records can have only a single numeric parameter
          string field_name = row_id + ".arg0";

          if (!cgi_params.ParameterExists(field_name))
          {
            // TODO: something more appropriate, perhaps?
            cout << "<font color=\"red\">ERROR: Unable to locate data for argument of row " << row_id << "!</font>" << endl;
            exit(0);
          }

          string new_value = cgi_params.GetParameter(field_name);

          record.Arguments[0] = new_value;
          if (!validate_number(new_value))
            record.ArgumentError[0] = true;
        }
        else
        {
          string &pattern = record_template->Pattern;

          int argument_index = 0;

          for (string::size_type pattern_index = 0; pattern_index < pattern.size(); pattern_index++)
          {
            switch (pattern[pattern_index])
            {
              case REMARK_CH:
              {
                string::size_type remark_end = pattern_index + 1;

                while ((remark_end < pattern.size()) &&  (pattern[remark_end] != NL_CH))
                  remark_end++;

                pattern_index = remark_end;

                break;
              }

              case ATOM_CH:
              case IP_CH:
              case IP6_CH:
              case NUMBER_CH:
              case STRING_CH:
              {
                int this_argument_index = argument_index++;

                string field_name = row_id + ".arg" + this_argument_index;

                if (!cgi_params.ParameterExists(field_name))
                {
                  // TODO: something more appropriate, perhaps?
                  cout << "<font color=\"red\">ERROR: Unable to locate data for argument " << this_argument_index << " of row " << row_id << "!</font>" << endl;
                  exit(0);
                }

                string new_value = cgi_params.GetParameter(field_name);

                bool good = false;

                switch (pattern[pattern_index])
                {
                  case ATOM_CH:   good = validate_atom(new_value);    break;
                  case IP_CH:     good = validate_ip(new_value);      break;
                  case IP6_CH:    good = validate_ip6(new_value);     break;
                  case NUMBER_CH: good = validate_number(new_value);  break;
                  case STRING_CH: good = true;                        break;
                }

                record.Arguments[this_argument_index] = new_value;
                if (!good)
                  record.ArgumentError[this_argument_index] = true;

                break;
              }
            }
          }
        }
      }
    }
  }

  sort(to_delete.begin(), to_delete.end());

  vector<int>::reverse_iterator i;
  for (i = to_delete.rbegin(); i != to_delete.rend(); i++)
    zone.Records.erase(zone.Records.begin() + *i);
}

void collect_transient_row(const CGIParameters &cgi_params, ConfigFile &zone, const string &row_id)
{
  string transient_row_class_and_type = cgi_params.GetParameter(row_id + ".type");

  ConfigFileRecordTemplate *record_template = lookup_template(transient_row_class_and_type);

  if (record_template != NULL)
  {
    ConfigFileRecord transient_row;

    transient_row.Template = record_template;
    transient_row.Transient = true;
    transient_row.Old = true; // it has made the round-trip :-)

    transient_row.Describee = cgi_params.GetParameter(row_id + ".describee");

    if (!validate_atom(transient_row.Describee))
      transient_row.DescribeeError = true;

    string &pattern = record_template->Pattern;

    for (size_t pattern_index = 0; pattern_index < pattern.size(); pattern_index++)
    {
      switch (pattern[pattern_index])
      {
        case REMARK_CH:
        {
          string::size_type remark_end = pattern_index + 1;

          while ((remark_end < pattern.size()) &&  (pattern[remark_end] != NL_CH))
            remark_end++;

          pattern_index = remark_end;

          break;
        }

        case ATOM_CH:
        case IP_CH:
        case IP6_CH:
        case NUMBER_CH:
        case STRING_CH:
        {
          int this_argument_index = transient_row.AddArgument("");

          string field_name = row_id + ".arg" + this_argument_index;

          if (!cgi_params.ParameterExists(field_name))
          {
            // TODO: something more appropriate, perhaps?
            cout << "<font color=\"red\">ERROR: Unable to locate data for argument " << this_argument_index << " of row " << row_id << "!</font>" << endl;
            exit(0);
          }

          string new_value = cgi_params.GetParameter(field_name);

          bool good = false;

          switch (pattern[pattern_index])
          {
            case ATOM_CH:   good = validate_atom(new_value);    break;
            case IP_CH:     good = validate_ip(new_value);      break;
            case IP6_CH:    good = validate_ip6(new_value);     break;
            case NUMBER_CH: good = validate_number(new_value);  break;
            case STRING_CH: good = true;                        break;
          }

          transient_row.Arguments[this_argument_index] = new_value;
          if (!good)
            transient_row.ArgumentError[this_argument_index] = true;

          break;
        }
      }
    }

    zone.Records.push_back(transient_row);
  }
}

void collect_transient_rows(const CGIParameters &cgi_params, ConfigFile &zone)
{
  // for each transient row, read in the values;
  // if values are complete and acceptable, commit
  // the new row, otherwise turn it into an old
  // transient row

  for (int i=0; true; i++)
  {
    string field_name = string("transient") + i;

    if (!cgi_params.ParameterExists(field_name + ".type"))
      break;

    collect_transient_row(cgi_params, zone, field_name);
  }
}

int find_serial(ConfigFile &zone, int validate_against = -1)
{
  for (size_t i=0; i < zone.Records.size(); i++)
  {
    ConfigFileRecord &record = zone.Records[i];

    if ((record.Template != NULL)
     && (record.Template->ClassAndType() == "IN SOA")
     && (record.Arguments.size() >= 2))
    {
      int ret = atoi(record.Arguments[2].c_str());

      if (ret <= validate_against)
        record.ArgumentError[2] = true;

      return ret;
    }
  }

  return -1;
}

#ifdef WIN32
const char *zonefile = "zonefile2";
#else
const char *zonefile = "/etc/namedb/elliana/bind9.pepperell.net";
#endif // WIN32

int main()
{
  load_environment_variables();

  CGIParameters cgi_params;

  cout << "Status: 200 OK" << endl
       << "Content-Type: text/html" << endl
       << "Cache-Control: no-store max-age=30" << endl
       << endl;

  ConfigFile zone;

  try
  {
    ifstream stream(zonefile);

    zone = ConfigFile::Load(stream);
  }
  catch (ConfigFileException ex)
  {
    cout << "<html>" << endl
         << " <body>" << endl
         << "  <font color=\"red\">" << endl
         << "   Error loading zone file!<br />" << endl
         << "   <br />" << endl
         << "   Reason: " << escape(ex.GetMessage()) << "<br />" << endl
         << "   Line: " << ex.GetLine() << " &nbsp; Column: " << ex.GetColumn() << "<br />" << endl
         << "  </font>" << endl
         << " </body>" << endl
         << "</html>" << endl;

    return 0;
  }

  int old_serial = find_serial(zone);

  if (cgi_params.ParameterExists("revert"))
    cout << "  <font color=\"blue\">" << endl
         << "   Changes to the zone file dropped. Reloading... done!" << endl
         << "  </font>" << endl;
  else
  {
    apply_edits(cgi_params, zone);
    collect_transient_rows(cgi_params, zone);
  }

  bool add_failure = false;

  if (cgi_params.ParameterExists("add"))
  {
    ConfigFileRecord new_row;

    new_row.Template = lookup_template(cgi_params.GetParameter("new_record_type"));

    if (new_row.Template != NULL)
    {
      new_row.Transient = true;
      zone.Records.push_back(new_row);
    }
    else
      add_failure = true;
  }
  else if (cgi_params.ParameterExists("update"))
  {
    if (zone.HasErrors())
      cout << "  <font color=\"red\">" << endl
           << "   There are errors in this zone configuration. Please correct them before updating the file." << endl
           << "  </font>" << endl;
    else
    {
      int new_serial = find_serial(zone, old_serial);

      if (new_serial == -1)
        cout << "  <font color=\"red\">" << endl
             << "   The zone does not contain a Start-Of-Authority (SOA) record with a valid serial number. Please add one before continuing." << endl
             << "  </font>" << endl;
      else if (new_serial < old_serial)
        cout << "  <font color=\"red\">" << endl
             << "   The zone's serial number is invalid, as it is lower in value than a previous serial number. Please correct this before continuing." << endl
             << "  </font>" << endl;
      else if (new_serial == old_serial)
        cout << "  <font color=\"red\">" << endl
             << "   The zone's serial number must be updated to a larger value before proceeding." << endl
             << "  </font>" << endl;
      else
      {
        ofstream stream(zonefile);

        int error_code = zone.Save(stream);

        stream.close();

        if (error_code == 0)
        {
          cout << "  <font color=\"blue\">" << endl
               << "   Zone file successfully written out. Reloading...";

          ifstream stream(zonefile);

          zone = ConfigFile::Load(stream);

          cout <<                                                    " done!" << endl
               << "  </font>" << endl;
        }
        else
          cout << "  <font color=\"red\">" << endl
               << "   Error writing zone file! " << strerror(error_code) << endl
               << "  </font>" << endl;
      }
    }

    cout << "<br /><br />" << endl;
  }
  else if (cgi_params.ParameterExists("refresh"))
  {
#ifndef WIN32
    FILE *pipe;
    int ch;
    
    chdir("/etc/namedb/elliana");
#endif

    cout << "Reload:<br />" << endl
         << "<br />" << endl
         << "<pre style=\"border: 1px solid black; background: #c0c0c0;\">" << endl;

#ifdef WIN32
    cout << "(not implemented for win32)" << endl;
#else
    int char_count = 0;

    pipe = popen("/usr/local/sbin/rndc -V reload 2>&1 | grep -Ev '(creat|setting|channel|post|connect|message|recv)'", "r");
    while ((ch = getc(pipe)) >= 0)
    {
      switch (ch)
      {
        case '&': cout << "&amp;"; break;
        case '<': cout << "&lt;"; break;
        case '>': cout << "&gt;"; break;
        case '"': cout << "&quot;"; break;

        default: cout << ((char)ch); break;
      }
      char_count++;
    }
    pclose(pipe);

    if (char_count == 0)
      cout << "(no output -- this is normal)" << endl;
#endif // WIN32

    cout << "</pre>" << endl
         << "<br />" << endl
         << "Version control check-in:<br />" << endl
         << "<br />" << endl
         << "<pre style=\"border: 1px solid black; background: #c0c0c0;\">" << endl;

#ifdef WIN32
    cout << "(not implemented for win32)" << endl;
#else
    putenv("HOME=/root");
    putenv("USER=root");

    setuid(0);

    pipe = popen("/usr/local/bin/svn ci -m 'Auto-commit by dnsedit' --non-interactive --username logiclrd 2>&1", "r");
    while ((ch = getc(pipe)) >= 0)
    {
      switch (ch)
      {
        case '&': cout << "&amp;"; break;
        case '<': cout << "&lt;"; break;
        case '>': cout << "&gt;"; break;
        case '"': cout << "&quot;"; break;

        default: cout << ((char)ch); break;
      }
    }
    pclose(pipe);
#endif // WIN32

    cout << "</pre>" << endl
         << "<br /><br />" << endl;
  }

  display_zone(zone, add_failure);
}
