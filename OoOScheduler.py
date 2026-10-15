import sys
from Parser import parse_file, InputError

def initStructuresAndCounts():
    # Read input and initialize registers and instructions
    pass


def commit():
    # Commit completed instructions in program order
    pass


def writeback():
    # Complete writeback for instructions issued last cycle
    pass


def issue():
    # Search the issue queue for ready instructions
    pass


def dispatch():
    # Move renamed instructions toward the issue queue
    pass


def rename():
    # Rename architectural registers to physical registers
    pass


def decode():
    # Advance decoded instructions
    pass


def fetch():
    # Fetch instructions in program order
    pass


def emitOutput():
    # Write all seven stage cycles to out.txt
    pass


def main():
    initStructuresAndCounts()

    while committedInsts < icount:
        commit()
        writeback()
        issue()
        dispatch()
        rename()
        decode()
        fetch()
        cyclecount += 1

    emitOutput()


if __name__ == "__main__":
    main()
