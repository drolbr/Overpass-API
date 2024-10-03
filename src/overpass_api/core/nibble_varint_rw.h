#include <cstdint>


#ifdef ENFORCE_LITTLE_ENDIAN


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
      if ((uintptr_t)ptr < (uintptr_t)end)
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


#else


struct Nibble_Varint_Reader
{
  Nibble_Varint_Reader(const uint8_t* data, uint64_t size)
      : ptr((const uint64_t*)((uintptr_t)data & ~0x7ll)), end(data + size), cur(0), bitpos(0)
  {
    if ((uintptr_t)ptr < (uintptr_t) end)
    {
      bitpos = ((uintptr_t)data - (uintptr_t)ptr)*8;
      cur = *ptr++;
    }
    else
      bitpos = 64;
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
  { return (uintptr_t)ptr + (num_bits + bitpos + 7)/8 <= (uintptr_t)end + 8; }
  bool good_flex() const
  { return good(5) || (good(4) && ((cur>>bitpos) & 0x1) == 0); }

private:
  const uint64_t* ptr;
  const uint8_t* end;
  uint64_t cur;
  uint_fast8_t bitpos; // assert: 0..63

  // assert: never read more than 64 bits in one go.
  uint64_t read_raw(uint_fast8_t num_bits)
  {
    if (bitpos == 64)
      return 0;
    
    uint64_t result = cur>>bitpos;

    if (64 <= num_bits + bitpos)
    {
      if ((uintptr_t)ptr + 7 < (uintptr_t)end)
        cur = *ptr++;
      else if ((uintptr_t)ptr < (uintptr_t)end)
      {
        cur = *ptr++;
        cur &= ~(-1ul<<(((uintptr_t)end - (uintptr_t)ptr)*8));
      }
      else
      {
        cur = 0;
        bitpos = 64;
        return result;
      }

      if (64 < num_bits + bitpos)
        result |= (uint64_t)cur<<(64 - bitpos);
      bitpos += num_bits - 64;
    }
    else
      bitpos += num_bits;

    return result;
  }
};


#endif


/* Variant that enforces little endian.
 * No other variant because no huge performance difference expected. */
struct Nibble_Varint_Writer
{
  Nibble_Varint_Writer(uint8_t* data) : ptr(data), bitpos(0) {}
  
  static uint64_t size_in_bits(const uint32_t LIMITS, uint64_t value)
  {
    if (value <= (uint64_t)~(-1ull<<((LIMITS & 0xff) - 1)))
      return (LIMITS & 0xff);
    else if (value <= (uint64_t)~(-1ull<<(((LIMITS>>8) & 0xff) - 2)))
      return ((LIMITS>>8) & 0xff);
    else if (value <= (uint64_t)~(-1ull<<(((LIMITS>>16) & 0xff) - 3)))
      return ((LIMITS>>16) & 0xff);
    else if (value <= (uint64_t)~(-1ull<<(((LIMITS>>24) & 0xff) - 4)))
      return ((LIMITS>>24) & 0xff);
    
    return 68;
  }
  
  static uint64_t brutto_size_in_bytes(uint64_t brutto_size_in_bits)
  {
    return (brutto_size_in_bits + 7)/8;
  }

  static uint64_t size_of_size_in_bytes(const uint32_t LIMITS, uint64_t netto_size_in_bits)
  {    
    if ((netto_size_in_bits + (LIMITS & 0xff) + 7)/8 <= (uint64_t)~(-1ull<<((LIMITS & 0xff) - 1)))
      return (LIMITS & 0xff);
    else if ((netto_size_in_bits + ((LIMITS>>8) & 0xff) + 7)/8 <= (uint64_t)~(-1ull<<(((LIMITS>>8) & 0xff) - 2)))
      return ((LIMITS>>8) & 0xff);
    else if ((netto_size_in_bits + ((LIMITS>>16) & 0xff) + 7)/8 <= (uint64_t)~(-1ull<<(((LIMITS>>16) & 0xff) - 3)))
      return ((LIMITS>>16) & 0xff);
    else if ((netto_size_in_bits + ((LIMITS>>24) & 0xff) + 7)/8 <= (uint64_t)~(-1ull<<(((LIMITS>>24) & 0xff) - 4)))
      return ((LIMITS>>24) & 0xff);
    
    return 68;
  }

  void write_flex(const uint32_t LIMITS, uint64_t value)
  {
    uint_fast8_t to_skip = 4;
    uint_fast8_t num_bits = 68;
    uint_fast8_t size_marker = 0xf;
    
    if (value <= (uint64_t)~(-1ull<<((LIMITS & 0xff) - 1)))
    {
      to_skip = 1;
      size_marker = 0;
      num_bits = (LIMITS & 0xff);
    }
    else if (value <= (uint64_t)~(-1ull<<(((LIMITS>>8) & 0xff) - 2)))
    {
      to_skip = 2;
      size_marker = 0x1;
      num_bits = ((LIMITS>>8) & 0xff);
    }
    else if (value <= (uint64_t)~(-1ull<<(((LIMITS>>16) & 0xff) - 3)))
    {
      to_skip = 3;
      size_marker = 0x3;
      num_bits = ((LIMITS>>16) & 0xff);
    }
    else if (value <= (uint64_t)~(-1ull<<(((LIMITS>>24) & 0xff) - 4)))
    {
      to_skip = 4;
      size_marker = 0x7;
      num_bits = ((LIMITS>>24) & 0xff);
    }
    
    if (num_bits <= 64)
      write_fixed(num_bits, (value<<to_skip) | size_marker);
    else
    {
      write_fixed(to_skip, size_marker);
      write_fixed(64, value);
    }
  }

  void write_fixed(uint_fast8_t num_bits, uint64_t value)
  {
    if (num_bits < 64)
      value &= ~(-1ull<<num_bits);
    
    *ptr = (*ptr & ~(-1ull<<bitpos)) | (uint8_t)((value<<bitpos) & 0xff);

    if (bitpos + num_bits < 8)
    {
      bitpos += num_bits;
      return;
    }
    uint_fast8_t valpos = 8 - bitpos;
    while (valpos < num_bits)
    {
      ++ptr;
      *ptr = (uint8_t)((value>>valpos) & 0xff);
      valpos += 8;
    }
    if (valpos == num_bits)
    {
      ++ptr;
      bitpos = 0;
    }
    else
      bitpos = 8 + num_bits - valpos;
  }
  
  void pad_to_byte()
  {
    if (bitpos > 0)
      *ptr = (*ptr | (-1ull<<bitpos));
  }

private:
  uint8_t* ptr;
  uint8_t cur;
  uint_fast8_t bitpos;
};
