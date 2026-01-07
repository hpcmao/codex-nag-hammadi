import pdfplumber
import os
import sys
from pypdf import PdfReader

def list_to_markdown_table(data):
    """
    Converts a list of lists (table data) to a Markdown table string.
    """
    if not data:
        return ""
    
    cleaned_data = []
    for row in data:
        cleaned_row = []
        for cell in row:
            if cell is None:
                cleaned_row.append("")
            else:
                cleaned_row.append(str(cell).replace("\n", " ").strip())
        cleaned_data.append(cleaned_row)
    
    if not cleaned_data:
        return ""
    num_columns = max(len(row) for row in cleaned_data)
    
    for row in cleaned_data:
        while len(row) < num_columns:
            row.append("")
            
    header = cleaned_data[0]
    body = cleaned_data[1:]
    
    md_lines = []
    md_lines.append("| " + " | ".join(header) + " |")
    md_lines.append("| " + " | ".join(["---"] * num_columns) + " |")
    for row in body:
        md_lines.append("| " + " | ".join(row) + " |")
        
    return "\n".join(md_lines)

def convert_with_pypdf(pdf_path, output_path):
    print(f"Fallback: Converting {pdf_path} using pypdf...", flush=True)
    md_content = []
    md_content.append(f"# {os.path.basename(pdf_path)}\n")
    
    try:
        reader = PdfReader(pdf_path)
        total_pages = len(reader.pages)
        print(f"Total pages: {total_pages}", flush=True)
        
        for i, page in enumerate(reader.pages):
            if (i+1) % 10 == 0:
                print(f"Processing page {i+1}/{total_pages}...", flush=True)
            
            md_content.append(f"## Page {i+1}\n")
            text = page.extract_text()
            if text:
                md_content.append(text)
                md_content.append("\n")
            md_content.append("\n---\n")
            
    except Exception as e:
        print(f"Error with pypdf: {e}", flush=True)
        return

    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(md_content))
    print(f"Done: {output_path}", flush=True)

def convert_pdf_to_md(pdf_path):
    output_path = os.path.splitext(pdf_path)[0] + ".md"
    if os.path.exists(output_path):
        print(f"Skipping {pdf_path}, output {output_path} already exists.")
        return

    # Special case for known problematic file
    if "codex-nag-hammadi.pdf" in pdf_path:
        convert_with_pypdf(pdf_path, output_path)
        return

    print(f"Converting {pdf_path} using pdfplumber...", flush=True)
    
    md_content = []
    md_content.append(f"# {os.path.basename(pdf_path)}\n")
    
    try:
        # Use pdfplumber
        with pdfplumber.open(pdf_path) as pdf:
            total_pages = len(pdf.pages)
            print(f"Total pages: {total_pages}", flush=True)
            for i, page in enumerate(pdf.pages):
                if (i+1) % 5 == 0 or i == 0 or i == total_pages - 1:
                    print(f"Processing page {i+1}/{total_pages}...", flush=True)
                
                md_content.append(f"## Page {i+1}\n")
                
                # Tables
                try:
                    tables = page.extract_tables(table_settings={"vertical_strategy": "lines", "horizontal_strategy": "lines"})
                    if not tables:
                         tables = page.extract_tables()
                except Exception as e:
                    print(f"Error extracting tables on page {i+1}: {e}", flush=True)
                    tables = []
                
                # Text
                try:
                    text = page.extract_text()
                except Exception as e:
                    print(f"Error extracting text on page {i+1}: {e}", flush=True)
                    text = ""
                
                if text:
                    md_content.append(text)
                    md_content.append("\n")
                
                if tables:
                    md_content.append(f"\n### Tables Detected on Page {i+1}\n")
                    for table in tables:
                        md_table = list_to_markdown_table(table)
                        md_content.append(md_table)
                        md_content.append("\n")
                
                md_content.append("\n---\n")
                
        with open(output_path, "w", encoding="utf-8") as f:
            f.write("\n".join(md_content))
        print(f"Done: {output_path}", flush=True)

    except Exception as e:
        print(f"Failed to convert with pdfplumber: {e}", flush=True)
        # Fallback to pypdf if pdfplumber fails (though 'hanging' is harder to catch here)
        convert_with_pypdf(pdf_path, output_path)

files = ["Gnose Nag Hammadi.pdf", "codex-nag-hammadi.pdf"]

for f in files:
    if os.path.exists(f):
        convert_pdf_to_md(f)
    else:
        print(f"File not found: {f}")
