
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>


int main(int argc, char* args[])
{
  std::vector< std::string > request(16*1024*1024);
  
  uint64_t cnt = 0;
  
  for (std::string buf; std::getline(std::cin, buf); )
  {
    if (buf.size() <= 3 || buf.substr(0, 3) != "202")
      continue;
    if (++cnt % 100000 == 0)
      std::cerr<<'.';
    
    std::stringstream buf_s(buf);
    std::string date, time, pid_s, action;
    std::getline(buf_s, date, ' ');
    std::getline(buf_s, time, ' ');
    std::getline(buf_s, pid_s, ' ');
    std::getline(buf_s, action, ' ');
    
    if (action == "read_finished")
    {
      std::string anon_hash;
      std::getline(buf_s, anon_hash, ' ');

      uint32_t pid = std::atol(&pid_s[1]);
      if (!request[pid].empty())
      {
        std::ofstream out(("sample." + anon_hash + ".txt").c_str(), std::ios::app);
        out<<request[pid]<<'\n';
        request[pid].clear();
      }
    }
    else if (action == "requesting" && pid_s.size() > 2)
    {
      uint32_t pid = std::atol(&pid_s[1]);
      if (pid > 0 && pid < 16*1024*1024)
        request[pid] = buf.substr(buf.find("requesting") + 11);
    }
  }
  
  std::cerr<<'\n';
  return 0;
}
