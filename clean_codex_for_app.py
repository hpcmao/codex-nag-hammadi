#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script de nettoyage du Codex Nag Hammadi pour l'application.
Version optimisee pour fichiers volumineux.
"""

import re
import sys
from pathlib import Path
from typing import Tuple, List, Dict
from dataclasses import dataclass


@dataclass
class Treatise:
    code: str
    title: str
    start_pos: int
    end_pos: int = 0


# Precompiler les regex pour performance
RE_PAGE_HEADER = re.compile(r'^## Page \d+\s*$', re.MULTILINE)
RE_SEPARATOR = re.compile(r'^---\s*$', re.MULTILINE)
RE_PIPE = re.compile(r'\s*\|\s*')
RE_LACUNA_BRACKETS = re.compile(r'\[[\s.]+\]')
RE_DOTS_SPACED = re.compile(r'\.\s+\.\s+\.[\s.]*')
RE_DOTS_MANY = re.compile(r'\.{4,}')
RE_BRACKET_SIMPLE = re.compile(r'\[([^\[\]]{1,60})\]')
RE_CHEVRON = re.compile(r'<([^<>]+)>')
RE_NOTE = re.compile(r'\(Note\*?\)')
RE_BROKEN_WORD = re.compile(r'\b([A-Z])\s+([a-z])')
RE_NH_REF = re.compile(r'\s*\(NH\s+[IVX]+,\s*\d+\)')
RE_MULTI_SPACE = re.compile(r' {2,}')
RE_MULTI_NL = re.compile(r'\n{3,}')
RE_NH_CODE = re.compile(r'\(NH\s+([IVX]+),\s*(\d+)\)')

# Patterns pour les numeros de versets
RE_VERSE_INLINE = re.compile(r'(\s)(\d{1,2})(\s)([A-Za-zÀ-ÿ«"])')
RE_VERSE_END = re.compile(r'(\s)(\d{1,2})(\s*[.!?»"]?\s*)$')
RE_LONE_NUM = re.compile(r'^\s*\d{1,3}\s*$', re.MULTILINE)

# Lignes a supprimer
RE_TRADUCTION = re.compile(r'^Traduction\s+de\s+.+$', re.MULTILINE)
RE_LACUNE_PARENTHESE = re.compile(r'\(Lacune[^)]*\)')
RE_DECORATION = re.compile(r'\(Décoration[^)]*\)')
RE_NOTE_STAR = re.compile(r'\(\*[^)]*\)')


def clean_for_narration(text: str) -> str:
    """Nettoie le texte pour la narration TTS"""
    # 1. Supprimer les headers et separateurs
    text = RE_PAGE_HEADER.sub('', text)
    text = RE_SEPARATOR.sub('', text)

    # 2. Supprimer les pipes et lacunes
    text = RE_PIPE.sub(' ', text)
    text = RE_LACUNA_BRACKETS.sub('', text)
    text = RE_DOTS_SPACED.sub('... ', text)
    text = RE_DOTS_MANY.sub('...', text)

    # 3. Supprimer les crochets (plusieurs passes)
    for _ in range(3):
        new_text = RE_BRACKET_SIMPLE.sub(r'\1', text)
        if new_text == text:
            break
        text = new_text

    # 4. Supprimer chevrons et notes
    text = RE_CHEVRON.sub(r'\1', text)
    text = RE_NOTE.sub('', text)
    text = RE_LACUNE_PARENTHESE.sub('', text)
    text = RE_DECORATION.sub('', text)
    text = RE_NOTE_STAR.sub('', text)

    # 5. Supprimer les lignes de traduction
    text = RE_TRADUCTION.sub('', text)

    # 6. Supprimer les numeros de versets inline (plusieurs passes)
    for _ in range(3):
        text = RE_VERSE_INLINE.sub(r'\1\4', text)
    text = RE_VERSE_END.sub(r'\3', text)
    text = RE_LONE_NUM.sub('', text)

    # 7. Corriger les espaces casses
    text = RE_BROKEN_WORD.sub(r'\1\2', text)
    text = RE_NH_REF.sub('', text)

    # 8. Supprimer les titres repetes
    lines = text.split('\n')
    cleaned_lines = []
    seen_titles = set()

    for line in lines:
        stripped = line.strip()

        # Ignorer les lignes vides
        if not stripped:
            cleaned_lines.append(line)
            continue

        # Detecter les titres (tout en majuscules, > 5 chars)
        if len(stripped) > 5 and stripped.isupper():
            normalized = stripped[:30]
            if normalized in seen_titles:
                continue  # Skip repetition
            seen_titles.add(normalized)

        cleaned_lines.append(line)

    text = '\n'.join(cleaned_lines)

    # 9. Nettoyer les espaces
    text = RE_MULTI_SPACE.sub(' ', text)
    text = RE_MULTI_NL.sub('\n\n', text)

    return text.strip()


def clean_for_display(text: str) -> str:
    """Nettoie le texte pour l'affichage"""
    text = RE_PIPE.sub(' ', text)
    text = RE_LACUNA_BRACKETS.sub('[...]', text)
    text = RE_DOTS_SPACED.sub('...', text)
    text = RE_NOTE.sub('', text)
    text = RE_BROKEN_WORD.sub(r'\1\2', text)
    text = RE_MULTI_SPACE.sub(' ', text)
    text = RE_MULTI_NL.sub('\n\n', text)
    return text.strip()


def extract_title_before_nh(content: str, pos: int) -> str:
    """Extrait le titre qui precede le pattern (NH X, Y)"""
    start = max(0, pos - 200)
    before = content[start:pos]

    lines = before.split('\n')
    for line in reversed(lines):
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith('Traduction') or stripped.startswith('traduction'):
            continue
        cleaned = stripped.strip('<>').strip()
        if cleaned:
            return cleaned

    return "Traite inconnu"


def parse_treatises(content: str) -> List[Treatise]:
    """Parse et deduplique les traites"""
    matches = list(RE_NH_CODE.finditer(content))
    print(f"Patterns (NH X, Y) trouves: {len(matches)}")

    seen: Dict[str, Treatise] = {}
    for m in matches:
        codex = m.group(1)
        num = m.group(2)
        code = f"{codex}-{num}"

        if code not in seen:
            title = extract_title_before_nh(content, m.start())
            line_start = content.rfind('\n', 0, m.start()) + 1
            seen[code] = Treatise(code, title, line_start)

    treatises = sorted(seen.values(), key=lambda t: t.start_pos)

    for i in range(len(treatises) - 1):
        treatises[i].end_pos = treatises[i + 1].start_pos
    if treatises:
        treatises[-1].end_pos = len(content)

    print(f"Traites uniques: {len(treatises)}")
    for t in treatises[:5]:
        print(f"  - {t.code}: {t.title[:50]}...")
    if len(treatises) > 5:
        print(f"  ... et {len(treatises) - 5} autres")

    return treatises


def process_file(input_path: Path) -> Tuple[str, str]:
    """Traite le fichier"""
    print(f"Lecture...")
    content = input_path.read_text(encoding='utf-8')
    print(f"Taille: {len(content):,} caracteres")

    treatises = parse_treatises(content)

    if not treatises:
        return clean_for_display(content), clean_for_narration(content)

    display_parts = ["# Codex de Nag Hammadi\n\n"]
    narration_parts = []

    print(f"Nettoyage de {len(treatises)} traites...")
    for i, t in enumerate(treatises):
        if (i + 1) % 10 == 0:
            print(f"  {i + 1}/{len(treatises)}...")

        chunk = content[t.start_pos:t.end_pos]

        display_parts.append(f"## {t.code} - {t.title}\n\n")
        display_parts.append(clean_for_display(chunk))
        display_parts.append("\n\n---\n\n")

        # Pour narration: nettoyer le texte et supprimer le titre en double
        cleaned = clean_for_narration(chunk)
        # Supprimer le titre au debut s'il correspond
        title_upper = t.title.upper()
        lines = cleaned.split('\n')
        # Supprimer les premieres lignes qui sont le titre ou vides
        while lines:
            first_line = lines[0].strip()
            if not first_line:
                lines.pop(0)
            elif first_line.upper() == title_upper or first_line.upper().startswith(title_upper[:20]):
                lines.pop(0)
            else:
                break
        cleaned = '\n'.join(lines)

        narration_parts.append(f"\n\n{t.title}\n\n")
        narration_parts.append(cleaned)

    print("  Termine!")
    return ''.join(display_parts), ''.join(narration_parts)


def main():
    input_file = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("Gnose Nag Hammadi.md")

    if not input_file.exists():
        print(f"Erreur: {input_file} non trouve")
        sys.exit(1)

    print("=" * 50)
    print("Nettoyage Codex Nag Hammadi")
    print("=" * 50)

    display, narration = process_file(input_file)

    output_dir = Path("texts_clean")
    output_dir.mkdir(exist_ok=True)

    stem = input_file.stem
    (output_dir / f"{stem}_display.md").write_text(display, encoding='utf-8')
    (output_dir / f"{stem}_narration.md").write_text(narration, encoding='utf-8')

    print(f"\nFichiers generes dans {output_dir}/")
    print(f"  - {stem}_display.md ({len(display):,} car.)")
    print(f"  - {stem}_narration.md ({len(narration):,} car.)")

    print(f"\n--- Extrait narration (debut) ---")
    print(narration[:1200])
    print("...")


if __name__ == "__main__":
    main()
