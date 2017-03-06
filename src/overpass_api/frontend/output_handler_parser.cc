
#include "output_handler_parser.h"


Output_Handler_Parser::Output_Handler_Parser(const std::string& format_name)
{
  registry()[format_name] = this;
}


Output_Handler_Parser* Output_Handler_Parser::get_format_parser(const std::string& format_name)
{
  std::map< std::string, Output_Handler_Parser* >::iterator
      it = registry().find(format_name);
  if (it == registry().end())
    return 0;
  else
    return it->second;
}


std::map< std::string, Output_Handler_Parser* >& Output_Handler_Parser::registry()
{
  static std::map< std::string, Output_Handler_Parser* > singleton;
  return singleton;
}
