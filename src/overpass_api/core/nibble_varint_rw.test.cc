#include "nibble_varint_rw.h"

#include <iostream>
#include <vector>


void show_good(const Nibble_Varint_Reader& reader)
{
  std::cout<<"Good: "<<std::dec<<reader.good(4)<<reader.good(8)<<reader.good(12)<<reader.good(16)
      <<reader.good(32)<<reader.good(64)<<reader.good_flex()<<'\n';
}


void show_hex(const std::vector< uint8_t >& data)
{
  std::string hexdigit("0123456789abcdef");
  std::cout<<"Raw data: ";
  for (auto i : data)
    std::cout<<hexdigit[i & 0xf]<<hexdigit[i>>4];
  std::cout<<'\n';
}


int main(int argc, char* args[])
{
  // Test that an empty reader does not crash
  {
    std::cout<<"Empty reader:\n";

    Nibble_Varint_Reader reader(nullptr, 0);
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
    Nibble_Varint_Reader reader(&src[1], src.size() - 1);
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
  
  // Test Nibble_Varint_Writer
  
  // Check the static size calculation functions
  std::cout<<"Brutto size in bytes "
      "of 0 is "<<Nibble_Varint_Writer::brutto_size_in_bytes(0)<<", "
      "of 1 is "<<Nibble_Varint_Writer::brutto_size_in_bytes(1)<<", "
      "of 7 is "<<Nibble_Varint_Writer::brutto_size_in_bytes(7)<<", "
      "of 8 is "<<Nibble_Varint_Writer::brutto_size_in_bytes(8)<<", "
      "of 9 is "<<Nibble_Varint_Writer::brutto_size_in_bytes(9)<<", "
      "of 1016 is "<<Nibble_Varint_Writer::brutto_size_in_bytes(1016)<<", "
      "of 1017 is "<<Nibble_Varint_Writer::brutto_size_in_bytes(1017)<<"\n\n";
  std::cout<<"Size of 0 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 0)<<'\n';
  std::cout<<"Size of 1 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 1)<<'\n';
  std::cout<<"Size of 7 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 7)<<'\n';
  std::cout<<"Size of 8 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 8)<<'\n';
  std::cout<<"Size of 63 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 63)<<'\n';
  std::cout<<"Size of 64 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 64)<<'\n';
  std::cout<<"Size of 8191 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 8191)<<'\n';
  std::cout<<"Size of 8192 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 8192)<<'\n';
  std::cout<<"Size of 1048575 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 1048575)<<'\n';
  std::cout<<"Size of 1048576 with limits 0x18100804: "<<Nibble_Varint_Writer::size_in_bits(0x18100804, 1048576)<<'\n';
  std::cout<<'\n';
  std::cout<<"Size of size 0 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 0)<<'\n';
  std::cout<<"Size of size 4 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 4)<<'\n';
  std::cout<<"Size of size 1008 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 1008)<<'\n';
  std::cout<<"Size of size 1012 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 1012)<<'\n';
  std::cout<<"Size of size 8172 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 8172)<<'\n';
  std::cout<<"Size of size 8176 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 8176)<<'\n';
  std::cout<<"Size of size 65512 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 65512)<<'\n';
  std::cout<<"Size of size 65516 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 65516)<<'\n';
  std::cout<<"Size of size 8388576 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 8388576)<<'\n';
  std::cout<<"Size of size 8388580 with limits 0x18100c08: "<<Nibble_Varint_Writer::size_of_size_in_bytes(0x18100c08, 8388580)<<'\n';
  std::cout<<'\n';
  
  // Test that an empty writer does not crash
  {
    std::cout<<"Empty writer:\n";

    Nibble_Varint_Writer writer((uint8_t*)nullptr);
    std::cout<<'\n';
  }
  
  // Write four bits, read four bits
  {
    std::cout<<"Write four bits, read four bits:\n";

    std::vector< uint8_t > data(8, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_fixed(4, 5);
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], data.size());
    std::cout<<"read_fixed(4): "<<reader.read_fixed< int64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write twelve bits, read twelve bits
  {
    std::cout<<"Write twelve bits, read twelve bits:\n";

    std::vector< uint8_t > data(8, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_fixed(12, 442);
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], data.size());
    std::cout<<"read_fixed(12): "<<reader.read_fixed< int64_t >(12)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write multiple full bytes
  {
    std::cout<<"Write multiple full bytes:\n";

    std::vector< uint8_t > data(32, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_fixed(8, 42);
      writer.write_fixed(64, 424242424242);
      writer.write_fixed(32, 42424242);
      writer.write_fixed(16, 44242);
      writer.write_fixed(8, 44);
      writer.write_fixed(64, 444244424442);
      writer.write_fixed(8, 24);
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], data.size());
    std::cout<<"read_fixed(8): "<<reader.read_fixed< uint64_t >(8)<<'\n';
    std::cout<<"read_fixed(64): "<<reader.read_fixed< uint64_t >(64)<<'\n';
    std::cout<<"read_fixed(32): "<<reader.read_fixed< uint64_t >(32)<<'\n';
    std::cout<<"read_fixed(16): "<<reader.read_fixed< uint64_t >(16)<<'\n';
    std::cout<<"read_fixed(8): "<<reader.read_fixed< uint64_t >(8)<<'\n';
    std::cout<<"read_fixed(64): "<<reader.read_fixed< uint64_t >(64)<<'\n';
    std::cout<<"read_fixed(8): "<<reader.read_fixed< uint64_t >(8)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write over byte boundaries
  {
    std::cout<<"Write over byte boundaries:\n";

    std::vector< uint8_t > data(16, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_fixed(4, 5);
      writer.write_fixed(8, 42);
      writer.write_fixed(64, 424242424242);
      writer.write_fixed(4, 11);
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], data.size());
    std::cout<<"read_fixed(4): "<<reader.read_fixed< uint64_t >(4)<<'\n';
    std::cout<<"read_fixed(8): "<<reader.read_fixed< uint64_t >(8)<<'\n';
    std::cout<<"read_fixed(64): "<<reader.read_fixed< uint64_t >(64)<<'\n';
    std::cout<<"read_fixed(4): "<<reader.read_fixed< uint64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write unaligned
  {
    std::cout<<"Write unaligned:\n";

    std::vector< uint8_t > data(16, 0x99);
    {
      Nibble_Varint_Writer writer(&data[1]);
      writer.write_fixed(4, 5);
      writer.write_fixed(8, 42);
      writer.write_fixed(64, 424242424242);
      writer.write_fixed(4, 11);
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[1], data.size() - 1);
    std::cout<<"read_fixed(4): "<<reader.read_fixed< uint64_t >(4)<<'\n';
    std::cout<<"read_fixed(8): "<<reader.read_fixed< uint64_t >(8)<<'\n';
    std::cout<<"read_fixed(64): "<<reader.read_fixed< uint64_t >(64)<<'\n';
    std::cout<<"read_fixed(4): "<<reader.read_fixed< uint64_t >(4)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write one flex entry
  {
    std::cout<<"Write one flex entry:\n";

    std::vector< uint8_t > data(16, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_flex(0x20100804, 5);
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], data.size());
    std::cout<<"read_flex(3+1): "<<reader.read_flex< uint64_t >(0x20100804)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write many flex entries
  {
    std::cout<<"Write many flex entries:\n";

    std::vector< uint8_t > data(16, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_flex(0x100c0804, 6);
      writer.write_flex(0x100c0804, 42);
      writer.write_flex(0x100c0804, 4042);
      writer.write_flex(0x100c0804, 4242);
      writer.write_flex(0x100c0804, 442);
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], data.size());
    std::cout<<"read_flex(3+1): "<<reader.read_flex< uint64_t >(0x100c0804)<<'\n';
    std::cout<<"read_flex(6+2): "<<reader.read_flex< uint64_t >(0x100c0804)<<'\n';
    std::cout<<"read_flex(12+4): "<<reader.read_flex< uint64_t >(0x100c0804)<<'\n';
    std::cout<<"read_flex(64+4): "<<reader.read_flex< uint64_t >(0x100c0804)<<'\n';
    std::cout<<"read_flex(9+3): "<<reader.read_flex< uint64_t >(0x100c0804)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write one flex entry with padding
  {
    std::cout<<"Trigger effective padding:\n";

    std::vector< uint8_t > data(16, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_flex(0x18100c08, 442);
      writer.pad_to_byte();
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], 2);
    std::cout<<"read_flex(10+2): "<<reader.read_flex< uint64_t >(0x18100c08)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }
  
  // Write one flex entry with padding
  {
    std::cout<<"Trigger empty padding:\n";

    std::vector< uint8_t > data(16, 0);
    {
      Nibble_Varint_Writer writer(&data[0]);
      writer.write_flex(0x18100c08, 4242);
      writer.pad_to_byte();
    }
    show_hex(data);

    Nibble_Varint_Reader reader(&data[0], 2);
    std::cout<<"read_flex(13+3): "<<reader.read_flex< uint64_t >(0x18100c08)<<'\n';
    show_good(reader);
    std::cout<<'\n';
  }

  return 0;
}
