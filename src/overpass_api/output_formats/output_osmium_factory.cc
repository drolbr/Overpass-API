
#include "../frontend/output_handler_parser.h"
#include "output_osmium.h"



class Output_Osmium_PBF_Generator : public Output_Handler_Parser
{
public:
  Output_Osmium_PBF_Generator() : Output_Handler_Parser("pbf") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_Osmium_PBF_Generator singleton;
};


Output_Osmium_PBF_Generator Output_Osmium_PBF_Generator::singleton;


Output_Handler* Output_Osmium_PBF_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
                                                         Tokenizer_Wrapper* token, Error_Output* error_output)
{
  return new Output_Osmium("pbf");
}


class Output_Osmium_OPL_Generator : public Output_Handler_Parser
{
public:
  Output_Osmium_OPL_Generator() : Output_Handler_Parser("opl") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_Osmium_OPL_Generator singleton;
};


Output_Osmium_OPL_Generator Output_Osmium_OPL_Generator::singleton;


Output_Handler* Output_Osmium_OPL_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
                                                         Tokenizer_Wrapper* token, Error_Output* error_output)
{
  return new Output_Osmium("opl");
}
