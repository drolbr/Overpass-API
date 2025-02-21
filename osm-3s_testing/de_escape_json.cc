
#include "../src/overpass_api/frontend/decode_text.h"

#include <iostream>


int main(int argc, char* args[])
{
  for (std::string buf; std::getline(std::cin, buf); )
    std::cout<<decode_json(buf, nullptr, 0)<<'\n';
  
  return 0;
}
