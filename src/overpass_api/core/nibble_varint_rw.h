#include <cstdint>


/* Variant that enforces little endian */
struct Nibble_Varint_Reader
{
  Nibble_Varint_Reader(const uint8_t* data, uint64_t size)
      : ptr(data), end(data + size), cur(0), bitpos(0)
  {
    if ((uintptr_t)ptr < (uintptr_t) end)
      cur = *ptr++;
    else
      bitpos = 8;
  }

  template< typename Int64 >
  Int64 read_flex(const uint32_t LIMITS)
  {
    uint_fast8_t to_skip = 4;
    uint_fast8_t num_bits = 68;

    if (((cur>>bitpos) & 0x1) == 0)
    {
      to_skip = 1;
      num_bits = (LIMITS & 0xff);
    }
    else if (((cur>>bitpos) & 0x3) == 0x1)
    {
      to_skip = 2;
      num_bits = ((LIMITS>>8) & 0xff);
    }
    else if (((cur>>bitpos) & 0x7) == 0x3)
    {
      to_skip = 3;
      num_bits = ((LIMITS>>16) & 0xff);
    }
    else if (((cur>>bitpos) & 0xf) == 0x7)
    {
      to_skip = 4;
      num_bits = ((LIMITS>>24) & 0xff);
    }

    if (num_bits <= 64)
      return (Int64)read_raw(num_bits)<<(64-num_bits)>>(64+to_skip-num_bits);
    
    read_raw(to_skip);
    return (Int64)read_raw(64);
  }

  template< typename Int64 >
  Int64 read_fixed(uint_fast8_t num_bits)
  {
    return (Int64)read_raw(num_bits)<<(64-num_bits)>>(64-num_bits);
  }
  
  bool good(uint_fast8_t num_bits) const
  { return (uintptr_t)ptr + (bitpos + num_bits - 1)/8 <= (uintptr_t)end || bitpos + num_bits <= 8; }
  bool good_flex() const
  { return good(5) || (good(4) && ((cur>>bitpos) & 0x1) == 0); }

private:
  const uint8_t* ptr;
  const uint8_t* end;
  uint8_t cur;
  uint_fast8_t bitpos;

  uint64_t read_raw(uint_fast8_t num_bits)
  {
    uint64_t result = cur>>bitpos;

    uint_fast8_t destpos = 8;
    while (destpos <= num_bits + bitpos)
    {
      if ((uintptr_t)ptr < (uintptr_t) end)
        cur = *ptr++;
      else
      {
        cur = 0;
        break;
      }
      if (destpos < num_bits + bitpos)
        result |= (uint64_t)cur<<(destpos - bitpos);
      destpos += 8;
    }
    bitpos += num_bits + 8 - destpos;

    return result;
  }
};
