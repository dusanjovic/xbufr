#!/bin/bash
set -eu

echo "Running ${BASH_SOURCE[0]}"

MYDIR=$(cd "$(dirname "${BASH_SOURCE[0]}" )" && pwd -P)

rm -f bufr_tables*.db
rm -rf tables

ln -s ${MYDIR}/../tables .

readonly MASTER=1
readonly LOCAL=0

#
# NCEP
#
for v in 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37
do
 echo "NCEP    $v ================="
 ./load_tables ncep  0 $v 7 0 1 $MASTER
done
 ./load_tables ncep  0 0  7 0 1 $LOCAL

#
# ecCodes
#
for v in 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36
do
 echo "ECCODES $v ================="
 ./load_tables eccodes 0 $v 0 0 0 $MASTER
done
 ./load_tables eccodes 0 0  98 0   1 $LOCAL
 ./load_tables eccodes 0 0  78 0   1 $LOCAL
 ./load_tables eccodes 0 0 254 0   1 $LOCAL
 ./load_tables eccodes 0 0  98 0   2 $LOCAL
 ./load_tables eccodes 0 0  78 0   2 $LOCAL
 ./load_tables eccodes 0 0  98 0   3 $LOCAL
 ./load_tables eccodes 0 0  78 0   3 $LOCAL
 ./load_tables eccodes 0 0  98 0 101 $LOCAL

rm -rf tables
