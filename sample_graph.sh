#!/bin/bash
# FILE_OUT=/home/lwang323/IM/data/sample_scc_plain.txt
FILE_OUT=/home/lwang323/IM/data/pmc_results_seq.txt
# FILE_OUT=/home/lwang323/IM/data/sample_scc_final.txt
# FILE_OUT=/home/lwang323/IM/data/sample_scc_tarjan.txt
# FILE_OUT_DIR=//home/lwang323/IM/data/SCC
FILE_OUT_DIR=/data/lwang323/graph/bin
BINARY_DATA_PATH_LARGE=/data/graphs/bin
BINARY_DATA_PATH_SMALL=/data/lwang323/graph/bin
SMALL_GRAPH="\
Epinions1 \
Slashdot \
"

LARGE_GRAPH="\
soc-LiveJournal1 \
HT_5 \
Household.lines_5 \
CHEM_5 \
grid_4000_4000 \
grid_4000_4000_03 \
grid_1000_10000 \
grid_1000_10000_03 \
GeoLifeNoScale_5 \
twitter \
sd_arc \
Cosmo50_5 \
"


COMMOND=IM
ulimit -s unlimited
# WIC
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -pmc -k 200 -R 200 >> $FILE_OUT
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -w 0.1 -pmc -k 200 -R 200 >> $FILE_OUT
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -w 0.2 -pmc -k 200 -R 200 >> $FILE_OUT
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin -pmc -k 200 -R 200 >> $FILE_OUT
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin -w 0.1 -pmc -k 200 -R 200 >> $FILE_OUT
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin -w 0.2 -pmc -k 200 -R 200 >> $FILE_OUT
done
