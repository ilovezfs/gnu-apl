#
# GNUplot commands for plotting file "performamce_data" which is
# written by Performance.apl
#
set xlabel "vector length ⍴,VEC"
set ylabel "microseconds"
set grid xtics ytics

plot "performamce_data" title "VEC + VEC" with lines
pause mouse keypress "\n      *** press ^C to continue → "

