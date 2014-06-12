BENCH   := parsec-3.0/pkgs/kernels/streamcluster/inst/amd64-linux.gcc/bin/streamcluster
RESULTS := Host Host-oprofile
HYPER   := Host-oprofile

default: all

all: times.pdf hyperlink.pdf
bench: $(BENCH)


times.pdf: times.pl $(patsubst %, %-I,  $(RESULTS)) \
                    $(patsubst %, %-NI, $(RESULTS))
	./times.pl $(RESULTS) > $@

hyperlink.pdf: hyperlink.pl $(patsubst %, %-I,  $(HYPER)) \
                            $(patsubst %, %-NI, $(HYPER))
	./hyperlink.pl $(HYPER) > $@


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
	-rm -rf times.pdf hyperlink.pdf
	-rm -rf parsec-3.0

distclean: clean
	-rm parsec-3.0-core.tar.gz
