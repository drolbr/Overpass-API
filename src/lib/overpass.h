#ifndef DE__OVERPASS_API__LIBOVERPASS
#define DE__OVERPASS_API__LIBOVERPASS

#ifdef __cplusplus
extern "C" {
#endif


struct Overpass_C_Node
{
  int id;
  double lat;
  double lon;
  
  int num_tags;
  char** tags;
};


struct Overpass_C_Way
{
  int id;
  int num_nodes;
  Overpass_C_Node** nodes;
  
  int num_tags;
  char** tags;
};


struct Overpass_C_Relation
{
  int id;
  
  int num_tags;
  char** tags;
};


struct Overpass_C_Handle
{
  int num_nodes;
  Overpass_C_Node* nodes;

  int num_ways;
  Overpass_C_Way* ways;

  int num_relations;
  Overpass_C_Relation* relations;
};


void alloc_overpass_handle(Overpass_C_Handle** handle);

void overpass_bbox(Overpass_C_Handle* handle,
		   double south, double west, double north, double east);

// Returns
// 0 if no elements are left
// 1 if next element is a node
// 2 if next element is a way
int has_next_overpass_handle(Overpass_C_Handle* handle);

// The library keeps ownership of the pointer
Overpass_C_Node* next_node_overpass_handle(Overpass_C_Handle* handle);

// The library keeps ownership of the pointer
Overpass_C_Way* next_way_overpass_handle(Overpass_C_Handle* handle);

// The library keeps ownership of the pointer
Overpass_C_Relation* next_relation_overpass_handle(Overpass_C_Handle* handle);

void free_overpass_handle(Overpass_C_Handle* handle);


#ifdef __cplusplus
}
#endif

#endif
