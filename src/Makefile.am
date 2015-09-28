
bin_mandatory = bin/osm3s_query bin/dispatcher bin/update_database bin/update_from_dir
bin_script_mandatory = bin/apply_osc_to_db.sh bin/fetch_osc.sh bin/rules_loop.sh bin/download_clone.sh
cgi_bin_mandatory = cgi-bin/interpreter cgi-bin/timestamp
cgi_bin_script_mandatory = cgi-bin/kill_my_queries cgi-bin/map

bin_customoutput = bin/uncgi bin/escape_xml
bin_script_customoutput =
cgi_bin_customoutput =
cgi_bin_script_customoutput = cgi-bin/template

bin_clone =
bin_script_clone = bin/compress_clone.sh bin/clone.sh
cgi_bin_clone =
cgi_bin_script_clone = cgi-bin/trigger_clone

bin_augmented_diffs = bin/process_augmented_diffs bin/uncgi
bin_script_augmented_diffs =
cgi_bin_augmented_diffs =
cgi_bin_script_augmented_diffs = cgi-bin/augmented_diff cgi-bin/augmented_diff_status cgi-bin/augmented_state_by_date

bin_xapi_layer = bin/translate_xapi
bin_script_xapi_layer = bin/cleanup_xapi_tmp.sh
cgi_bin_xapi_layer = 
cgi_bin_script_xapi_layer = cgi-bin/convert_xapi cgi-bin/xapi cgi-bin/xapi_meta

bin_openlayers = bin/tocgi bin/uncgi bin/escape_xml
bin_script_openlayers =
cgi_bin_openlayers =
cgi_bin_script_openlayers = cgi-bin/convert

bin_public_transport = bin/sketch_route_svg bin/draw_route_svg bin/bbox_brim_query bin/uncgi bin/escape_xml
bin_script_public_transport =
cgi_bin_public_transport =
cgi_bin_script_public_transport = cgi-bin/draw-line cgi-bin/sketch-line cgi-bin/sketch-options cgi-bin/sketch-route

bin_convenience =
bin_script_convenience = bin/fetch_osc_and_apply.sh bin/init_osm3s.sh bin/run_osm3s_minutely.sh
cgi_bin_convenience =
cgi_bin_script_convenience = cgi-bin/ping cgi-bin/sitemap

bindir = ${prefix}/bin
bin_PROGRAMS = \
  ${bin_mandatory} \
  ${bin_customoutput} \
  ${bin_clone} \
  ${bin_augmented_diffs} \
  ${bin_xapi_layer} \
  ${bin_openlayers} \
  ${bin_public_transport} \
  ${bin_convenience}
  
dist_bin_SCRIPTS = \
  ${bin_script_mandatory} \
  ${bin_script_customoutput} \
  ${bin_script_clone} \
  ${bin_script_augmented_diffs} \
  ${bin_script_xapi_layer} \
  ${bin_script_openlayers} \
  ${bin_script_public_transport} \
  ${bin_script_convenience}
  

cgibindir = ${prefix}/cgi-bin
cgibin_PROGRAMS = \
  ${cgi_bin_mandatory} \
  ${cgi_bin_customoutput} \
  ${cgi_bin_clone} \
  ${cgi_bin_augmented_diffs} \
  ${cgi_bin_xapi_layer} \
  ${cgi_bin_openlayers} \
  ${cgi_bin_public_transport} \
  ${cgi_bin_convenience}

dist_cgibin_SCRIPTS = \
  ${cgi_bin_script_mandatory} \
  ${cgi_bin_script_customoutput} \
  ${cgi_bin_script_clone} \
  ${cgi_bin_script_augmented_diffs} \
  ${cgi_bin_script_xapi_layer} \
  ${cgi_bin_script_openlayers} \
  ${cgi_bin_script_public_transport} \
  ${cgi_bin_script_convenience}

templatesdir = ${prefix}/templates
dist_templates_DATA = templates/default.wiki templates/base.wiki

noinst_LTLIBRARIES = libcore.la libdata.la libdispatcher.la libexpatwrapper.la libfrontend.la liboutput.la libsettings.la libweboutput.la

statements_cc = \
overpass_api/statements/statement.cc \
overpass_api/statements/area_query.cc \
overpass_api/statements/around.cc \
overpass_api/statements/bbox_query.cc \
overpass_api/statements/changed.cc \
overpass_api/statements/coord_query.cc \
overpass_api/statements/difference.cc \
overpass_api/statements/foreach.cc \
overpass_api/statements/id_query.cc \
overpass_api/statements/item.cc \
overpass_api/statements/make_area.cc \
overpass_api/statements/map_to_area.cc \
overpass_api/statements/newer.cc \
overpass_api/statements/osm_script.cc \
overpass_api/statements/pivot.cc \
overpass_api/statements/polygon_query.cc \
overpass_api/statements/print.cc \
overpass_api/statements/query.cc \
overpass_api/statements/recurse.cc \
overpass_api/statements/union.cc \
overpass_api/statements/user.cc \
overpass_api/frontend/print_target.cc expat/escape_xml.cc

libcore_la_SOURCES = overpass_api/frontend/map_ql_parser.cc overpass_api/statements/statement_dump.cc expat/map_ql_input.cc
libcore_la_LIBADD = libdispatcher.la libexpatwrapper.la libsettings.la libweboutput.la
libdata_la_SOURCES = overpass_api/data/collect_members.cc
libdata_la_LIBADD =
libdispatcher_la_SOURCES = template_db/dispatcher_client.cc overpass_api/dispatch/resource_manager.cc overpass_api/osm-backend/area_updater.cc
libdispatcher_la_LIBADD = -lrt
libexpatwrapper_la_SOURCES = expat/expat_justparse_interface.cc
libexpatwrapper_la_LIBADD = -lexpat
libfrontend_la_SOURCES = overpass_api/frontend/cgi-helper.cc overpass_api/frontend/user_interface.cc
libfrontend_la_LIBADD = liboutput.la
liboutput_la_SOURCES = overpass_api/frontend/output.cc
liboutput_la_LIBADD =
libsettings_la_SOURCES = overpass_api/core/settings.cc
libsettings_la_LIBADD =
# libstatements_la_SOURCES = ${statements_cc}
# libstatements_la_LIBADD = libdata.la
libweboutput_la_SOURCES = overpass_api/frontend/web_output.cc
libweboutput_la_LIBADD = libfrontend.la

osm_updater_cc = overpass_api/osm-backend/meta_updater.cc overpass_api/osm-backend/basic_updater.cc overpass_api/osm-backend/node_updater.cc overpass_api/osm-backend/way_updater.cc overpass_api/osm-backend/relation_updater.cc overpass_api/osm-backend/osm_updater.cc expat/escape_xml.cc


bin_update_database_SOURCES = ${osm_updater_cc} overpass_api/osm-backend/update_database.cc template_db/types.cc template_db/zlib_wrapper.cc
bin_update_database_LDADD = libdata.la libdispatcher.la libexpatwrapper.la liboutput.la libsettings.la -lz
bin_update_from_dir_SOURCES = ${osm_updater_cc} overpass_api/osm-backend/update_from_dir.cc template_db/types.cc template_db/zlib_wrapper.cc
bin_update_from_dir_LDADD = libdata.la libdispatcher.la libexpatwrapper.la liboutput.la libsettings.la -lz
bin_osm3s_query_SOURCES = ${statements_cc} overpass_api/frontend/console_output.cc overpass_api/dispatch/osm3s_query.cc overpass_api/osm-backend/clone_database.cc overpass_api/dispatch/scripting_core.cc overpass_api/dispatch/dispatcher_stub.cc template_db/types.cc template_db/zlib_wrapper.cc
bin_osm3s_query_LDADD = libcore.la libdata.la -lz
bin_dispatcher_SOURCES = template_db/dispatcher.cc template_db/file_tools.cc template_db/types.cc overpass_api/dispatch/dispatcher_server.cc
bin_dispatcher_LDADD = libdispatcher.la libfrontend.la libsettings.la


cgi_bin_interpreter_SOURCES = ${statements_cc} overpass_api/dispatch/web_query.cc overpass_api/dispatch/scripting_core.cc overpass_api/dispatch/dispatcher_stub.cc template_db/types.cc template_db/zlib_wrapper.cc
cgi_bin_interpreter_LDADD = libcore.la libdata.la -lz
cgi_bin_timestamp_SOURCES = overpass_api/dispatch/db_timestamp.cc overpass_api/dispatch/dispatcher_stub.cc template_db/types.cc template_db/zlib_wrapper.cc
cgi_bin_timestamp_LDADD = libdispatcher.la libsettings.la libweboutput.la -lz


escape_xml_cc = expat/escape_xml.cc

bin_sketch_route_svg_SOURCES = ${escape_xml_cc} pt_diagrams/sketch-route-svg.cc pt_diagrams/processed_input.cc
bin_sketch_route_svg_LDADD = libexpatwrapper.la
bin_draw_route_svg_SOURCES = ${escape_xml_cc} pt_diagrams/read_input.cc pt_diagrams/topographic.cc
bin_draw_route_svg_LDADD = libexpatwrapper.la
bin_uncgi_SOURCES = pt_diagrams/uncgi.cc
bin_uncgi_LDADD =
bin_tocgi_SOURCES = pt_diagrams/tocgi.cc
bin_tocgi_LDADD =
bin_escape_xml_SOURCES = expat/escape_xml.cc expat/escape_xml_main.cc
bin_escape_xml_LDADD =
bin_bbox_brim_query_SOURCES = pt_diagrams/bbox-brim-query.cc
bin_bbox_brim_query_LDADD = libexpatwrapper.la
bin_translate_xapi_SOURCES = expat/translate_xapi.cc expat/escape_xml.cc
bin_translate_xapi_LDADD = 
bin_process_augmented_diffs_SOURCES = expat/process_augmented_diffs.cc expat/escape_xml.cc
bin_process_augmented_diffs_LDADD = libexpatwrapper.la

distdir = osm-3s_v0.7.52

nobase_dist_HEADERS = pt_diagrams/test_output.h pt_diagrams/processed_input.h pt_diagrams/read_input.h expat/map_ql_input.h expat/escape_xml.h expat/expat_justparse_interface.h overpass_api/frontend/cgi-helper.h overpass_api/frontend/web_output.h overpass_api/frontend/user_interface.h overpass_api/frontend/output.h overpass_api/frontend/console_output.h overpass_api/frontend/print_target.h overpass_api/frontend/map_ql_parser.h overpass_api/data/abstract_processing.h overpass_api/data/collect_items.h overpass_api/data/filter_ids_by_tags.h overpass_api/data/regular_expression.h overpass_api/data/geometry.h overpass_api/data/collect_members.h overpass_api/data/filter_by_tags.h overpass_api/data/filenames.h overpass_api/dispatch/dispatcher_stub.h overpass_api/dispatch/resource_manager.h overpass_api/dispatch/scripting_core.h overpass_api/core/settings.h overpass_api/core/type_tags.h overpass_api/core/datatypes.h overpass_api/core/type_relation.h overpass_api/core/type_area.h overpass_api/core/basic_types.h overpass_api/core/type_way.h overpass_api/core/type_node.h overpass_api/core/index_computations.h overpass_api/statements/user.h overpass_api/statements/map_to_area.h overpass_api/statements/coord_query.h overpass_api/statements/recurse.h overpass_api/statements/difference.h overpass_api/statements/bbox_query.h overpass_api/statements/polygon_query.h overpass_api/statements/print.h overpass_api/statements/query.h overpass_api/statements/union.h overpass_api/statements/item.h overpass_api/statements/make_area.h overpass_api/statements/around.h overpass_api/statements/pivot.h overpass_api/statements/id_query.h overpass_api/statements/area_query.h overpass_api/statements/statement.h overpass_api/statements/osm_script.h overpass_api/statements/newer.h overpass_api/statements/meta_collector.h overpass_api/statements/statement_dump.h overpass_api/statements/foreach.h overpass_api/statements/changed.h overpass_api/osm-backend/basic_updater.h overpass_api/osm-backend/osm_updater.h overpass_api/osm-backend/clone_database.h overpass_api/osm-backend/node_updater.h overpass_api/osm-backend/way_updater.h overpass_api/osm-backend/tags_updater.h overpass_api/osm-backend/meta_updater.h overpass_api/osm-backend/area_updater.h overpass_api/osm-backend/relation_updater.h template_db/dispatcher_client.h template_db/file_blocks_index.h template_db/file_blocks.h template_db/block_backend.h template_db/dispatcher.h template_db/zlib_wrapper.h template_db/transaction.h template_db/random_file_index.h template_db/types.h template_db/random_file.h template_db/file_tools.h lib/overpass.h

EXTRA_DIST = ../html/command_line.html ../html/full_installation.html ../html/index.html ../html/no_frills.html ../html/open_layers_bbox.html ../html/open_layers_fixed.html ../html/open_layers_mashup.html ../html/open_layers_multilayer.html ../html/open_layers_popup.html ../html/output_formats.html ../html/overpass.js ../html/public_transport.html ../html/query_form.html ../html/technical_overview.html

# Put test-bin here and test-bin/Makefile in configure.ac to activate test-bin
SUBDIRS = test-bin
#SUBDIRS =
