#!/bin/bash

DATA=native


if [ $1 ] ; then
  mkdir -p $1-NI || exit 1
  mkdir -p $1-I || exit 1
fi

cd parsec-3.0 || exit 1
source ./env.sh

rm -rf log/* || exit 1


for i in `seq 1 3` ; do
  sudo opcontrol --reset
  sudo opcontrol --start
  OPLOG="../$1-NI/opreport_48_${DATA}_`date +'%Y-%m-%d_%H:%M:%S'`.log"
  parsecmgmt -a run -p streamcluster -i $DATA -n 48 || exit 1
  sudo opcontrol --dump
  sudo opcontrol --stop
  opreport 2>/dev/null | grep streamcluster
  [ $1 ] && opreport >"$OPLOG" 2>/dev/null
done
[ $1 ] && mv log/*/*.log "../$1-NI/"

for i in `seq 1 3` ; do
  sudo opcontrol --reset
  sudo opcontrol --start
  OPLOG="../$1-I/opreport_48_${DATA}_`date +'%Y-%m-%d_%H:%M:%S'`.log"
  numactl --interleave=all parsecmgmt -a run -p streamcluster -i $DATA -n 48 || exit 1
  sudo opcontrol --dump
  sudo opcontrol --stop
  opreport 2>/dev/null | grep streamcluster
  [ $1 ] && opreport >"$OPLOG" 2>/dev/null
done
[ $1 ] && mv log/*/*.log "../$1-I/"

