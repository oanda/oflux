#!/bin/bash

test_exec=`echo $0 | sed s/run-rw.sh/test_rw_atomic/g`

set -e

num_threads_max=4
num_iterations_max=100
num_events_per_thread_max=10
num_atomics_max=10

for nt in `seq 4 $num_threads_max`; do
 for it in `seq 1 $num_iterations_max`; do
  for ept in `seq 1 $num_events_per_thread_max`; do
   for itms in `seq 1 $num_atomics_max`; do
    $test_exec $nt $it $ept $itms
   done
  done
 done
done
