import pdfplumber
import sys

print("Attempting to open codex-nag-hammadi.pdf...")
try:
    with pdfplumber.open("codex-nag-hammadi.pdf") as pdf:
        print("Opened successfully.")
        print(f"Pages: {len(pdf.pages)}")
except Exception as e:
    print(f"Error: {e}")
print("Done.")
