testbindir = ${prefix}/test-bin
testbin_PROGRAMS = file_blocks around block_backend random_file node_updater way_updater relation_updater dump_database compare_osm_base_maps generate_test_file diff_updater test_dispatcher area_query bbox_query complete difference foreach convert if make make_area polygon_query print query recurse union generate_test_file_areas generate_test_file_meta generate_test_file_interpreter index_computations four_field_index consistency_check
dist_testbin_SCRIPTS = apply_osc.test.sh run_testsuite.sh run_testsuite_template_db.sh run_testsuite_osm_backend.sh run_unittests_statements.sh run_testsuite_osm3s_query.sh run_testsuite_map_ql.sh run_testsuite_interpreter.sh run_testsuite_translate_xapi.sh run_testsuite_diff_updater.sh run_unittests_areas.sh run_unittests_implicit_areas.sh run_unittests_meta.sh run_unittests_attic.sh run_unittests_output_csv.sh run_unittests_vlt.sh run_and_compare.sh

expat_cc = ../expat/expat_justparse_interface.cc
settings_cc = ../overpass_api/core/settings.cc
output_cc = ../overpass_api/frontend/output.cc
statements_cc = \
  ../expat/escape_json.cc \
  ../expat/escape_xml.cc \
  ../overpass_api/core/four_field_index.cc \
  ../overpass_api/core/geometry.cc \
  ../overpass_api/data/bbox_filter.cc \
  ../overpass_api/data/collect_members.cc \
  ../overpass_api/data/diff_set.cc \
  ../overpass_api/data/geometry_from_quad_coords.cc \
  ../overpass_api/data/ranges.inst.cc \
  ../overpass_api/data/relation_geometry_store.cc \
  ../overpass_api/data/set_comparison.cc \
  ../overpass_api/data/way_geometry_store.cc \
  ../overpass_api/frontend/output_handler_parser.cc \
  ../overpass_api/osm-backend/area_updater.cc \
  ../overpass_api/statements/aggregators.cc \
  ../overpass_api/statements/area_query.cc \
  ../overpass_api/statements/around.cc \
  ../overpass_api/statements/bbox_query.cc \
  ../overpass_api/statements/binary_operators.cc \
  ../overpass_api/statements/changed.cc \
  ../overpass_api/statements/compare.cc \
  ../overpass_api/statements/complete.cc \
  ../overpass_api/statements/convert.cc \
  ../overpass_api/statements/coord_query.cc \
  ../overpass_api/statements/difference.cc \
  ../overpass_api/statements/evaluator.cc \
  ../overpass_api/statements/explicit_geometry.cc \
  ../overpass_api/statements/filter.cc \
  ../overpass_api/statements/for.cc \
  ../overpass_api/statements/foreach.cc \
  ../overpass_api/statements/geometry_endomorphisms.cc \
  ../overpass_api/statements/id_query.cc \
  ../overpass_api/statements/if.cc \
  ../overpass_api/statements/item.cc \
  ../overpass_api/statements/item_geometry.cc \
  ../overpass_api/statements/localize.cc \
  ../overpass_api/statements/make_area.cc \
  ../overpass_api/statements/make.cc \
  ../overpass_api/statements/map_to_area.cc \
  ../overpass_api/statements/newer.cc \
  ../overpass_api/statements/osm_script.cc \
  ../overpass_api/statements/per_member.cc \
  ../overpass_api/statements/pivot.cc \
  ../overpass_api/statements/polygon_query.cc \
  ../overpass_api/statements/print.cc \
  ../overpass_api/statements/query.cc \
  ../overpass_api/statements/recurse.cc \
  ../overpass_api/statements/retro.cc \
  ../overpass_api/statements/runtime_value.cc \
  ../overpass_api/statements/set_list_operators.cc \
  ../overpass_api/statements/set_prop.cc \
  ../overpass_api/statements/statement.cc \
  ../overpass_api/statements/string_endomorphisms.cc \
  ../overpass_api/statements/tag_value.cc \
  ../overpass_api/statements/ternary_operator.cc \
  ../overpass_api/statements/testing_tools.cc \
  ../overpass_api/statements/timeline.cc \
  ../overpass_api/statements/unary_functions.cc \
  ../overpass_api/statements/unary_operators.cc \
  ../overpass_api/statements/union.cc \
  ../overpass_api/statements/user.cc \
  ../template_db/lz4_wrapper.cc \
  ../template_db/types.cc \
  ../template_db/zlib_wrapper.cc

output_formats_dir = ../overpass_api/output_formats

testenv_cc = ${settings_cc} ../overpass_api/dispatch/resource_manager.cc ../overpass_api/frontend/console_output.cc ../overpass_api/frontend/user_interface.cc ../overpass_api/frontend/output.cc ../overpass_api/frontend/basic_formats.cc ../overpass_api/frontend/cgi-helper.cc ../overpass_api/frontend/decode_text.cc ../overpass_api/frontend/output_handler.cc ../overpass_api/frontend/tokenizer_utils.cc ../expat/map_ql_input.cc ${output_formats_dir}/output_xml.cc ${output_formats_dir}/output_xml_factory.cc

file_blocks_SOURCES = ../template_db/file_blocks.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
file_blocks_LDADD = @COMPRESS_LIBS@

block_backend_SOURCES = ../template_db/block_backend.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
block_backend_LDADD = @COMPRESS_LIBS@

random_file_SOURCES = ../template_db/random_file.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
random_file_LDADD = @COMPRESS_LIBS@

node_updater_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/data/ranges.inst.cc ../overpass_api/osm-backend/area_updater.cc ../overpass_api/osm-backend/meta_updater.cc ../overpass_api/osm-backend/basic_updater.cc ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/node_updater.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
node_updater_LDADD = -lexpat @COMPRESS_LIBS@
way_updater_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/data/ranges.inst.cc ../overpass_api/osm-backend/area_updater.cc ../overpass_api/osm-backend/meta_updater.cc ../overpass_api/osm-backend/basic_updater.cc ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/way_updater.cc ../overpass_api/osm-backend/way_updater.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
way_updater_LDADD = -lexpat @COMPRESS_LIBS@
relation_updater_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/data/ranges.inst.cc ../overpass_api/osm-backend/area_updater.cc ../overpass_api/osm-backend/meta_updater.cc ../overpass_api/osm-backend/basic_updater.cc ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/way_updater.cc ../overpass_api/osm-backend/relation_updater.cc ../overpass_api/osm-backend/relation_updater.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
relation_updater_LDADD = -lexpat @COMPRESS_LIBS@
#complete_updater_SOURCES = ${expat_cc} ${settings_cc} ../overpass_api/osm-backend/complete_updater.test.cc 
#complete_updater_LDADD = -lexpat
diff_updater_SOURCES = ${settings_cc} ../overpass_api/data/ranges.inst.cc ../overpass_api/osm-backend/diff_updater.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
diff_updater_LDADD = @COMPRESS_LIBS@
compare_osm_base_maps_SOURCES = ${settings_cc} ../overpass_api/osm-backend/compare_osm_base_maps.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
compare_osm_base_maps_LDADD = @COMPRESS_LIBS@
dump_database_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/data/ranges.inst.cc ../overpass_api/osm-backend/area_updater.cc ../overpass_api/osm-backend/meta_updater.cc ../overpass_api/osm-backend/basic_updater.cc ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/way_updater.cc ../overpass_api/osm-backend/relation_updater.cc ../overpass_api/osm-backend/dump_database.test.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
dump_database_LDADD = -lexpat @COMPRESS_LIBS@
consistency_check_SOURCES = ../overpass_api/dispatch/consistency_check.cc ${statements_cc} ${testenv_cc} ../overpass_api/dispatch/scripting_core.cc ../overpass_api/dispatch/dispatcher_stub.cc ../overpass_api/frontend/map_ql_parser.cc ../overpass_api/frontend/hash_request.cc ../overpass_api/statements/statement_dump.cc ../template_db/dispatcher_client.cc
# consistency_check_SOURCES = ../overpass_api/dispatch/consistency_check.cc ${statements_cc} ../overpass_api/core/settings.cc ../overpass_api/frontend/console_output.cc ../overpass_api/dispatch/scripting_core.cc ../template_db/dispatcher.cc
consistency_check_LDADD = -lexpat @COMPRESS_LIBS@
#example_queries_SOURCES = ${expat_cc} ${settings_cc} ../overpass_api/osm-backend/example_queries.cc
#example_queries_LDADD = -lexpat
generate_test_file_SOURCES = ../overpass_api/osm-backend/generate_test_file.cc
generate_test_file_areas_SOURCES = ../overpass_api/osm-backend/generate_test_file_areas.cc
generate_test_file_interpreter_SOURCES = ../overpass_api/osm-backend/generate_test_file_interpreter.cc ../overpass_api/core/settings.cc
generate_test_file_meta_SOURCES = ../overpass_api/osm-backend/generate_test_file_meta.cc ../overpass_api/core/settings.cc
index_computations_SOURCES = ../overpass_api/core/index_computations.test.cc ../overpass_api/data/ranges.inst.cc
index_computations_LDADD =
four_field_index_SOURCES = ../overpass_api/core/four_field_index.cc ../overpass_api/core/four_field_index.test.cc
four_field_index_LDADD =

area_query_SOURCES = ../overpass_api/statements/area_query.test.cc ${statements_cc} ${testenv_cc}
area_query_LDADD = @COMPRESS_LIBS@
around_SOURCES = ../overpass_api/statements/around.test.cc ${statements_cc} ${testenv_cc}
around_LDADD = @COMPRESS_LIBS@
bbox_query_SOURCES = ../overpass_api/statements/bbox_query.test.cc ${statements_cc} ${testenv_cc}
bbox_query_LDADD = @COMPRESS_LIBS@
complete_SOURCES = ../overpass_api/statements/complete.test.cc ${statements_cc} ${testenv_cc}
complete_LDADD = @COMPRESS_LIBS@
convert_SOURCES = ../overpass_api/statements/convert.test.cc ${statements_cc} ${testenv_cc}
convert_LDADD = @COMPRESS_LIBS@
difference_SOURCES = ../overpass_api/statements/difference.test.cc ${statements_cc} ${testenv_cc}
difference_LDADD = @COMPRESS_LIBS@
foreach_SOURCES = ../overpass_api/statements/foreach.test.cc ${statements_cc} ${testenv_cc}
foreach_LDADD = @COMPRESS_LIBS@
if_SOURCES = ../overpass_api/statements/if.test.cc ${statements_cc} ${testenv_cc}
if_LDADD = @COMPRESS_LIBS@
#id_query_SOURCES = ../overpass_api/statements/id_query.test.cc ${statements_cc} ${testenv_cc}
#id_query_LDADD = 
make_SOURCES = ../overpass_api/statements/make.test.cc ${statements_cc} ${testenv_cc}
make_LDADD = @COMPRESS_LIBS@
make_area_SOURCES = ../overpass_api/statements/make_area.test.cc ${statements_cc} ${testenv_cc}
make_area_LDADD = @COMPRESS_LIBS@
polygon_query_SOURCES = ../overpass_api/statements/polygon_query.test.cc ${statements_cc} ${testenv_cc}
polygon_query_LDADD = @COMPRESS_LIBS@
print_SOURCES = ../overpass_api/statements/print.test.cc ${statements_cc} ${testenv_cc}
print_LDADD = @COMPRESS_LIBS@
query_SOURCES = ../overpass_api/statements/query.test.cc ${statements_cc} ${testenv_cc}
query_LDADD = @COMPRESS_LIBS@
recurse_SOURCES = ../overpass_api/statements/recurse.test.cc ${statements_cc} ${testenv_cc}
recurse_LDADD = @COMPRESS_LIBS@
union_SOURCES = ../overpass_api/statements/union.test.cc ${statements_cc} ${testenv_cc}
union_LDADD = @COMPRESS_LIBS@
#benchmark_SOURCES = ../overpass_api/statements/benchmark.cc ${statements_cc} ${testenv_cc}
#benchmark_LDADD = 
test_dispatcher_SOURCES = ../template_db/dispatcher.test.cc ../template_db/dispatcher_client.cc ../template_db/dispatcher.cc ../template_db/file_tools.cc ../template_db/transaction_insulator.cc ../template_db/types.cc ../template_db/zlib_wrapper.cc ../template_db/lz4_wrapper.cc
test_dispatcher_LDADD = @COMPRESS_LIBS@

AM_CXXFLAGS = -std=c++11


distdir = osm-3s_v0.7.57.1

nobase_dist_HEADERS = \
  ../overpass_api/statements/testing_tools.h
