rm -f src/duckdb.cpp src/duckdb.h src/duckdb.o src/duckdbr.o src/duckdb.so src/Makevars
rm -rf src/duckdb
R -q -e "if (is.element('duckdb', installed.packages()[,1])) { remove.packages('duckdb') }"
rm -f duckdb_*.tar.gz
