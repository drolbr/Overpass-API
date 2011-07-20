cgibindir = ${prefix}/cgi-bin
cgibin_PROGRAMS = interpreter
dist_cgibin_SCRIPTS = draw-line sketch-line sketch-options sketch-route xapi

escape_xml_cc = ../expat/escape_xml.cc
expat_cc = ../expat/expat_justparse_interface.cc
settings_cc = ../overpass_api/core/settings.cc
frontend_cc = ../overpass_api/frontend/user_interface.cc ../overpass_api/frontend/cgi-helper.cc ../overpass_api/frontend/output.cc
web_frontend_cc = ${frontend_cc} ../overpass_api/frontend/web_output.cc
dispatcher_cc = ../overpass_api/dispatch/scripting_core.cc ../overpass_api/dispatch/resource_manager.cc ../overpass_api/osm-backend/stopwatch.cc ../template_db/dispatcher.cc

statements_dir = ../overpass_api/statements
statements_cc = ${statements_dir}/statement.cc ${statements_dir}/area_query.cc ${statements_dir}/around.cc ${statements_dir}/bbox_query.cc ${statements_dir}/coord_query.cc ${statements_dir}/foreach.cc ${statements_dir}/id_query.cc ${statements_dir}/item.cc ${statements_dir}/make_area.cc ${statements_dir}/osm_script.cc ${statements_dir}/print.cc ${statements_dir}/query.cc ${statements_dir}/recurse.cc ${statements_dir}/union.cc 

interpreter_SOURCES = ${expat_cc} ${settings_cc} ${web_frontend_cc} ${statements_cc} ${dispatcher_cc} ../overpass_api/dispatch/web_query.cc ../overpass_api/osm-backend/area_updater.cc
interpreter_LDADD = -lexpat -lrt
