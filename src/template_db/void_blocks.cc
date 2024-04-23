#include "void_blocks.h"


uint32_t Void_Blocks::allocate_block(uint32_t data_size)
{
  uint32_t result = used.size();
  uint32_t first_large_gap = used.size();

  uint32_t gap_start = 0;
  for (uint32_t i = 0; i < used.size(); ++i)
  {
    if (used[i])
    {
      if (i - gap_start == data_size)
      {
        result = gap_start;
        break;
      }
      else if (i - gap_start > large_threshold && i - gap_start > data_size && first_large_gap == used.size())
        first_large_gap = gap_start;

      gap_start = i+1;
    }
  }
  
  if (result == used.size())
    result = first_large_gap;
  if (result == used.size())
    result = gap_start;
  
  set_used(result, data_size);
  
  return result;
}


void Void_Blocks::release_block(uint32_t offset, uint32_t data_size)
{
  for (uint32_t i = offset; i < offset + data_size && i < used.size(); ++i)
  {
    if (i < reserved.size() && reserved[i])
      return;

    used[i] = false;
  }
}


void Void_Blocks::set_size(uint32_t size)
{
  used.resize(size, false);
  reserved.resize(size, false);
}


void Void_Blocks::set_used(uint32_t offset, uint32_t data_size)
{
  if (used.size() < offset + data_size)
    used.resize(offset + data_size, false);
  for (uint32_t i = offset; i < offset + data_size; ++i)
    used[i] = true;
}


void Void_Blocks::set_reserved(uint32_t offset, uint32_t data_size)
{
  if (reserved.size() < offset + data_size)
    reserved.resize(offset + data_size, false);
  for (uint32_t i = offset; i < offset + data_size; ++i)
    reserved[i] = true;
  
  set_used(offset, data_size);
}


std::vector< bool > Void_Blocks::get_index_footprint() const
{
  std::vector< bool > result(used.size(), false);
  decltype(result.size()) j = 0;
  for (auto i : used)
    result[j++] = i;
  return result;
}


uint32_t Void_Blocks::num_distinct_blocks() const
{
  uint32_t result = 0;

  uint32_t gap_start = 0;
  for (uint32_t i = 0; i < used.size(); ++i)
  {
    if (used[i])
    {
      if (gap_start < i)
        ++result;

      gap_start = i+1;
    }
  }
  if (gap_start < used.size())
    ++result;

  return result;
}


void Void_Blocks::dump_distinct_blocks(uint8_t* target) const
{
  uint32_t gap_start = 0;
  for (uint32_t i = 0; i < used.size(); ++i)
  {
    if (used[i])
    {
      if (gap_start < i)
      {
        *(uint32_t*)target = i - gap_start;
        *(uint32_t*)(target+4) = gap_start;
        target += 8;
      }

      gap_start = i+1;
    }
  }
  if (gap_start < used.size())
  {
    *(uint32_t*)target = used.size() - gap_start;
    *(uint32_t*)(target+4) = gap_start;
  }
}


void Void_Blocks::read_dump(uint8_t* source, uint32_t num_entries)
{
  uint32_t used_start = 0;
  for (uint32_t i = 0; i < num_entries; ++i)
  {
    if (used_start < *(uint32_t*)(source+4))
      set_reserved(used_start, *(uint32_t*)(source+4) - used_start);
    used_start = *(uint32_t*)source + *(uint32_t*)(source+4);
    source += 8;
  }
  if (used_start < used.size())
    set_reserved(used_start, used.size() - used_start);
}
