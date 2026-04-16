import sys

OP_PINMODE = 0x10
OP_WRITE   = 0x11
OP_DELAY   = 0x12

PIN_MAP = {f"D{i}": i for i in range(14)}
MODE_MAP = {"OUTPUT": 1, "INPUT": 0}
VALUE_MAP = {"HIGH": 1, "LOW": 0}

def parse_pin(token):
    token = token.upper()
    return PIN_MAP[token]

def compile_source(text):
    out = bytearray()

    for raw_line in text.splitlines():
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue

        upper = line.upper()

        if upper in {"SETUP", "LOOP", "END"}:
            continue

        parts = upper.split()
        cmd = parts[0]

        if cmd == "PINMODE":
            pin = parse_pin(parts[1])
            mode = MODE_MAP[parts[2]]
            out += bytes([OP_PINMODE, pin, mode])
            print(out)

        elif cmd == "WRITE":
            pin = parse_pin(parts[1])
            value = VALUE_MAP[parts[2]]
            out += bytes([OP_WRITE, pin, value])

        elif cmd == "DELAY":
            ms = int(parts[1])
            out += bytes([OP_DELAY, ms & 0xFF, (ms >> 8) & 0xFF])

        else:
            raise ValueError(f"Unknown command: {line}")

    return out

def main():
    if len(sys.argv) != 3:
        print("Usage: py compiler.py <input.my> <output.bin>")
        return

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    with open(input_file, "r", encoding="utf-8") as f:
        source = f.read()

    bytecode = compile_source(source)
    print("Bytecode:", bytecode)

    with open(output_file, "wb") as f:
        f.write(bytecode)

    print("Compiled successfully.")
    print("Size:", len(bytecode), "bytes")
    print("Hex :", " ".join(f"{b:02X}" for b in bytecode))

if __name__ == "__main__":
    main()