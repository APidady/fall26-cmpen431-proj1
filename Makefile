.PHONY: clean NUKE

all: DSE

DSE: 431project.cpp 431project.h 431projectUtils.cpp YOURCODEHERE.cpp
	g++ -O3 431project.cpp 431projectUtils.cpp YOURCODEHERE.cpp -lm -o DSE ; chmod +x worker.sh

clean:
	-rm DSE

NUKE:
	-rm DSE; rm -rf rawProjectOutputData summaryfiles logs

RUN_ED2P:
	./DSE e

RUN_EDP:
	./DSE p

RUN_EDAP:
	./DSE d

RUN_ED2AP:
	./DSE D
