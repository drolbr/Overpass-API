#ifndef DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_POPUP_H
#define DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_POPUP_H


#include <string>
#include <vector>


struct Tag_Filter
{
  std::string key;
  std::string value;
  bool straight;  
};


struct Category_Filter
{
  std::string title;
  std::string title_key;
  std::vector< std::vector< Tag_Filter > > filter_disjunction;
};


class Output_Popup : public Output_Handler
{
public:
  Output_Popup(std::vector< Category_Filter > categories_) : categories(categories_) {}
  
  virtual void print_global_bbox(const Bbox_Double& bbox) {}
  
private:
  std::vector< Category_Filter > categories;
};


#endif
