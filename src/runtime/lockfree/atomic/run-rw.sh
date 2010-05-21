#!/bin/bash

test_exec=`echo $0 | sed s/run-rw.sh/test_rw_atomic2/g`

set -e

num_threads_max=4
num_iterations_max=120
num_events_per_thread_max=9
num_atomics_max=9

for nt in `seq 2 $num_threads_max`; do
 for it in `seq 1 $num_iterations_max`; do
  for ept in `seq 1 $num_events_per_thread_max`; do
   for itms in `seq 1 $num_atomics_max`; do
    $test_exec $nt $it $ept $itms
   done
  done
 done
done
