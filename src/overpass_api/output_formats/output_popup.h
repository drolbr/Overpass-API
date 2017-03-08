#ifndef DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_POPUP_H
#define DE__OSM3S___OVERPASS_API__OUTPUT_FORMATS__OUTPUT_POPUP_H


#include "../core/datatypes.h"
#include "../core/geometry.h"
#include "../data/regular_expression.h"
#include "../frontend/output_handler.h"

#include <string>
#include <vector>


class Tag_Filter
{
public:
  Tag_Filter(const std::string& key_, const std::string& value_, bool straight_)
      : key(key_), straight(straight_), condition(value_, true) {}
  
  bool matches(const std::vector< std::pair< std::string, std::string > >* tags) const;
  
private:
  std::string key;
  bool straight;
  Regular_Expression condition;
};


struct Category_Filter
{
  Category_Filter() {}
  
  ~Category_Filter()
  {
    for (std::vector< std::vector< Tag_Filter* > >::iterator it_conj = filter_disjunction.begin();
        it_conj != filter_disjunction.end(); ++it_conj)
    {
      for (std::vector< Tag_Filter* >::iterator it = it_conj->begin(); it != it_conj->end(); ++it)
        delete *it;
    }
  }
  
  void set_title(const std::string& title);
  void set_title_key(const std::string& title_key);
  void add_filter(const std::vector< Tag_Filter* >& conjunction) { filter_disjunction.push_back(conjunction); }
  
  bool consider(const Node_Skeleton& skel, const std::vector< std::pair< std::string, std::string > >* tags);
  bool consider(const Way_Skeleton& skel, const std::vector< std::pair< std::string, std::string > >* tags);
  bool consider(const Relation_Skeleton& skel, const std::vector< std::pair< std::string, std::string > >* tags);
  
  std::string result() const { return output; }
  
private:
  Category_Filter(const Category_Filter&);
  const Category_Filter& operator=(const Category_Filter&);
  
  std::string output;
  std::string title_key;
  std::vector< std::vector< Tag_Filter* > > filter_disjunction;
};


class Output_Popup : public Output_Handler
{
public:
  Output_Popup(std::vector< Category_Filter* > categories_) : categories(categories_) {}
  
  virtual ~Output_Popup()
  {
    for (std::vector< Category_Filter* >::iterator it = categories.begin(); it != categories.end(); ++it)
      delete *it;
  }

  virtual bool write_http_headers();
  virtual void write_payload_header(const std::string& db_dir,
				    const std::string& timestamp, const std::string& area_timestamp);
  virtual void write_footer();
  virtual void display_remark(const std::string& text);
  virtual void display_error(const std::string& text);

  virtual void print_global_bbox(const Bbox_Double& bbox) {}
  
  virtual void print_item(const Node_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Node_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Node::Id_Type >* new_meta = 0);
  
  virtual void print_item(const Way_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* meta,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Way_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Way::Id_Type >* new_meta = 0);
      
  virtual void print_item(const Relation_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* meta,
      const std::map< uint32, std::string >* roles,
      const std::map< uint32, std::string >* users,
      Output_Mode mode,
      const Feature_Action& action = keep,
      const Relation_Skeleton* new_skel = 0,
      const Opaque_Geometry* new_geometry = 0,
      const std::vector< std::pair< std::string, std::string > >* new_tags = 0,
      const OSM_Element_Metadata_Skeleton< Relation::Id_Type >* new_meta = 0);
                            
  virtual void print_item(const Derived_Skeleton& skel,
      const Opaque_Geometry& geometry,
      const std::vector< std::pair< std::string, std::string > >* tags,
      Output_Mode mode);
  
private:
  std::vector< Category_Filter* > categories;
};


#endif
