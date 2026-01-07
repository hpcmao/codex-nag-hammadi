from pypdf import PdfReader
import sys

print("Attempting to open codex-nag-hammadi.pdf with pypdf...")
try:
    reader = PdfReader("codex-nag-hammadi.pdf")
    print(f"Opened successfully. Encrypted: {reader.is_encrypted}")
    print(f"Pages: {len(reader.pages)}")
    print("Extracting text from page 1...")
    print(reader.pages[0].extract_text()[:200])
except Exception as e:
    print(f"Error: {e}")
print("Done.")
