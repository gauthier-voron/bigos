#!/bin/bash

if [ $1 ] ; then
  mkdir -p $1-NI || exit 1
  mkdir -p $1-I || exit 1
fi

cd parsec-3.0 || exit 1
source ./env.sh

rm -rf log/* || exit 1


for i in `seq 1 3` ; do
  parsecmgmt -a run -p streamcluster -i native -n 48 || exit 1
done
[ $1 ] && mv log/*/*.log "../$1-NI/"

for i in `seq 1 3` ; do
  numactl --interleave=all parsecmgmt -a run -p streamcluster -i native -n 48 || exit 1
done
[ $1 ] && mv log/*/*.log "../$1-I/"

