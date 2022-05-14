#!/bin/bash
FILE_OUT=/home/lwang323/IM/data/IM3_sketch.txt
MEM_OUT=/home/lwang323/IM/data/IM3_memory.txt
FILE_OUT_DIR=//home/lwang323/IM/data
BINARY_DATA_PATH_LARGE=/data/graphs/bin
BINARY_DATA_PATH_SMALL=/data/lwang323/graph/bin
SMALL_GRAPH="\
HepPh_sym.bin \
Epinions1_sym.bin \
Slashdot_sym.bin \
DBLP_sym.bin \
Youtube_sym.bin \
"

LARGE_GRAPH="\
com-orkut_sym.bin \
soc-LiveJournal1_sym.bin \
twitter_sym.bin \
friendster_sym.bin \
sd_arc_sym.bin \
RoadUSA_sym.bin \
Germany_sym.bin \
HT_5_sym.bin \
Household.lines_5_sym.bin \
CHEM_5_sym.bin \
GeoLifeNoScale_5_sym.bin \
Cosmo50_5_sym.bin \
grid_4000_4000_sym.bin \
grid_4000_4000_03_sym.bin \
grid_1000_10000_sym.bin \
grid_1000_10000_03_sym.bin \
"

COMMOND=IM

# WIC
echo "WIC"
for g in $SMALL_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    # /usr/bin/time -v numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g  >> $FILE_OUT 2>>$MEM_OUT 
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g  >> $FILE_OUT
    # $FILE_OUT_DIR/$g.wic
done

for g in $LARGE_GRAPH; do
    echo "$g " >> $FILE_OUT
    echo "$g "
    # /usr/bin/time -v numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g >> $FILE_OUT 2>>$MEM_OUT
    numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g >> $FILE_OUT
    # $FILE_OUT_DIR/$g.wic
done

# #UIC w = 0.1
# echo "UIC w = 0.1"
# for g in $SMALL_GRAPH; do
#     echo "$g " >> $FILE_OUT
#     echo "$g "
#     # /usr/bin/time -v numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g -w 0.1 >> $FILE_OUT 2>>$MEM_OUT
#     numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_SMALL/$g -w 0.1 >> $FILE_OUT
#     # $FILE_OUT_DIR/$g.uic
# done

# for g in $LARGE_GRAPH; do
#     echo "$g " >> $FILE_OUT
#     echo "$g "
#     # /usr/bin/time -v numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g -w 0.1 >> $FILE_OUT 2>>$MEM_OUT
#     numactl -i all /home/lwang323/IM/$COMMOND $BINARY_DATA_PATH_LARGE/$g -w 0.1 >> $FILE_OUT
#     # $FILE_OUT_DIR/$g.uic
# done
