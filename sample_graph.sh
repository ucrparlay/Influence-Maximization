#!/bin/bash
# FILE_OUT=/home/lwang323/IM/data/sample_scc_plain.txt
FILE_OUT=/home/lwang323/IM/data/sample_scc_local1.txt
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
twitter \
sd_arc \
HT_5 \
Household.lines_5 \
CHEM_5 \
GeoLifeNoScale_5 \
Cosmo50_5 \
grid_4000_4000 \
grid_1000_10000 \
grid_4000_4000_03 \
grid_1000_10000_03 \
"


COMMOND=scc 
ulimit -s unlimited
# WIC
echo "WIC"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -status -local_reach >> $FILE_OUT
    # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin $FILE_OUT_DIR/WIC/$g\_wic
    # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -static -t 1 >> $FILE_OUT_DIR/$g.wic
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin -status -local_reach >> $FILE_OUT
    # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin $FILE_OUT_DIR/WIC/$g\_wic
    # numactl -i all /home/lwang323/IM/scc $BINARY_DATA_PATH_LARGE/$g.bin -static -t 1 >> $FILE_OUT_DIR/$g.wic
done

#UIC w = 0.2
echo "UIC w = 0.2"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -status -w 0.2 -local_reach >> $FILE_OUT
    # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin $FILE_OUT_DIR/UIC/$g\_20 -w 0.2
    # numactl -i all /home/lwang323/IM/scc $BINARY_DATA_PATH_SMALL/$g.bin -w 0.025 -static -t 1>> $FILE_OUT_DIR/$g.uic.25
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin -status -w 0.2 -local_reach >> $FILE_OUT
    # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin $FILE_OUT_DIR/UIC/$g\_20 -w 0.2
    # numactl -i all /home/lwang323/IM/scc $BINARY_DATA_PATH_LARGE/$g.bin -w 0.025 -static -t 1>> $FILE_OUT_DIR/$g.uic.25
done

#UIC w = 0.1
echo "UIC w = 0.1"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -status  -w 0.1 -local_reach >> $FILE_OUT
    # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin $FILE_OUT_DIR/UIC/$g\_10 -w 0.1
    # numactl -i all /home/lwang323/IM/scc $BINARY_DATA_PATH_SMALL/$g.bin -w 0.1 -static -t 1>> $FILE_OUT_DIR/$g.uic100
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin -status -w 0.1 -local_reach >> $FILE_OUT
    # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin $FILE_OUT_DIR/UIC/$g\_10 -w 0.1
    # numactl -i all /home/lwang323/IM/scc $BINARY_DATA_PATH_LARGE/$g.bin -w 0.1 -static -t 1>> $FILE_OUT_DIR/$g.uic.100
done



# #UIC w = 0.01
# echo "UIC w = 0.01"
# for g in $SMALL_GRAPH; do
#     echo "$g " >> $FILE_OUT
#     echo "$g "
#     numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin -status -local_scc -local_reach -w 0.01 >> $FILE_OUT
#     # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g.bin $FILE_OUT_DIR/UIC/$g\_1 -w 0.01
#     # numactl -i all /home/lwang323/IM/scc $BINARY_DATA_PATH_SMALL/$g.bin  -w 0.01 -static -t 1>> $FILE_OUT_DIR/$g.uic.10
# done

# for g in $LARGE_GRAPH; do
#     echo "$g " >> $FILE_OUT
#     echo "$g "
#     numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin -status -local_scc -local_reach -w 0.01 >> $FILE_OUT
#     # numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g.bin $FILE_OUT_DIR/UIC/$g\_1 -w 0.01
#     # numactl -i all /home/lwang323/IM/scc $BINARY_DATA_PATH_LARGE/$g.bin  -w 0.01 -static -t 1>> $FILE_OUT_DIR/$g.uic.10
# done