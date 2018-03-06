
#include "../frontend/output_handler_parser.h"
#include "output_custom.h"


class Output_Custom_Generator : public Output_Handler_Parser
{
public:
  Output_Custom_Generator() : Output_Handler_Parser("custom") {}

  Output_Handler* new_output_handler(const std::map< std::string, std::string >& input_params,
      Tokenizer_Wrapper* token, Error_Output* error_output);

  static Output_Custom_Generator singleton;
};


Output_Custom_Generator Output_Custom_Generator::singleton;


Output_Handler* Output_Custom_Generator::new_output_handler(const std::map< std::string, std::string >& input_params,
							    Tokenizer_Wrapper* token, Error_Output* error_output)
{
  std::map< std::string, std::string >::const_iterator redirect_it = input_params.find("redirect");

  std::map< std::string, std::string >::const_iterator url_it = input_params.find("url");
  std::string url = (url_it == input_params.end() ?
      "http://www.openstreetmap.org/browse/{{{type}}}/{{{id}}}" : url_it->second);

  std::map< std::string, std::string >::const_iterator template_it = input_params.find("template");
  std::string template_name = (template_it == input_params.end() ? "default.wiki" : template_it->second);
  // sanity check for template_name
  if (template_name.find("/") != std::string::npos)
  {
    if (error_output)
      error_output->add_encoding_error("Parameter \"template\" must not contain slashes.");
    template_name = "";
  }

  return new Output_Custom(redirect_it == input_params.end() || redirect_it->second != "no",
      template_name, url);
}
