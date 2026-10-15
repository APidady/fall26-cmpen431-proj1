PYTHON = python3
INPUT_FILE = input.txt

.PHONY: all clean

all: test
        chmod +x test

test:
     echo '#!/bin/sh' > test
     echo '$(PYTHON) OoSched.py "$$@"' >> test


clean:
    rm -f test out.txt
    rm -rf __pycache__

