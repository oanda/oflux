#!/bin/bash

filename=$1


cat $filename | \
sed -e 's/^/ /g' | \
awk 'BEGIN { lineno=0; comment = 0; } /\/\*/ { comment = comment+1 } /.*/ { lineno=lineno+1; if((lineno%2) == 0) { bgcol=" bgcolor=\"#ffffff\""; } else { bgcol=""; } if(comment>0) { fgcol = "color=\"#0000ff\""; } else { fgcol = "color=black"; } print "<tr" bgcol "><td bgcolor=black><font color=yellow>" lineno "</font></td><td><font " fgcol ">" $0 "</font></td></tr>"; } /\*\// { comment = comment -1; }' | \
sed -e 's/ \(as\) \| \(where\) \| \(terminate\) \| \(handle\) \| \(begin\) \| \(end\) \| \(if\) \| \(terminate\) \| begin$\| end$/ <font color="#00a000">&<\/font> /g' | \
sed -e 's/ \(guard\) \| \(readwrite\) \| \(sequence\) \| \(pool\) \| \(condition\) \| \(node\) \| \(source\) \| \(error\) \| \(atomic\) \| \(instance\) \| \(module\) \| \(exclusive\) \| \(initial\) \| \(plugin\) \| \(free\) / <font color="#ff2020">&<\/font> /g' | \
sed -e 's/ \(detached\) \| \(abstract\) \| \(read\) \| \(write\) \| \(external\) \| \(unordered\) / <font color="#2020ff">&<\/font> /g' | \
sed -e 's/ \(include\) [ ]*\([a-zA-Z_0-9]*\)\.flux/ <font color="#1010ee">include<\/font> <a href="\2.html">\2.flux<\/a>/g' | \
sed -e 's/ \(=>\)\| \(->\)\| \(=\)\| \(>\)\| \(\&=\)\|\]\|\[\|(\|)\|;/ <font color="#80008f">&<\/font>/g' | \
sed -e 's/ : / <font color="#80008f">&<\/font> /g' | \
sed -e 's/ \(\/\*\)/ <font color="#0000ff">&/g' | \
sed -e 's/ \(\*\/\)/ &<\/font>/g' | \
sed -e 's/\([-=]\)>/\1\&gt;/g' | \
awk 'BEGIN { print "<html><title> '$filename' </title><head> <style type=\\\"text/css\\\"> body{ background: #FFF; color: #222; font-family: Verdana, Tahoma, Arial, Trebuchet MS, Sans-Serif; font-size: 11px; line-height: 135%; margin: 0px; padding: 0px; /* required for Opera to have 0 margin */ text-align: center; /* centers board in MSIE */ } </style> </head><body bgcolor=\"#e0e0e0\"><h1>'$filename'</h1><table>"; } /.*/ { print } END { print "</table></body></html>" }'
