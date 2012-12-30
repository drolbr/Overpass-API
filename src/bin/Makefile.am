bin_PROGRAMS = osm3s_query dispatcher update_database update_from_dir sketch_route_svg draw_route_svg tocgi uncgi escape_xml bbox_brim_query translate_xapi process_augmented_diffs
dist_bin_SCRIPTS = apply_osc_to_db.sh cleanup_xapi_tmp.sh clone.sh compress_clone.sh download_clone.sh fetch_osc.sh run_osm3s_minutely.sh rules_loop.sh 

escape_xml_cc = ../expat/escape_xml.cc
expat_cc = ../expat/expat_justparse_interface.cc
settings_cc = ../overpass_api/core/settings.cc
output_cc = ../overpass_api/frontend/output.cc
frontend_cc = ../overpass_api/frontend/user_interface.cc ../overpass_api/frontend/cgi-helper.cc ../overpass_api/frontend/output.cc
console_frontend_cc = ${frontend_cc} ../overpass_api/frontend/console_output.cc
web_frontend_cc = ${frontend_cc} ../overpass_api/frontend/web_output.cc
osm_updater_cc = ../overpass_api/osm-backend/meta_updater.cc ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/way_updater.cc ../overpass_api/osm-backend/relation_updater.cc ../overpass_api/osm-backend/osm_updater.cc ../template_db/dispatcher.cc ../overpass_api/data/collect_members.cc ../overpass_api/dispatch/resource_manager.cc ../overpass_api/osm-backend/area_updater.cc
dispatcher_cc = ../overpass_api/dispatch/scripting_core.cc ../overpass_api/frontend/map_ql_parser.cc ../overpass_api/statements/statement_dump.cc ../expat/map_ql_input.cc ../overpass_api/dispatch/resource_manager.cc ../overpass_api/osm-backend/area_updater.cc ../template_db/dispatcher.cc

statements_dir = ../overpass_api/statements
statements_cc = ${statements_dir}/statement.cc ${statements_dir}/area_query.cc ${statements_dir}/around.cc ${statements_dir}/bbox_query.cc ${statements_dir}/coord_query.cc ${statements_dir}/foreach.cc ${statements_dir}/id_query.cc ${statements_dir}/item.cc ${statements_dir}/make_area.cc ${statements_dir}/newer.cc ${statements_dir}/osm_script.cc ${statements_dir}/polygon_query.cc ${statements_dir}/print.cc ${statements_dir}/query.cc ${statements_dir}/recurse.cc ${statements_dir}/union.cc ${statements_dir}/user.cc ../overpass_api/frontend/print_target.cc ../overpass_api/data/collect_members.cc

update_database_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ${osm_updater_cc} ../overpass_api/osm-backend/update_database.cc 
update_database_LDADD = -lexpat
update_from_dir_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ${osm_updater_cc} ../overpass_api/osm-backend/update_from_dir.cc
update_from_dir_LDADD = -lexpat
osm3s_query_SOURCES = ${expat_cc} ${settings_cc} ${console_frontend_cc} ${statements_cc} ${dispatcher_cc} ../overpass_api/dispatch/osm3s_query.cc ../overpass_api/osm-backend/clone_database.cc ../overpass_api/frontend/web_output.cc
osm3s_query_LDADD = -lexpat
dispatcher_SOURCES = ../template_db/dispatcher.cc ../overpass_api/core/settings.cc ../overpass_api/dispatch/dispatcher_server.cc ../overpass_api/frontend/user_interface.cc ../overpass_api/frontend/cgi-helper.cc
dispatcher_LDADD = 

sketch_route_svg_SOURCES = ${expat_cc} ${escape_xml_cc} ../pt_diagrams/sketch-route-svg.cc ../pt_diagrams/processed_input.cc
sketch_route_svg_LDADD = -lexpat
draw_route_svg_SOURCES = ${expat_cc} ${escape_xml_cc} ../pt_diagrams/read_input.cc ../pt_diagrams/topographic.cc
draw_route_svg_LDADD = -lexpat
uncgi_SOURCES = ../pt_diagrams/uncgi.cc
uncgi_LDADD = -lexpat
tocgi_SOURCES = ../pt_diagrams/tocgi.cc
tocgi_LDADD = 
escape_xml_SOURCES = ../expat/escape_xml.cc ../expat/escape_xml_main.cc
bbox_brim_query_SOURCES = ${expat_cc} ../pt_diagrams/bbox-brim-query.cc
bbox_brim_query_LDADD = -lexpat
translate_xapi_SOURCES = ../expat/translate_xapi.cc ../expat/escape_xml.cc
translate_xapi_LDADD = 
process_augmented_diffs_SOURCES = ../expat/process_augmented_diffs.cc ../expat/expat_justparse_interface.cc ../expat/escape_xml.cc
process_augmented_diffs_LDADD = -lexpat
