testbindir = ${prefix}/test-bin
testbin_PROGRAMS = file_blocks block_backend random_file node_updater way_updater relation_updater complete_updater diff_updater dump_database test_dispatcher example_queries id_query print recurse query bbox_query foreach union benchmark
dist_testbin_SCRIPTS = apply_osc.test.sh

statements_cc = ../statements/statement.cc ../statements/bbox_query.cc ../statements/foreach.cc ../statements/id_query.cc ../statements/osm_script.cc ../statements/print.cc ../statements/query.cc ../statements/recurse.cc ../statements/union.cc 

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

bbox_query_SOURCES = ../statements/bbox_query.test.cc ${statements_cc} ../core/settings.cc
bbox_query_LDADD = -lrt
foreach_SOURCES = ../statements/foreach.test.cc ${statements_cc} ../core/settings.cc
foreach_LDADD = -lrt
id_query_SOURCES = ../statements/id_query.test.cc ${statements_cc} ../core/settings.cc
id_query_LDADD = -lrt
print_SOURCES = ../statements/print.test.cc ${statements_cc} ../core/settings.cc
print_LDADD = -lrt
query_SOURCES = ../statements/query.test.cc ${statements_cc} ../core/settings.cc
query_LDADD = -lrt
recurse_SOURCES = ../statements/recurse.test.cc ${statements_cc} ../core/settings.cc
recurse_LDADD = -lrt
union_SOURCES = ../statements/union.test.cc ${statements_cc} ../core/settings.cc
union_LDADD = -lrt
benchmark_SOURCES = ../statements/benchmark.cc ${statements_cc} ../core/settings.cc
benchmark_LDADD = -lrt
test_dispatcher_SOURCES = ../dispatch/dispatcher.test.cc ../core/settings.cc
test_dispatcher_LDADD = -lrt
