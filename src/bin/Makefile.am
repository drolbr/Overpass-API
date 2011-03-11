bin_PROGRAMS = update_database update_from_dir osm3s_query scheduler
dist_bin_SCRIPTS = apply_osc_to_db.sh fetch_osc.sh

expat_cc = ../expat/expat_justparse_interface.cc
settings_cc = ../overpass_api/core/settings.cc
statements_dir = ../overpass_api/statements
statements_cc = ${statements_dir}/statement.cc ${statements_dir}/area_query.cc ${statements_dir}/bbox_query.cc ${statements_dir}/coord_query.cc ${statements_dir}/foreach.cc ${statements_dir}/id_query.cc ${statements_dir}/item.cc ${statements_dir}/make_area.cc ${statements_dir}/osm_script.cc ${statements_dir}/print.cc ${statements_dir}/query.cc ${statements_dir}/recurse.cc ${statements_dir}/union.cc 

update_database_SOURCES = ${expat_cc} ${settings_cc} ../overpass_api/osm-backend/update_database.cc 
update_database_LDADD = -lexpat_cc
update_from_dir_SOURCES = ${expat_cc} ${settings_cc} ../overpass_api/osm-backend/update_from_dir.cc
update_from_dir_LDADD = -lexpat
osm3s_query_SOURCES = ${expat_cc} ${settings_cc} ../overpass_api/dispatch/osm3s_query.cc ../overpass_api/dispatch/resource_manager.cc ${statements_cc} ../overpass_api/frontend/console_output.cc ../overpass_api/frontend/cgi-helper.cc ../overpass_api/frontend/user_interface.cc  ../overpass_api/osm-backend/stopwatch.cc
osm3s_query_LDADD = -lexpat -lrt
scheduler_SOURCES = ../dispatch/dispatcher.cc
scheduler_LDADD = -lrt
