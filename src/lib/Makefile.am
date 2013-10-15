lib_LTLIBRARIES = liboverpass-0.1.la

escape_xml_cc = ../expat/escape_xml.cc
expat_cc = ../expat/expat_justparse_interface.cc
settings_cc = ../overpass_api/core/settings.cc
frontend_cc = ../overpass_api/frontend/user_interface.cc ../overpass_api/frontend/cgi-helper.cc ../overpass_api/frontend/output.cc
web_frontend_cc = ${frontend_cc} ../overpass_api/frontend/web_output.cc
dispatcher_cc = ../overpass_api/dispatch/scripting_core.cc ../overpass_api/frontend/map_ql_parser.cc ../overpass_api/statements/statement_dump.cc ../expat/map_ql_input.cc ../overpass_api/dispatch/resource_manager.cc ../template_db/dispatcher.cc

statements_dir = ../overpass_api/statements
statements_cc = ${statements_dir}/statement.cc ${statements_dir}/area_query.cc ${statements_dir}/around.cc ${statements_dir}/bbox_query.cc ${statements_dir}/coord_query.cc ${statements_dir}/difference.cc ${statements_dir}/foreach.cc ${statements_dir}/id_query.cc ${statements_dir}/item.cc ${statements_dir}/make_area.cc ${statements_dir}/newer.cc ${statements_dir}/osm_script.cc ${statements_dir}/polygon_query.cc ${statements_dir}/print.cc ${statements_dir}/query.cc ${statements_dir}/recurse.cc ${statements_dir}/union.cc ${statements_dir}/user.cc ../overpass_api/frontend/print_target.cc ../overpass_api/data/collect_members.cc

liboverpass_0_1_la_SOURCES = ${expat_cc} ${settings_cc} ${web_frontend_cc} ${statements_cc} ${dispatcher_cc} overpass.cc
liboverpass_0_1_la_LIBADD =

library_includedir = $(includedir)/overpass-0.1/overpass
library_include_HEADERS = overpass.h
