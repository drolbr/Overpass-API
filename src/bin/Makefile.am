bin_PROGRAMS = update_database update_from_dir osm3s_query scheduler
dist_bin_SCRIPTS = apply_osc_to_db.sh fetch_osc.sh

statements_cc = ../statements/statement.cc ../statements/area_query.cc ../statements/bbox_query.cc ../statements/coord_query.cc ../statements/foreach.cc ../statements/id_query.cc ../statements/item.cc ../statements/make_area.cc ../statements/osm_script.cc ../statements/print.cc ../statements/query.cc ../statements/recurse.cc ../statements/union.cc 

update_database_SOURCES = ../osm-backend/update_database.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
update_database_LDADD = -lexpat
update_from_dir_SOURCES = ../osm-backend/update_from_dir.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
update_from_dir_LDADD = -lexpat
osm3s_query_SOURCES = ../dispatch/osm3s_query.cc ../dispatch/resource_manager.cc ${statements_cc} ../core/settings.cc ../frontend/console_output.cc ../frontend/cgi-helper.cc ../frontend/user_interface.cc ../expat/expat_justparse_interface.cc ../osm-backend/stopwatch.cc
osm3s_query_LDADD = -lexpat -lrt
scheduler_SOURCES = ../dispatch/dispatcher.cc
scheduler_LDADD = -lrt
