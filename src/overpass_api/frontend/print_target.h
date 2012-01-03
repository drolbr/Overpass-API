#ifndef PRINT_TARGET
#define PRINT_TARGET

#include "../statements/print.h"

using namespace std;

class Output_Handle
{
  public:
    Output_Handle(string type_) : type(type_), mode(0), print_target(0) {}
    ~Output_Handle();
    
    Print_Target& get_print_target(uint32 mode, Transaction& transaction);
    
  private:
    string type;
    uint32 mode;
    Print_Target* print_target;
};

#endif
