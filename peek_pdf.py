import os

files = ["Gnose Nag Hammadi.pdf", "codex-nag-hammadi.pdf"]

def peek(path):
    print(f"--- Peeking {path} ---")
    size = os.path.getsize(path)
    with open(path, 'rb') as f:
        head = f.read(1024)
        f.seek(max(0, size - 2048))
        tail = f.read(2048)

    with open("peek.txt", "a") as log:
        log.write(f"--- Peeking {path} ---\n")
        log.write("HEADER:\n")
        log.write(str(head[:100]) + "\n")
        
        eof_count = tail.count(b'%%EOF')
        log.write(f"Tail %%EOF count: {eof_count}\n")
        
        if b'/Linearized' in head:
            log.write("Detected /Linearized dictionary (Fast Web View)\n")
        else:
            log.write("No /Linearized dictionary found\n")
        log.write("\n")

if os.path.exists("peek.txt"):
    os.remove("peek.txt")

for f in files:
    peek(f)
