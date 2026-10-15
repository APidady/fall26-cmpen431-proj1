PYTHON = python3

.PHONY: all test clean

all: test

test:
	$(PYTHON) OoOScheduler.py test.in > out.txt

clean:
	rm -f out.txt
