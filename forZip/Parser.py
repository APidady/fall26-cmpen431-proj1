import re

class InputError(Exception):
    pass

def is_number(value):
    return re.fullmatch(r"[0-9]+", value) is not None

def parse_file(filename):
    with open(filename, "r") as source:
        lines = source.read().splitlines()

    if not lines:
        raise InputError("Empty input file")

    header = lines[0].split(",")
    if len(header) != 2:
        raise InputError("Invalid header")

    physical_registers = int(header[0])
    issue_width = int(header[1])

    if physical_registers <= 32 or issue_width <= 0:
        raise InputError("Invalid machine settings")

    # Check the number of instructions
    lines = lines[1:]
    if len(lines) < 1 or len(lines) > 256:
        raise InputError("Invalid instruction count")

    instructions = []
 
    for index, line in enumerate(lines):
        fields = line.split(",")
        op = fields[0]

        if len(fields) != 4:
            raise InputError("Invalid instruction format")

        # Convert operands to integers
        if not all(is_number(x) for x in fields[1:]):
            raise InputError("Invalid operand")

        a, b, c = map(int, fields[1:])

        if op == "R":
            if any(r > 31 for r in (a, b, c)):
                raise InputError("Register out of range")

            inst = {
                "index": index,
                "op": "R",
                "dest": a,
                "srcs": [b, c]
            }

        elif op == "I":
            if a > 31 or b > 31 or c > 65535:
                raise InputError("I instruction operand out of range")

            inst = {
                "index": index,
                "op": "I",
                "dest": a,
                "srcs": [b],
                "imm": c
            }

        elif op == "L":
            if a > 31 or b > 65535 or c > 31:
                raise InputError("L instruction operand out of range")

            inst = {
                "index": index,
                "op": "L",
                "dest": a,
                "srcs": [c],
                "imm": b
            }

        elif op == "S":
            if a > 31 or b > 65535 or c > 31:
                raise InputError("S instruction operand out of range")

            inst = {
                "index": index,
                "op": "S",
                "dest": None,
                "srcs": [a, c],
                "imm": b
            }

        else:
            raise InputError("Unknown instruction")

        instructions.append(inst)

    return physical_registers, issue_width, instructions
