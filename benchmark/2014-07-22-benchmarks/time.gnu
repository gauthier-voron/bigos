set terminal pdf
set output "time.pdf"

set style fill solid 1.00 border -1
set style data histogram
set style histogram clustered

set title "Application Time"
set ylabel "Computation Time (sec)"

set xrange [.7:5]

set boxwidth 0.18 absolute
set style fill solid .2

plot './time.dat' u ($1):12           w boxes lt rgb "red" t col, \
          '' u ($1):12:13:14     w yerrorbars ps 0 lt rgb "red" notitle, \
          '' u ($1+.2):9         w boxes lt rgb "green" t col, \
          '' u ($1+.2):9:10:11   w yerrorbars ps 0 lt rgb "green" notitle, \
          '' u ($1+.4):3:xtic(2) w boxes lt rgb "blue" t col, \
          '' u ($1+.4):3:4:5     w yerrorbars ps 0 lt rgb "blue" notitle, \
          '' u ($1+.6):6         w boxes lt rgb "purple" t col, \
          '' u ($1+.6):6:7:8     w yerrorbars ps 0 lt rgb "purple" notitle
