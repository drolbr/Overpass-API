testbindir = ${prefix}/test-bin
testbin_PROGRAMS = file_blocks block_backend random_file node_updater way_updater relation_updater complete_updater diff_updater dump_database test_dispatcher example_queries area_query bbox_query foreach id_query make_area print query recurse union benchmark
dist_testbin_SCRIPTS = apply_osc.test.sh

statements_cc = ../statements/statement.cc ../statements/area_query.cc ../statements/bbox_query.cc ../statements/coord_query.cc ../statements/foreach.cc ../statements/id_query.cc ../statements/item.cc  ../statements/make_area.cc ../statements/osm_script.cc ../statements/print.cc ../statements/query.cc ../statements/recurse.cc ../statements/union.cc

testenv_cc = ../osm-backend/stopwatch.cc ../dispatch/resource_manager.cc ../core/settings.cc ../frontend/console_output.cc

file_blocks_SOURCES = ../backend/file_blocks.test.cc
block_backend_SOURCES = ../backend/block_backend.test.cc
random_file_SOURCES = ../backend/random_file.test.cc

node_updater_SOURCES = ../osm-backend/node_updater.test.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
node_updater_LDADD = -lexpat
way_updater_SOURCES = ../osm-backend/way_updater.test.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
way_updater_LDADD = -lexpat
relation_updater_SOURCES = ../osm-backend/relation_updater.test.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
relation_updater_LDADD = -lexpat
complete_updater_SOURCES = ../osm-backend/complete_updater.test.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
complete_updater_LDADD = -lexpat
diff_updater_SOURCES = ../osm-backend/diff_updater.test.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
diff_updater_LDADD = -lexpat
dump_database_SOURCES = ../osm-backend/dump_database.test.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
dump_database_LDADD = -lexpat
example_queries_SOURCES = ../osm-backend/example_queries.cc ../core/settings.cc ../expat/expat_justparse_interface.cc
example_queries_LDADD = -lexpat

area_query_SOURCES = ../statements/area_query.test.cc ${statements_cc} ${testenv_cc}
area_query_LDADD = -lrt
bbox_query_SOURCES = ../statements/bbox_query.test.cc ${statements_cc} ${testenv_cc}
bbox_query_LDADD = -lrt
foreach_SOURCES = ../statements/foreach.test.cc ${statements_cc} ${testenv_cc}
foreach_LDADD = -lrt
id_query_SOURCES = ../statements/id_query.test.cc ${statements_cc} ${testenv_cc}
id_query_LDADD = -lrt
make_area_SOURCES = ../statements/make_area.test.cc ${statements_cc} ${testenv_cc}
make_area_LDADD = -lrt
print_SOURCES = ../statements/print.test.cc ${statements_cc} ${testenv_cc}
print_LDADD = -lrt
query_SOURCES = ../statements/query.test.cc ${statements_cc} ${testenv_cc}
query_LDADD = -lrt
recurse_SOURCES = ../statements/recurse.test.cc ${statements_cc} ${testenv_cc}
recurse_LDADD = -lrt
union_SOURCES = ../statements/union.test.cc ${statements_cc} ${testenv_cc}
union_LDADD = -lrt
benchmark_SOURCES = ../statements/benchmark.cc ${statements_cc} ${testenv_cc}
benchmark_LDADD = -lrt
test_dispatcher_SOURCES = ../dispatch/dispatcher.test.cc ../core/settings.cc ../osm-backend/stopwatch.cc
test_dispatcher_LDADD = -lrt
