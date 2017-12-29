#include "four_field_index.h"

#include <iostream>
#include <vector>


void check_result(const Four_Field_Index& idx, const std::string& expected)
{
  std::string result = idx.to_string();
  if (result == expected)
    std::cout<<"OK"<<'\n';
  else
    std::cout<<"failed: "<<result<<'\n';
}


int main(int argc, char* args[])
{
  if (argc < 2)
  {
    std::cout<<"Usage: "<<args[0]<<" test_to_execute\n";
    return 0;
  }
  std::string test_to_execute = args[1];
  
  if (test_to_execute.empty() || test_to_execute == "1")
  {
    Four_Field_Index idx;
    check_result(idx, "[]");
    idx.add_point(51., 7.);
    check_result(idx, "[ {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 1} ]");
    idx.add_point(51.001, 7.001);
    check_result(idx, "[ {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 1} ]");
    idx.add_point(50.995, 7.01);
    check_result(idx, "[ {(50.9902976, 7.0057984, 50.9968511, 7.0123519), 1}  {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 1} ]");
    idx.add_point(51., .01);
    check_result(idx, "[ {(50.9968512, 0.0065536, 51.0034047, 0.0131071), 1}  {(50.9902976, 7.0057984, 50.9968511, 7.0123519), 1}  {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 1} ]");
    idx.add_point(48., 4.);
    check_result(idx, "[ {(47.9953024, 3.9976960, 48.0018559, 4.0042495), 1}  {(50.9968512, 0.0065536, 51.0034047, 0.0131071), 1}  {(50.9902976, 7.0057984, 50.9968511, 7.0123519), 1}  {(50.9968512, 6.9992448, 51.0034047, 7.0057983), 1} ]");
  }
  
  if (test_to_execute.empty() || test_to_execute == "2")
  {
    Four_Field_Index idx;
    check_result(idx, "[]");
    idx.add_segment(51., 7., 51.001, 7.001);
    check_result(idx, "[]");
    idx.add_segment(51.001, 7.001, 51.005, 7.003);
    check_result(idx, "[]");
    idx.add_segment(51.001, 7.007, 51.005, 7.003);
    check_result(idx, "[]");
  }
  
  return 0;
}
