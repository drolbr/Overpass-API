#include "nibble_varint_rw.h"

#include <iostream>
#include <vector>


void show_good(const Nibble_Varint_Reader& reader)
{
  std::cout<<"Good: "<<std::dec<<reader.good(4)<<reader.good(8)<<reader.good(12)<<reader.good(16)
      <<reader.good(32)<<reader.good(64)<<reader.good_flex()<<'\n';
}


int main(int argc, char* args[])
{
  // Test that an empty reader does not crash
  {
    std::cout<<"Empty reader:\n";

    Nibble_Varint_Reader reader(0, 0);
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Test one byte input available, static reading
  {
    std::cout<<"One byte, non-flex end:\n";

    std::vector< uint8_t > src = { 0x50 };
    Nibble_Varint_Reader reader(&src[0], src.size());
    show_good(reader);
    std::cout<<"read_fixed(4): "<<reader.read_fixed< int64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<"read_fixed(4): "<<reader.read_fixed< int64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Test one byte input available, static then flex reading
  {
    std::cout<<"One byte, flex end, signed:\n";

    std::vector< uint8_t > src = { 0xa8 };
    Nibble_Varint_Reader reader(&src[0], src.size());
    show_good(reader);
    std::cout<<"read_fixed(4): "<<reader.read_fixed< int64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<"read_flex(): "<<reader.read_flex< int64_t >(0x20100804)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Test one byte input available, static then flex reading
  {
    std::cout<<"One byte, flex end, unsigned:\n";

    std::vector< uint8_t > src = { 0xa8 };
    Nibble_Varint_Reader reader(&src[0], src.size());
    show_good(reader);
    std::cout<<"read_fixed(4): "<<reader.read_fixed< uint64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<"read_flex(): "<<reader.read_flex< uint64_t >(0x20100804)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Test multibyte input available, static reading
  {
    std::cout<<"Multiple bytes, only static reading:\n";

    std::vector< uint8_t > src = { 0x2a, 0xba, 0x21, 0x09, 0x21, 0x9b, 0x1a, 0x6d, 0x2c, 0x06, 0x00, 0x00, 0x20, 0x93, 0x67 };
    Nibble_Varint_Reader reader(&src[0], src.size());
    show_good(reader);
    std::cout<<"read_fixed(8): "<<reader.read_fixed< uint64_t >(8)<<'\n';
    show_good(reader);
    std::cout<<"read_fixed(12): "<<reader.read_fixed< uint64_t >(12)<<'\n';
    show_good(reader);
    std::cout<<"read_fixed(16): "<<reader.read_fixed< uint64_t >(16)<<'\n';
    show_good(reader);
    std::cout<<"read_fixed(64): "<<reader.read_fixed< uint64_t >(64)<<'\n';
    show_good(reader);
    std::cout<<"read_fixed(20): "<<reader.read_fixed< uint64_t >(20)<<'\n';
    show_good(reader);
    std::cout<<"read_fixed(4): "<<reader.read_fixed< uint64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<"read_fixed(64): "<<reader.read_fixed< uint64_t >(64)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Test multibyte input available, flex reading
  {
    std::cout<<"Multiple bytes, only flex reading:\n";

    std::vector< uint8_t > src = { 0x54, 0xe9, 0x36, 0x49, 0xf8, 0xb2, 0xa9, 0xd1, 0xc6, 0x62, 0x00, 0x00, 0x00, 0x27, 0xcd, 0xfa };
    Nibble_Varint_Reader reader(&src[0], src.size());
    show_good(reader);
    std::cout<<"read_flex(7+1): "<<reader.read_flex< uint64_t >(0x14100c08)<<'\n';
    show_good(reader);
    std::cout<<"read_flex(10+2): "<<reader.read_flex< uint64_t >(0x14100c08)<<'\n';
    show_good(reader);
    std::cout<<"read_flex(13+3): "<<reader.read_flex< uint64_t >(0x14100c08)<<'\n';
    show_good(reader);
    std::cout<<"read_flex(64+4): "<<reader.read_flex< uint64_t >(0x14100c08)<<'\n';
    show_good(reader);
    std::cout<<"read_flex(16+4): "<<reader.read_flex< uint64_t >(0x14100c08)<<'\n';
    show_good(reader);
    std::cout<<"read_flex(8): "<<reader.read_flex< uint64_t >(0x14100c08)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  return 0;
}
