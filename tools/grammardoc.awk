BEGIN { ok = 1; 
	print "<html><title>oflux<b> </b>grammar</title> <body>";
	print "<p>This<b> </b>is<b> </b>the<b> </b><a href=\"http://en.wikipedia.org/wiki/Backus-Naur_form\">BNF</a><b> </b>for<b> </b>OFlux:</p>";
	print "<table>";
	ignore = 0;
}
END { print "</table></body></html>"; }
/%entry% :.*program/ { ok=0; }
/^$/ { if(ok) print "<tr></tr>"; }
/0  \$accept : \%entry\%/ { ignore = 1;}
/[ ]*[^ ]* [^ ]* :/ { 
	if(ok && !ignore) { 
		printf "<tr><td>" $1 "</td><td><a name=\"" $2 "\">" $2 "</a></td><td>" $3 "</td><td> "; 
		for(i = 4; i <= NF; i++) {
			printf $i " ";
		}
		printf "</td></tr>\n";
	}
	ignore = 0;
}
/.*\|.*/ { 
	if(ok && !ignore) { 
		printf "<tr><td>" $1 "</td><td></td><td>" $2 "</td><td> "; 
		for(i = 3; i <= NF; i++) {
			printf $i " ";
		}
		printf "</td></tr>\n";
	}
	ignore=0;
}
