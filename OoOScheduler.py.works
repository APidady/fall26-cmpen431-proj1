import sys
from Parser import parse_file, InputError


def schedule(physical_registers, issue_width, instructions):
    count = len(instructions)
    mapping = list(range(32))
    free_registers = list(range(32, physical_registers))
    ready_cycle = [0] * physical_registers

    for inst in instructions:
        inst.update({
            "FE": inst["index"] // issue_width,
            "DE": inst["index"] // issue_width + 1,
            "RE": None, "DI": None, "IS": None, "WB": None, "CO": None,
            "psrcs": [], "new_phys": None, "old_phys": None
        })

    next_rename = 0
    next_commit = 0
    freed_by_cycle = {}
    cycle = 0

    while next_commit < count:
        free_registers.extend(freed_by_cycle.pop(cycle, []))

        # In-order commit.
        committed = 0
        while next_commit < count and committed < issue_width:
            inst = instructions[next_commit]
            if inst["WB"] is None or inst["WB"] >= cycle:
                break
            inst["CO"] = cycle
            if inst["old_phys"] is not None:
                freed_by_cycle.setdefault(cycle + 1, []).append(
                    inst["old_phys"]
                )
            next_commit += 1
            committed += 1

        # In-order rename.
        renamed = 0
        while next_rename < count and renamed < issue_width:
            inst = instructions[next_rename]
            if inst["DE"] >= cycle:
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

            inst["RE"] = cycle
            inst["DI"] = cycle + 1
            next_rename += 1
            renamed += 1

        # Issue ready instructions out of order.
        issued = 0
        memory_issued_this_cycle = []

        for inst in instructions:
            if issued >= issue_width:
                break
            if inst["IS"] is not None:
                continue
            if inst["DI"] is None or inst["DI"] >= cycle:
                continue

            if not all(ready_cycle[p] <= cycle for p in inst["psrcs"]):
                continue

            if inst["op"] in ("L", "S"):
                blocked = False

                # Older stores must issue before younger memory operations.
                for older in instructions[:inst["index"]]:
                    if older["op"] == "S":
                        if older["IS"] is None or older["IS"] >= cycle:
                            blocked = True
                            break

                if blocked:
                    continue

                # Loads may share a cycle with other loads.
                # A store may not share a cycle with any memory operation.
                if inst["op"] == "S" and memory_issued_this_cycle:
                    continue

                if inst["op"] == "L" and any(
                    issued_mem["op"] == "S"
                    for issued_mem in memory_issued_this_cycle
                ):
                    continue

            inst["IS"] = cycle
            inst["WB"] = cycle + 1

            if inst["new_phys"] is not None:
                ready_cycle[inst["new_phys"]] = inst["WB"]

            if inst["op"] in ("L", "S"):
                memory_issued_this_cycle.append(inst)

            issued += 1

        cycle += 1

    return instructions


def main():
    with open("out.txt", "w") as output:
        if len(sys.argv) != 2:
            return
        try:
            physical_registers, issue_width, instructions = parse_file(
                sys.argv[1]
            )
            instructions = schedule(
                physical_registers, issue_width, instructions
            )
            for inst in instructions:
                output.write("{},{},{},{},{},{},{}\n".format(
                    inst["FE"], inst["DE"], inst["RE"], inst["DI"],
                    inst["IS"], inst["WB"], inst["CO"]
                ))
        except (InputError, OSError, ValueError):
            return


if __name__ == "__main__":
    main()
