PYTHON = python3

.PHONY: all test clean

all: test

test:
	echo '#!/bin/sh' > test
	echo 'exec  $(PYTHON) OoOScheduler.py "$$@"' >> test
	chmod +x test

clean:
	rm -f test out.txt
