#
# use the HUP output from exercise program to estimate some metrics
#
function report_on(i,s) {
 stat_val = stat[s,i] - ( i > 1 ? stat[s,i-1] : 0);
 printf " %-10s per sec  total is %-10.2f for incr total %-10d cumu total %-12d\n", s, stat_val / (end_time[i]-( i > 1 ? end_time[i-1] : start_time)), stat_val, stat[s,i];
}
function report(i) {
 print "over " ( i > 1 ? "the next " : "")  "" (end_time[i]-( i > 1 ? end_time[i-1] : start_time)) " seconds";
 report_on(i,"e.run");
 report_on(i,"threads");
 report_on(i,"slps");
 report_on(i,"e.stl");
 report_on(i,"e.stl.at");
}
BEGIN {
 ind = 0;
}
/oflux::lockfree::RunTime/ {
 start_time = $1;
}
/RT running/ {
 ind = ind+1
 end_time[ind] = $1;
}
/.*info  thread.*/ {
 stat["threads",ind] = stat["threads",ind] + 1;
 for (i = 7; i <= NF; i = i+1) {
  if ( split($(i), arr,/:/) >= 2) {
    stat[arr[1],ind] = stat[arr[1],ind] + arr[2];
  }
 }
}
/RTend/ {
 if(ind > 0) { report(ind); }
}
