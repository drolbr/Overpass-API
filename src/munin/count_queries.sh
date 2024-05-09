#!/usr/bin/env bash

tail -f /OSM_DB_DIR/transactions.log | ./continuously_count_lines.sh # adapt directory
