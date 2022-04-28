#!/bin/bash
FILE_OUT=/home/lwang323/IM/data/sample_graph.txt
FILE_OUT_DIR=//home/lwang323/IM/data
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
GeoLifeNoScale_5 \
grid_4000_4000 \
grid_4000_4000_03 \
grid_1000_10000 \
grid_1000_10000_03 \
twitter \
sd_arc \
Cosmo50_5 \
"

# WIC
echo "WIC"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_SMALL/$g.bin $BINARY_DATA_PATH_SMALL/WIC/$g.wic >> $FILE_OUT
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_LARGE/$g.bin $BINARY_DATA_PATH_SMALL/WIC/$g.wic >> $FILE_OUT
done

#UIC w = 0.1
echo "UIC w = 0.1"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_SMALL/$g.bin $BINARY_DATA_PATH_SMALL/UIC/$g.uic100 -w 0.1 >> $FILE_OUT
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_LARGE/$g.bin $BINARY_DATA_PATH_SMALL/UIC/$g.uic100 -w 0.1 >> $FILE_OUT
done

#UIC w = 0.025
echo "UIC w = 0.025"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_SMALL/$g.bin $BINARY_DATA_PATH_SMALL/UIC/$g.uic25 -w 0.025 >> $FILE_OUT
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_LARGE/$g.bin $BINARY_DATA_PATH_SMALL/UIC/$g.uic25 -w 0.025 >> $FILE_OUT
done

#UIC w = 0.01
echo "UIC w = 0.01"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_SMALL/$g.bin $BINARY_DATA_PATH_SMALL/UIC/$g.uic10 -w 0.01 >> $FILE_OUT
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/graph $BINARY_DATA_PATH_LARGE/$g.bin $BINARY_DATA_PATH_SMALL/UIC/$g.uic10 -w 0.01 >> $FILE_OUT
done