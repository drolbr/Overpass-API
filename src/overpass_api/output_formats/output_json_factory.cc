
#include "../frontend/output_handler_parser.h"
#include "output_json.h"


class Output_JSON_Generator : public Output_Handler_Parser
{
public:
  Output_JSON_Generator() : Output_Handler_Parser("json") {}
  
  Output_Handler* new_output_handler(Tokenizer_Wrapper* token, Error_Output* error_output);  
  
  static Output_JSON_Generator singleton;
};


Output_JSON_Generator Output_JSON_Generator::singleton;


Output_Handler* Output_JSON_Generator::new_output_handler(Tokenizer_Wrapper* token, Error_Output* error_output)
{
  return new Output_JSON();
}
