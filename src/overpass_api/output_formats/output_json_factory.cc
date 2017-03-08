
#include "../frontend/output_handler_parser.h"
#include "output_json.h"


class Output_JSON_Generator : public Output_Handler_Parser
{
public:
  Output_JSON_Generator() : Output_Handler_Parser("json") {}
  
  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);  
  
  static Output_JSON_Generator singleton;
};


Output_JSON_Generator Output_JSON_Generator::singleton;


Output_Handler* Output_JSON_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
							  Tokenizer_Wrapper* token, Error_Output* error_output)
{
  std::map< std::string, std::string >::const_iterator jsonp_it = input_params.find("jsonp");
  std::string jsonp = (jsonp_it == input_params.end() ? "" : jsonp_it->second);
  
  // sanity check for jsonp
  for (std::string::size_type i = 0; i < jsonp.size(); ++i)
  {
    if (!isalnum(jsonp[i]) && !(jsonp[i] == '_') && !(jsonp[i] == '.'))
    {
      error_output->add_encoding_error("Parameter \"jsonp\" must contain only letters, digits, or the underscore.");
      jsonp = "";
    }
  }
    
  return new Output_JSON(jsonp);
}
