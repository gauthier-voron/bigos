set terminal pdf
set output 'access.pdf'

set boxwidth 0.5
set style fill solid 1.0 border -1
set style data histogram
set style histogram rows

set xtics font ",8" rotate by -45
set ytics font ",8"
set yrange [0:120]

set title "Access Type"
set xlabel offset 0,-1
set ylabel "Access proportion (%)"

plot \
  newhistogram 'dc', \
     'access.dat' u 2:xtic(1) t col lt rgb "red", \
               '' u 3         t col lt rgb "green", \
  newhistogram 'fluidanimate', \
     'access.dat' u 4:xtic(1) notitle lt rgb "red", \
               '' u 5         notitle lt rgb "green", \
  newhistogram 'streamcluster', \
     'access.dat' u 6:xtic(1) notitle lt rgb "red", \
               '' u 7         notitle lt rgb "green", \
  newhistogram 'swaptions', \
     'access.dat' u 8:xtic(1) notitle lt rgb "red", \
               '' u 9         notitle lt rgb "green", \
