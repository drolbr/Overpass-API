testbindir = ${prefix}/test-bin
testbin_PROGRAMS = file_blocks block_backend random_file node_updater way_updater relation_updater dump_database compare_osm_base_maps generate_test_file diff_updater test_dispatcher area_query bbox_query foreach make_area print query recurse union
dist_testbin_SCRIPTS = apply_osc.test.sh run_testsuite.sh run_testsuite_template_db.sh run_testsuite_osm_backend.sh run_unittests_statements.sh run_and_compare.sh

expat_cc = ../expat/expat_justparse_interface.cc
settings_cc = ../overpass_api/core/settings.cc
output_cc = ../overpass_api/frontend/output.cc
statements_dir = ../overpass_api/statements
statements_cc = ${statements_dir}/statement.cc ${statements_dir}/area_query.cc ${statements_dir}/bbox_query.cc ${statements_dir}/coord_query.cc ${statements_dir}/foreach.cc ${statements_dir}/id_query.cc ${statements_dir}/item.cc ${statements_dir}/make_area.cc ${statements_dir}/osm_script.cc ${statements_dir}/print.cc ${statements_dir}/query.cc ${statements_dir}/recurse.cc ${statements_dir}/union.cc 

testenv_cc = ${settings_cc} ../overpass_api/osm-backend/stopwatch.cc ../overpass_api/dispatch/resource_manager.cc ../overpass_api/frontend/console_output.cc ../overpass_api/frontend/user_interface.cc ../overpass_api/frontend/output.cc ../overpass_api/frontend/cgi-helper.cc

file_blocks_SOURCES = ../template_db/file_blocks.test.cc
block_backend_SOURCES = ../template_db/block_backend.test.cc
random_file_SOURCES = ../template_db/random_file.test.cc

node_updater_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/node_updater.test.cc
node_updater_LDADD = -lexpat
way_updater_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/way_updater.cc ../overpass_api/osm-backend/way_updater.test.cc
way_updater_LDADD = -lexpat
relation_updater_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/way_updater.cc ../overpass_api/osm-backend/relation_updater.cc ../overpass_api/osm-backend/relation_updater.test.cc
relation_updater_LDADD = -lexpat
#complete_updater_SOURCES = ${expat_cc} ${settings_cc} ../overpass_api/osm-backend/complete_updater.test.cc 
#complete_updater_LDADD = -lexpat
diff_updater_SOURCES = ${settings_cc} ../overpass_api/osm-backend/diff_updater.test.cc
diff_updater_LDADD =
compare_osm_base_maps_SOURCES = ${settings_cc} ../overpass_api/osm-backend/compare_osm_base_maps.test.cc
compare_osm_base_maps_LDADD =
dump_database_SOURCES = ${expat_cc} ${settings_cc} ${output_cc} ../overpass_api/osm-backend/node_updater.cc ../overpass_api/osm-backend/way_updater.cc ../overpass_api/osm-backend/relation_updater.cc ../overpass_api/osm-backend/dump_database.test.cc
dump_database_LDADD = -lexpat
#example_queries_SOURCES = ${expat_cc} ${settings_cc} ../overpass_api/osm-backend/example_queries.cc
#example_queries_LDADD = -lexpat
generate_test_file_SOURCES = ../overpass_api/osm-backend/generate_test_file.cc

area_query_SOURCES = ${statements_dir}/area_query.test.cc ${statements_cc} ${testenv_cc}
area_query_LDADD = -lrt
bbox_query_SOURCES = ${statements_dir}/bbox_query.test.cc ${statements_cc} ${testenv_cc}
bbox_query_LDADD = -lrt
foreach_SOURCES = ${statements_dir}/foreach.test.cc ${statements_cc} ${testenv_cc}
foreach_LDADD = -lrt
#id_query_SOURCES = ${statements_dir}/id_query.test.cc ${statements_cc} ${testenv_cc}
#id_query_LDADD = -lrt
make_area_SOURCES = ${statements_dir}/make_area.test.cc ${statements_cc} ${testenv_cc}
make_area_LDADD = -lrt
print_SOURCES = ${statements_dir}/print.test.cc ${statements_cc} ${testenv_cc}
print_LDADD = -lrt
query_SOURCES = ${statements_dir}/query.test.cc ${statements_cc} ${testenv_cc}
query_LDADD = -lrt
recurse_SOURCES = ${statements_dir}/recurse.test.cc ${statements_cc} ${testenv_cc}
recurse_LDADD = -lrt
union_SOURCES = ${statements_dir}/union.test.cc ${statements_cc} ${testenv_cc}
union_LDADD = -lrt
#benchmark_SOURCES = ${statements_dir}/benchmark.cc ${statements_cc} ${testenv_cc}
#benchmark_LDADD = -lrt
test_dispatcher_SOURCES = ../template_db/dispatcher.test.cc ../template_db/dispatcher.cc
test_dispatcher_LDADD = -lrt
