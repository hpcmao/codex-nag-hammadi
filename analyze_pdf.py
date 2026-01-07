import sys
import os
import re

files = ["Gnose Nag Hammadi.pdf", "codex-nag-hammadi.pdf"]

def analyze_raw(path):
    if not os.path.exists(path):
        print(f"File not found: {path}")
        return

    size = os.path.getsize(path)
    with open(path, 'rb') as f:
        data = f.read()
    
    # Simple heuristics
    obj_count = len(re.findall(b'\\d+ \\d+ obj', data))
    stream_count = len(re.findall(b'stream', data))
    font_count = len(re.findall(b'/Font', data))
    image_count = len(re.findall(b'/Image', data))
    xobject_count = len(re.findall(b'/XObject', data))
    producer_match = re.search(b'/Producer \\((.*?)\\)', data)
    creator_match = re.search(b'/Creator \\((.*?)\\)', data)
    
    with open("analysis.txt", "a") as out:
        out.write(f"File: {path}\n")
        out.write(f"  Size: {size:,} bytes\n")
        out.write(f"  Objects: {obj_count}\n")
        out.write(f"  Streams: {stream_count}\n")
        out.write(f"  Fonts occurrences: {font_count}\n")
        out.write(f"  Images occurrences: {image_count}\n")
        out.write(f"  XObject occurrences: {xobject_count}\n")
        if producer_match:
            out.write(f"  Producer: {producer_match.group(1)}\n")
        if creator_match:
            out.write(f"  Creator: {creator_match.group(1)}\n")
        out.write("-" * 40 + "\n")

if os.path.exists("analysis.txt"):
    os.remove("analysis.txt")

print("Starting Analysis...")
for f in files:
    analyze_raw(f)
print("Analysis done.")
