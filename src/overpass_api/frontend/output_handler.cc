
#include "output_handler.h"


const std::string& member_type_name(uint32 type)
{
  static std::vector< std::string > MEMBER_TYPE;
  if (MEMBER_TYPE.empty())
  {
    MEMBER_TYPE.push_back("invalid");
    MEMBER_TYPE.push_back("node");
    MEMBER_TYPE.push_back("way");
    MEMBER_TYPE.push_back("relation");
  }

  if (type < MEMBER_TYPE.size())
    return MEMBER_TYPE[type];
  else
    return MEMBER_TYPE[0];
}
