
#include "../frontend/output_handler_parser.h"
#include "output_custom.h"


class Output_Custom_Generator : public Output_Handler_Parser
{
public:
  Output_Custom_Generator() : Output_Handler_Parser("custom") {}
  
  Output_Handler* new_output_handler(Tokenizer_Wrapper* token, Error_Output* error_output);  
  
  static Output_Custom_Generator singleton;
};


Output_Custom_Generator Output_Custom_Generator::singleton;


Output_Handler* Output_Custom_Generator::new_output_handler(Tokenizer_Wrapper* token, Error_Output* error_output)
{
  return new Output_Custom();
}
