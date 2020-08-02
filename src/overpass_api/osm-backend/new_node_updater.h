#ifndef DE__OSM3S___OVERPASS_API__OSM_BACKEND__NEW_NODE_UPDATER_H
#define DE__OSM3S___OVERPASS_API__OSM_BACKEND__NEW_NODE_UPDATER_H

#include "data_from_osc.h"

#include "../../template_db/transaction.h"


void update_nodes(Transaction& transaction, Data_From_Osc& new_data);


#endif
