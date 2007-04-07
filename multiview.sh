#!/bin/sh
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms
# of the Common Development and Distribution License
# (the "License").  You may not use this file except
# in compliance with the License.
#
# You can obtain a copy of the license at
# src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing
# permissions and limitations under the License.
#
# When distributing Covered Code, include this CDDL
# HEADER in each file and include the License file at
# usr/src/OPENSOLARIS.LICENSE.  If applicable,
# add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your
# own identifying information: Portions Copyright [yyyy]
# [name of copyright owner]
#
# CDDL HEADER END
#

#
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# ident	"@(#)multiview	1.6	05/08/04 SMI"
#

#
# output html comparison of several libmicro output data files
# usage: multiview file1 file2 file3 file4 ...
#
#	relative ranking is calculated using first as reference
#	color interpolation is done to indicate relative performance;
#	the redder the color, the slower the result, the greener the
#       faster
 
/bin/nawk '	BEGIN { 
  benchmark_count = 0;
  header_count = 0;
}
/^#/ {
	continue;
	}
/errors/ {
	continue;
	}
/^\!/ { 
	split($0, A_header, ":"); 
	name = substr(A_header[1],2);
	headers[name]=name; 
	header_data[name,FILENAME] = substr($0, length(name) + 3);
	if (header_names[name] == 0) {
		header_names[name] = ++header_count;
		headers[header_count] = name;
	}
	continue;
}

	{ 
	if(NF >= 7) {
	      	if (benchmark_names[$1] == 0) {
			benchmark_names[$1] = ++benchmark_count;
			benchmarks[benchmark_count] = $1;
		}
		if ($6 == 0)
		   	benchmark_data[$1,FILENAME] = $4;
		else 
			benchmark_data[$1,FILENAME] = -1;
	}
}	    

END { 
	
	printf("<html>\n");
	printf("<head>\n");
	printf("<meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n");
	printf("<meta name=\"author\" content=\"autogen\">\n");
	printf("<title>multiview comparison</title>\n")
	printf("</head>\n");
	printf("<body bgcolor=\"#ffffff\" link=\"#0000ee\" vlink=\"#cc0000\" alink=\"#0000ee\">\n");
	printf("<table border=\"1\" cellspacing=\"1\">\n");
	printf("<tbody>\n");
	for(i = 1; i <= header_count; i++) {
		hname = headers[i];
		printf("<tr><td>%s</td>\n", hname);
		
		for (j = 1; j <= ARGC; j++) {
			printf("<td>%s</td>\n", header_data[hname, ARGV[j]]);
		}
	}
	printf("</tr>\n");
	printf("<tr>\n");
	printf("<td><b>BENCHMARK</b><br></td>\n");
	printf("<td align=\"right\"><b>USECS</b><br></td>\n");

	for (i = 2; i < ARGC; i++) 
		printf("<td align=\"right\"><b>USECS [percentage]</b><br></td>\n");

	for(i = 1; i < benchmark_count; i++) {
	  for(j = 1; j < benchmark_count; j++) {
	    if (benchmarks[j] > benchmarks[j + 1]) {
	      tmp = benchmarks[j]; 
	      benchmarks[j] =  benchmarks[j+1];
	      benchmarks[j+1] = tmp;
	    }
	  }
	}

	for(i = 1; i <= benchmark_count; i++) {
		name = benchmarks[i];
		a = benchmark_data[name, ARGV[1]];

		printf("<tr>\n");
		printf("<td align=\"right\">%25s</td>\n", name);
		if (a > 0)
			printf("<td align=\"right\">%25.5f</td>\n", a);
		else {
			if (a < 0)
				printf("<td align=\"right\" bgcolor=\"#ff0000\">%25s</td>\n", "ERRORS");
			else 
				printf("<td align=\"right\">%25s</td>\n", "missing");
				
			for (j = 2; j < ARGC; j++) 
				printf("<td align=\"right\">%25s</td>\n", "not computed");
			continue;
		}

		for (j = 2; j < ARGC; j++) {
			b = benchmark_data[name, ARGV[j]];
			if (b > 0) {
				factor = b/a;
				bgcolor = colormap(factor);
				if (factor > 1)
				  percentage = -(factor * 100 - 100);
				if (factor <= 1)
				  percentage = 100/factor - 100;
				
				printf("<td align=\"right\" bgcolor=\"%s\"><pre>%11.5f[%#+7.1f%%]</td>\n", 
					bgcolor, b, percentage);
			}
			
			else if (b < 0) 
				printf("<td align=\"right\" bgcolor=\"#ff0000\">%25s</td>\n", "ERRORS");
			else
				printf("<td align=\"right\">%25s</td>\n", "missing");
			
		}
		printf("</tr>\n");

	}
	printf("</tbody></table><br></body></html>\n");

} 

function colormap(value, bgcolor, r, g, b) 
{	
	if (value <= .2)
		value = .2;
	if (value > 5)
		value = 5;

	if (value < .9) {
		r = colorcalc(.2, value, .9, 0, 255);
		g = colorcalc(.2, value, .9, 153, 255);
		b = colorcalc(.2, value, .9, 0, 255);
		bgcolor=sprintf("#%2x%2x%2x",  r, g, b);
	}
	else if (value < 1.1)
		bgcolor="#ffffff";
	else {
		r = 255;
		g = colorcalc(1.1, value, 5, 255, 0);
		b = colorcalc(1.1, value, 5, 255, 0);
		bgcolor=sprintf("#%2x%2x%2x",  r, g, b);
	}
	return (bgcolor);
}

function colorcalc(min, value, max, mincolor, maxcolor)
{
        return((value - min)/(max-min) * (maxcolor-mincolor) + mincolor);
}

' "$@"


