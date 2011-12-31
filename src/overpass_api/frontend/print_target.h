#ifndef PRINT_TARGET
#define PRINT_TARGET

#include "../statements/print.h"

using namespace std;

class Print_Target_Xml : public Print_Target
{
  public:
    Print_Target_Xml(uint32 mode, Transaction& transaction)
        : Print_Target(mode, transaction) {}
    
    virtual void print_item(uint32 ll_upper, const Node_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Way_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Relation_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
    virtual void print_item(uint32 ll_upper, const Area_Skeleton& skel,
			    const vector< pair< string, string > >* tags = 0,
			    const OSM_Element_Metadata_Skeleton* meta = 0,
			    const map< uint32, string >* users = 0) const;
};

#endif
