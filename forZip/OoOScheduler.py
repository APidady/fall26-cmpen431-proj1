import sys
from Parser import parse_file, InputError

# Simulation configuration and instruction tracking
phys_reg = 0
issue_width = 0
instructions = []
icount = 0
cyclecount = 0
mapping = []
free_reg = []
ready_cycle = []
freed_by_cycle = {}

# Pipeline stage pointers / counters
next_rename = 0
next_commit = 0

def initStructuresAndCounts():
    # Read input and initialize registers and instructions
    global phys_reg, issue_width, instructions, icount
    global mapping, free_reg, ready_cycle
    global next_rename, next_commit, freed_by_cycle, cyclecount

    phys_reg, issue_width, instructions = parse_file(sys.argv[1])
    icount = len(instructions)

    mapping = list(range(32))
    free_reg = list(range(32, phys_reg))
    ready_cycle = [0] * phys_reg

    for inst in instructions:
        inst.update({
            "FE": None, 
            "DE": None, 
            "RE": None, 
            "DI": None, 
            "IS": None, 
            "WB": None, 
            "CO": None,
            "psrcs": [], 
            "new_phys": None, 
            "old_phys": None
        })

    next_rename = 0
    next_commit = 0
    freed_by_cycle = {}
    cyclecount = 0


def commit():
    global next_commit, freed_by_cycle, cyclecount 
    global instructions, issue_width, icount, free_reg
    
    # Free registers to be released in this cycle
    free_reg.extend(freed_by_cycle.pop(cyclecount, []))

    committed = 0

    while next_commit < icount and committed < issue_width:
        inst = instructions[next_commit]

        if inst["WB"] is None or inst["WB"] >= cyclecount:
            break

        inst["CO"] = cyclecount

        if inst["old_phys"] is not None:
            freed_by_cycle.setdefault(cyclecount + 1, []).append(
                inst["old_phys"])
        
        next_commit += 1
        committed += 1


def writeback():
    # Complete writeback for instructions issued last cycle
    global cyclecount, instructions, ready_cycle

    for inst in instructions:
        if inst["IS"] is None or inst["WB"] is not None:
            continue

        if inst["IS"] + 1 <= cyclecount:
            inst["WB"] = inst["IS"] + 1

            if inst["new_phys"] is not None:
                ready_cycle[inst["new_phys"]] = inst["WB"]


def issue():
    # Search the issue queue for ready instructions
    global cyclecount, instructions, issue_width, ready_cycle

    issued = 0
    mem_issued_this_cycle = []

    for inst in instructions:
        if issued >= issue_width:
            break

        if inst["IS"] is not None:
            continue

        if inst["DI"] is None or inst["DI"] >= cyclecount:
            continue

        if not all(
            ready_cycle[p] <= cyclecount 
            for p in inst["psrcs"]
        ):
            continue

        if inst["op"] in ("L", "S"):
            blocked = False

            # issue old instr stores before new memory operation
            for old in instructions[:inst["index"]]:
                if old["op"] == "S":
                    if (
                        old["IS"] is None
                        or old["IS"] >= cyclecount
                    ):
                        blocked = True
                        break

            if blocked:
                continue

            # cannot issue stores with another memory operation.
            if (
                inst["op"] == "S"
                and mem_issued_this_cycle
            ):
                continue

            # cannot issue loads in a cycle with a store.
            if (
                inst["op"] == "L"
                and any(
                    issued_mem["op"] == "S"
                    for issued_mem in mem_issued_this_cycle
                )
            ):
                continue


        inst["IS"] = cyclecount

        if inst["op"] in ("L", "S"):
            mem_issued_this_cycle.append(inst)

        issued += 1


def dispatch():
    # Move renamed instructions to the issue queue
    # Dispatch happens between rename and issue queue
    pass


def rename():
    # Rename arch registers to phys registers
    global next_rename, cyclecount, instructions, issue_width
    global icount, mapping, free_reg, ready_cycle

    renamed = 0

    while next_rename < icount and renamed < issue_width:
        inst = instructions[next_rename]

        # Decode must be completed in an earlier cycle
        if inst["DE"] is None or inst["DE"] >= cyclecount:
            break

        # Stall if no phys register is available
        if inst["dest"] is not None and not free_reg:
            break

        inst["psrcs"] = [
             mapping[r] for r in inst["srcs"]
        ]

        if inst["dest"] is not None:
            new_phys = free_reg.pop(0)

            inst["new_phys"] = new_phys
            inst["old_phys"] = mapping[inst["dest"]]

            mapping[inst["dest"]] = new_phys
            ready_cycle[new_phys] = float("inf")

        inst["RE"] = cyclecount
        inst["DI"] = cyclecount + 1

        next_rename += 1
        renamed += 1


def decode():
    global cyclecount,instructions 
    
    # check for decode stalls
    for inst in instructions:
        if inst["FE"] is not None and inst["DE"] is None:
            if inst["FE"] + 1 <= cyclecount:
                inst["DE"] = inst["FE"] + 1


def fetch():
    global cyclecount, instructions, icount, issue_width

    fetched = 0

    for i, inst in enumerate(instructions):
        if inst["FE"] is not None:
            continue

        expected_fe = i // issue_width

        if expected_fe > cyclecount:
            break

        inst["FE"] = cyclecount
        fetched += 1

        if fetched >= issue_width:
            break


def emitOutput():
    # Write the seven stage cycles to out.txt
    with open("out.txt", "w") as output:
        for inst in instructions:
            output.write("{},{},{},{},{},{},{}\n".format(
                inst["FE"], inst["DE"], inst["RE"], inst["DI"],
                inst["IS"], inst["WB"], inst["CO"]
            ))


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
