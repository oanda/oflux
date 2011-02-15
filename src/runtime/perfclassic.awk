#
# perf from a classic runtime snapshot
#
BEGIN {
 report = 0;
}
/info  load_flow/ {
 startt = $1;
}
/Node report/ {
 report = 1;
 sum_exec = 0;
 sum_inst = 0;
}
/Runtime snapshot/ {
 report = 0;
 endt = $1;
 printf "after %d sec, %f inst/sec, %f exec/sec\n", (endt-startt), sum_inst/(endt-startt), sum_exec/(endt-startt);
}
/info/ {
 if(report) {
  sum_inst += $4;
  sum_exec += $5;
 }
}
