#ifndef DE__OSM3S___TEMPLATE_DB__VOID_BLOCKS_H
#define DE__OSM3S___TEMPLATE_DB__VOID_BLOCKS_H

#include <cstdint>
#include <vector>


class Void_Blocks
{
public:
  uint32_t allocate_block(uint32_t data_size);
  void release_block(uint32_t offset, uint32_t data_size);

  void set_size(uint32_t size);
  void set_used(uint32_t offset, uint32_t data_size);
  void set_reserved(uint32_t offset, uint32_t data_size);

  uint32_t size() const { return used.size(); }
  std::vector< bool > get_index_footprint() const;
  uint32_t num_distinct_blocks() const;
  void dump_distinct_blocks(uint8_t* target) const;

  void read_dump(uint8_t* source, uint32_t num_entries);

private:
  std::vector< uint8_t > reserved;
  std::vector< uint8_t > used;
  static const uint8_t large_threshold = 8;
};


#endif
