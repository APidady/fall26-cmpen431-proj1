import sys
from Parser import parse_file, InputError

# Simulation configuration and instruction tracking
physical_registers = 0
issue_width = 0
instructions = []
icount = 0
cyclecount = 0
mapping = []
free_registers = []
ready_cycle = []
freed_by_cycle = {}

# Pipeline stage pointers / counters
next_rename = 0
next_commit = 0

def initStructuresAndCounts():
    # Read input and initialize registers and instructions
    global physical_registers, issue_width, instructions, icount
    global mapping, free_registers, ready_cycle, next_rename, next_commit, freed_by_cycle, cyclecount

    physical_registers, issue_width, instructions = parse_file(sys.argv[1])
    icount = len(instructions)
    mapping = list(range(32))
    free_registers = list(range(32, physical_registers))
    ready_cycle = [0] * physical_registers

    for inst in instructions:
        inst.update({
            "FE": None, "DE": None, "RE": None, "DI": None, "IS": None, "WB": None, "CO": None,
            "psrcs": [], "new_phys": None, "old_phys": None
        })

    next_rename = 0
    next_commit = 0
    freed_by_cycle = {}
    cyclecount = 0


def commit():
    # Commit completed instructions in program order
    global next_commit, freed_by_cycle, cyclecount, instructions, issue_width, icount, free_registers
    
    # Free registers scheduled to be released this cycle
    free_registers.extend(freed_by_cycle.pop(cyclecount, []))

    committed = 0
    while next_commit < icount and committed < issue_width:
        inst = instructions[next_commit]
        if inst["WB"] is None or inst["WB"] >= cyclecount:
            break
        inst["CO"] = cyclecount
        if inst["old_phys"] is not None:
            freed_by_cycle.setdefault(cyclecount + 1, []).append(
                inst["old_phys"]
            )
        next_commit += 1
        committed += 1


def writeback():
    # Complete writeback for instructions issued last cycle
    # Writeback is synchronized with issue/execution completion
    pass


def issue():
    # Search the issue queue for ready instructions
    global cyclecount, instructions, issue_width, ready_cycle

    issued = 0
    memory_issued_this_cycle = []

    for inst in instructions:
        if issued >= issue_width:
            break
        if inst["IS"] is not None:
            continue
        if inst["DI"] is None or inst["DI"] >= cyclecount:
            continue

        if not all(ready_cycle[p] <= cyclecount for p in inst["psrcs"]):
            continue

        if inst["op"] in ("L", "S"):
            blocked = False

            # Older stores must issue before younger memory operations.
            for older in instructions[:inst["index"]]:
                if older["op"] == "S":
                    if older["IS"] is None or older["IS"] >= cyclecount:
                        blocked = True
                        break

            if blocked:
                continue

            if inst["op"] == "S" and memory_issued_this_cycle:
                continue

            if inst["op"] == "L" and any(
                issued_mem["op"] == "S"
                for issued_mem in memory_issued_this_cycle
            ):
                continue

        inst["IS"] = cyclecount
        inst["WB"] = cyclecount + 1

        if inst["new_phys"] is not None:
            ready_cycle[inst["new_phys"]] = inst["WB"]

        if inst["op"] in ("L", "S"):
            memory_issued_this_cycle.append(inst)

        issued += 1


def dispatch():
    # Move renamed instructions toward the issue queue
    # Dispatch coordination happens between rename and issue queue
    pass


def rename():
    # Rename architectural registers to physical registers
    global next_rename, cyclecount, instructions, issue_width, icount, mapping, free_registers, ready_cycle

    renamed = 0
    while next_rename < icount and renamed < issue_width:
        inst = instructions[next_rename]
        if inst["DE"] is None or inst["DE"] >= cyclecount:
            break
        if inst["dest"] is not None and not free_registers:
            break

        inst["psrcs"] = [mapping[r] for r in inst["srcs"]]

        if inst["dest"] is not None:
            new_phys = free_registers.pop(0)
            inst["new_phys"] = new_phys
            inst["old_phys"] = mapping[inst["dest"]]
            mapping[inst["dest"]] = new_phys
            ready_cycle[new_phys] = float("inf")

        inst["RE"] = cyclecount
        inst["DI"] = cyclecount + 1
        next_rename += 1
        renamed += 1


def decode():
    # Advance decoded instructions
    global cyclecount, instructions, issue_width, icount
    for i, inst in enumerate(instructions):
        if inst["FE"] is not None and inst["DE"] is None:
            expected_de = inst["FE"] + 1
            if expected_de <= cyclecount:
                inst["DE"] = cyclecount


def fetch():
    # Fetch instructions in program order based on issue width / cycle
    global cyclecount, instructions, icount, issue_width
    for i, inst in enumerate(instructions):
        if inst["FE"] is None:
            expected_fe = i // issue_width
            if expected_fe <= cyclecount:
                inst["FE"] = cyclecount
            break


def emitOutput():
    # Write all seven stage cycles to out.txt
    try:
        with open("out.txt", "w") as output:
            for inst in instructions:
                output.write("{},{},{},{},{},{},{}\n".format(
                    inst["FE"], inst["DE"], inst["RE"], inst["DI"],
                    inst["IS"], inst["WB"], inst["CO"]
                ))
    except (OSError, ValueError):
        return


def main():
    global cyclecount, next_commit, icount

    if len(sys.argv) != 2:
        return
    try:
        initStructuresAndCounts()

        while next_commit < icount:
            # Synchronous pipeline stage evaluations in pipeline order
            commit()
            writeback()
            issue()
            dispatch()
            rename()
            decode()
            fetch()
            
            cyclecount += 1

        emitOutput()
    except (InputError, OSError, ValueError):
        return

if __name__ == "__main__":
    main()
