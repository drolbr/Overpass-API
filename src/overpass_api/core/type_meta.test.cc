#include "datatypes.h"
#include "type_meta.h"

#include <iostream>
#include <vector>


void show_hex(const std::vector< uint8_t >& data)
{
  std::string hexdigit("0123456789abcdef");
  std::cout<<"Raw data: ";
  for (auto i : data)
    std::cout<<hexdigit[i & 0xf]<<hexdigit[i>>4];
  std::cout<<'\n';
}


void show_changeset(const Meta_Per_Changeset_Skeleton& arg)
{
  std::cout<<"Per changeset "<<arg.get_changeset()<<" from user "<<arg.get_user_id()<<", "
    <<(arg.get_is_redacted() ? "redacted" : "unredacted")<<", "<<arg.get_refs().size()<<" entries:\n";
    
  for (auto i : arg.get_refs())
    std::cout<<"  ref "<<i.ref<<", ver "<<i.version<<", timestamp "<<Timestamp(i.timestamp).str()<<'\n';
  std::cout<<'\n';
}


int main(int argc, char* args[])
{
  {
    std::vector< uint8_t > data(32, 0);
  
    Meta_Per_Changeset_Skeleton before(4711, false, 496);

    before.to_data(&data[0]);
    show_hex(data);
    std::cout<<"Expected size: "<<before.size_of()<<" from before, "
      <<Meta_Per_Changeset_Skeleton::size_of(&data[0])<<" from written data.\n";
    
    Meta_Per_Changeset_Skeleton after(&data[0]);
    show_changeset(after);
  }

  {
    std::vector< uint8_t > data(32, 0);
  
    Meta_Per_Changeset_Skeleton before(4711, false, 496);
    before.add_ref({ 9001001000, 1, Timestamp(2024, 10, 3, 11, 30, 0).timestamp });

    before.to_data(&data[0]);
    show_hex(data);
    std::cout<<"Expected size: "<<before.size_of()<<" from before, "
      <<Meta_Per_Changeset_Skeleton::size_of(&data[0])<<" from written data.\n";
    
    Meta_Per_Changeset_Skeleton after(&data[0]);
    show_changeset(after);
  }

  {
    std::vector< uint8_t > data(32, 0);
  
    Meta_Per_Changeset_Skeleton before(4711, false, 496);
    before.add_ref({ 9001001001, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001002, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001003, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001004, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });

    before.to_data(&data[0]);
    show_hex(data);
    std::cout<<"Expected size: "<<before.size_of()<<" from before, "
      <<Meta_Per_Changeset_Skeleton::size_of(&data[0])<<" from written data.\n";
    
    Meta_Per_Changeset_Skeleton after(&data[0]);
    show_changeset(after);
  }

  {
    std::vector< uint8_t > data(32, 0);
  
    Meta_Per_Changeset_Skeleton before(4711, true, 496);
    before.add_ref({ 9001001001, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001002, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001003, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001004, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });

    before.to_data(&data[0]);
    show_hex(data);
    std::cout<<"Expected size: "<<before.size_of()<<" from before, "
      <<Meta_Per_Changeset_Skeleton::size_of(&data[0])<<" from written data.\n";
    
    Meta_Per_Changeset_Skeleton after(&data[0]);
    show_changeset(after);
  }

  {
    std::vector< uint8_t > data(64, 0);
  
    Meta_Per_Changeset_Skeleton before(4711, false, 496);
    before.add_ref({    1001000, 999, Timestamp(2024, 10, 3, 11, 30, 30).timestamp });
    before.add_ref({ 1001001000, 4, Timestamp(2024, 10, 3, 11, 30, 30).timestamp });
    before.add_ref({ 4001001000, 9, Timestamp(2024, 10, 3, 11, 30, 30).timestamp });
    before.add_ref({ 9001001004, 2, Timestamp(2024, 10, 3, 11, 30, 30).timestamp });

    before.to_data(&data[0]);
    show_hex(data);
    std::cout<<"Expected size: "<<before.size_of()<<" from before, "
      <<Meta_Per_Changeset_Skeleton::size_of(&data[0])<<" from written data.\n";
    
    Meta_Per_Changeset_Skeleton after(&data[0]);
    show_changeset(after);
  }

  {
    std::vector< uint8_t > data(32, 0);
  
    Meta_Per_Changeset_Skeleton before(4711, false, 496);
    before.add_ref({ 9001001001, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001002, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001003, 1, Timestamp(2024, 10, 3, 11, 30, 15).timestamp });
    before.add_ref({ 9001001001, 2, Timestamp(2024, 10, 3, 12, 28, 45).timestamp });
    before.add_ref({ 9001001001, 3, Timestamp(2024, 10, 4, 10, 32, 0).timestamp });

    before.to_data(&data[0]);
    show_hex(data);
    std::cout<<"Expected size: "<<before.size_of()<<" from before, "
      <<Meta_Per_Changeset_Skeleton::size_of(&data[0])<<" from written data.\n";
    
    Meta_Per_Changeset_Skeleton after(&data[0]);
    show_changeset(after);
  }

  return 0;
}
