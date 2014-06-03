BENCH       := parsec-3.0/pkgs/kernels/streamcluster/inst/amd64-linux.gcc/bin/streamcluster
RESULTS-INI := Host Dom0
RESULTS-DPI := DomU 2xDomU48 2xDomU24

default: all

all: times.pdf
bench: $(BENCH)


times.pdf: times.pl $(patsubst %, %-I,  $(RESULTS-INI)) \
                    $(patsubst %, %-NI, $(RESULTS-INI)) \
                    $(patsubst %, %-default,  $(RESULTS-DPI)) \
                    $(patsubst %, %-pinned, $(RESULTS-DPI))
	./times.pl $(RESULTS-INI) $(RESULTS-DPI) > $@


parsec-3.0/pkgs/kernels/streamcluster/inst/amd64-linux.gcc/bin/streamcluster: \
  parsec-3.0
	/bin/bash -c "cd $< ; source ./env.sh ; parsecmgmt -a build -p streamcluster"

parsec-3.0: parsec-3.0-core.tar.gz
	tar -xzf $^
	touch $@

parsec-3.0-core.tar.gz:
	curl http://parsec.cs.princeton.edu/download/3.0/$@ > $@


clean:
	-find . -name "*~" -exec rm {} \;
	-rm -rf times.pdf
	-rm -rf parsec-3.0

distclean: clean
	-rm parsec-3.0-core.tar.gz
