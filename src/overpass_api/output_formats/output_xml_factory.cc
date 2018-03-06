
#include "../frontend/output_handler_parser.h"
#include "output_xml.h"


class Output_XML_Generator : public Output_Handler_Parser
{
public:
  Output_XML_Generator() : Output_Handler_Parser("xml") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_XML_Generator singleton;
};


Output_XML_Generator Output_XML_Generator::singleton;


Output_Handler* Output_XML_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
							 Tokenizer_Wrapper* token, Error_Output* error_output)
{
  return new Output_XML();
}
