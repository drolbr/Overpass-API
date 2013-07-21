cgibindir = ${prefix}/cgi-bin
cgibin_PROGRAMS = interpreter position_flag show_position timestamp
dist_cgibin_SCRIPTS = augmented_diff augmented_state_by_date convert convert_xapi draw-line kill_my_queries ping sitemap sketch-line sketch-options sketch-route template trigger_clone xapi xapi_meta

escape_xml_cc = ../expat/escape_xml.cc
expat_cc = ../expat/expat_justparse_interface.cc
settings_cc = ../overpass_api/core/settings.cc
frontend_cc = ../overpass_api/frontend/user_interface.cc ../overpass_api/frontend/cgi-helper.cc ../overpass_api/frontend/output.cc
web_frontend_cc = ${frontend_cc} ../overpass_api/frontend/web_output.cc
dispatcher_cc = ../overpass_api/dispatch/scripting_core.cc ../overpass_api/frontend/map_ql_parser.cc ../overpass_api/statements/statement_dump.cc ../expat/map_ql_input.cc ../overpass_api/dispatch/resource_manager.cc ../template_db/dispatcher.cc

statements_dir = ../overpass_api/statements
statements_cc = ${statements_dir}/statement.cc ${statements_dir}/area_query.cc ${statements_dir}/around.cc ${statements_dir}/bbox_query.cc ${statements_dir}/coord_query.cc ${statements_dir}/difference.cc ${statements_dir}/foreach.cc ${statements_dir}/id_query.cc ${statements_dir}/item.cc ${statements_dir}/make_area.cc ${statements_dir}/newer.cc ${statements_dir}/osm_script.cc ${statements_dir}/pivot.cc ${statements_dir}/polygon_query.cc ${statements_dir}/print.cc ${statements_dir}/query.cc ${statements_dir}/recurse.cc ${statements_dir}/union.cc ${statements_dir}/user.cc ../overpass_api/frontend/print_target.cc ../overpass_api/data/collect_members.cc

interpreter_SOURCES = ${expat_cc} ${settings_cc} ${web_frontend_cc} ${statements_cc} ${dispatcher_cc} ../overpass_api/dispatch/web_query.cc ../overpass_api/osm-backend/area_updater.cc
interpreter_LDADD = -lexpat
timestamp_SOURCES = ${expat_cc} ${settings_cc} ${web_frontend_cc} ${statements_cc} ${dispatcher_cc} ../overpass_api/dispatch/db_timestamp.cc ../overpass_api/osm-backend/area_updater.cc
timestamp_LDADD = -lexpat
position_flag_SOURCES = ${settings_cc} ../overpass_api/frontend/cgi-helper.cc ../pt_diagrams/position_flag.cc
position_flag_LDADD = -lexpat
show_position_SOURCES = ${settings_cc} ../overpass_api/frontend/cgi-helper.cc ../pt_diagrams/show_position.cc
show_position_LDADD = -lexpat
