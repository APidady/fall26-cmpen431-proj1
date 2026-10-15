import re

class InputError(Exception):
    """
    Raised when the input file does not match the required format.
    """
    pass

def parse_file(filename):
    """
    Return:
        physical_registers: number of physical registers
        issue_width: machine issue width
        instructions: list of instruction dictionaries

    Raises InputError if the input is invalid.
    """
    with open(filename, "r") as source:
        lines = source.read().splitlines()

    if not lines:
        raise InputError("Empty input file")

    header = lines[0].split(",")
    if len(header) != 2:
        raise InputError("Invalid header")

    if not all(re.fullmatch(r"[0-9]+", x) for x in header):
        raise InputError("Header values must be decimal integers")

    physical_registers = int(header[0])
    issue_width = int(header[1])

    if physical_registers <= 0 or issue_width <= 0:
        raise InputError("Header values must be positive")

    if physical_registers <= 32:
        raise InputError("At least 33 physical registers are required")

    instruction_lines = lines[1:]

    if not 1 <= len(instruction_lines) <= 256:
        raise InputError("Invalid instruction count")

    instructions = []

    for index, line in enumerate(instruction_lines):
        fields = line.split(",")

        if fields[0] == "R" and len(fields) == 4:
            opcode = "R"
            dest, src1, src2 = fields[1:]
            if not all(re.fullmatch(r"[0-9]+", x)
                       for x in (dest, src1, src2)):
                raise InputError("Invalid register")
            dest, src1, src2 = int(dest), int(src1), int(src2)
            if any(r > 31 for r in (dest, src1, src2)):
                raise InputError("Register out of range")
            instruction = {
                "index": index, "op": opcode, "dest": dest,
                "srcs": [src1, src2]
            }

        elif fields[0] == "I" and len(fields) == 4:
            opcode = "I"
            dest, src, imm = fields[1:]
            if not all(re.fullmatch(r"[0-9]+", x)
                       for x in (dest, src, imm)):
                raise InputError("Invalid I instruction")
            dest, src, imm = int(dest), int(src), int(imm)
            if dest > 31 or src > 31 or imm > 65535:
                raise InputError("I instruction operand out of range")
            instruction = {
                "index": index, "op": opcode, "dest": dest,
                "srcs": [src], "imm": imm
            }

        elif fields[0] == "L" and len(fields) == 4:
            opcode = "L"
            dest, imm, base = fields[1:]
            if not all(re.fullmatch(r"[0-9]+", x)
                       for x in (dest, imm, base)):
                raise InputError("Invalid L instruction")
            dest, imm, base = int(dest), int(imm), int(base)
            if dest > 31 or base > 31 or imm > 65535:
                raise InputError("L instruction operand out of range")
            instruction = {
                "index": index, "op": opcode, "dest": dest,
                "srcs": [base], "imm": imm
            }

        elif fields[0] == "S" and len(fields) == 4:
            opcode = "S"
            data, imm, base = fields[1:]
            if not all(re.fullmatch(r"[0-9]+", x)
                       for x in (data, imm, base)):
                raise InputError("Invalid S instruction")
            data, imm, base = int(data), int(imm), int(base)
            if data > 31 or base > 31 or imm > 65535:
                raise InputError("S instruction operand out of range")
            instruction = {
                "index": index, "op": opcode, "dest": None,
                "srcs": [data, base], "imm": imm
            }

        else:
            raise InputError("Invalid instruction format")

        instructions.append(instruction)

    return physical_registers, issue_width, instructions
