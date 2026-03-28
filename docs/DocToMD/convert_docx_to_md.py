from __future__ import annotations

import re
import shutil
from pathlib import Path
from zipfile import ZipFile
from xml.etree import ElementTree as ET


ROOT_DIR = Path("/Users/leg4/.nuke/LGA_OpenInNukeX")
DOCX_PATH = ROOT_DIR / "docs" / "DocToMD" / "LGA_OpenInNukeX.docx"
OUTPUT_MD = ROOT_DIR / "docs" / "DocToMD" / "README.md"
MEDIA_ORIGINAL = ROOT_DIR / "docs" / "DocToMD" / "media_original"
MEDIA_CONVERTED = ROOT_DIR / "docs" / "DocToMD" / "media_converted"

NS = {
    "w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
    "a": "http://schemas.openxmlformats.org/drawingml/2006/main",
    "r": "http://schemas.openxmlformats.org/officeDocument/2006/relationships",
}


def clean_text(text: str) -> str:
    text = text.replace("\xa0", " ")
    text = text.replace("\u2028", " ")
    text = re.sub(r"\s+", " ", text)
    return text.strip()


def normalize_text(text: str) -> str:
    replacements = {
        "": " ",
    }
    for source, target in replacements.items():
        text = text.replace(source, target)
    text = text.replace(".Si", ". Si")
    return clean_text(text)


def paragraph_text(paragraph: ET.Element) -> str:
    parts = []
    for child in paragraph.iter():
        tag = child.tag.rsplit("}", 1)[-1]
        if tag == "t" and child.text:
            parts.append(child.text)
        elif tag in {"tab", "br"}:
            parts.append("\n")
    return normalize_text("".join(parts))


def paragraph_images(paragraph: ET.Element, rel_map: dict[str, str]) -> list[str]:
    images: list[str] = []
    for blip in paragraph.findall(".//a:blip", NS):
        embed = blip.attrib.get(f"{{{NS['r']}}}embed")
        target = rel_map.get(embed)
        if target:
            images.append(target)
    return images


def table_text(table: ET.Element) -> str:
    parts = []
    for cell in table.findall(".//w:t", NS):
        if cell.text:
            parts.append(cell.text)
    return normalize_text("".join(parts))


def prepare_media() -> None:
    if MEDIA_ORIGINAL.exists():
        shutil.rmtree(MEDIA_ORIGINAL)
    if MEDIA_CONVERTED.exists():
        shutil.rmtree(MEDIA_CONVERTED)

    MEDIA_ORIGINAL.mkdir(parents=True, exist_ok=True)
    MEDIA_CONVERTED.mkdir(parents=True, exist_ok=True)

    with ZipFile(DOCX_PATH) as docx:
        for name in docx.namelist():
            if not name.startswith("word/media/"):
                continue
            filename = Path(name).name
            data = docx.read(name)
            (MEDIA_ORIGINAL / filename).write_bytes(data)
            (MEDIA_CONVERTED / filename).write_bytes(data)


def main() -> None:
    prepare_media()

    with ZipFile(DOCX_PATH) as docx:
        document = ET.fromstring(docx.read("word/document.xml"))
        rels = ET.fromstring(docx.read("word/_rels/document.xml.rels"))
        rel_map = {
            rel.attrib["Id"]: rel.attrib["Target"].replace("\\", "/")
            for rel in rels
            if rel.attrib.get("Target", "").startswith("media/")
        }
        body = document.find("w:body", NS)
        if body is None:
            raise RuntimeError("No se encontró word/body en el docx")

        lines: list[str] = []
        pending_images: list[str] = []

        for child in body:
            tag = child.tag.rsplit("}", 1)[-1]

            if tag == "tbl":
                code = table_text(child)
                if code:
                    lines.extend(["```python", code, "```", ""])
                continue

            if tag != "p":
                continue

            text = paragraph_text(child)
            images = [
                f"media_converted/{Path(path).name}"
                for path in paragraph_images(child, rel_map)
            ]

            if images and not text:
                pending_images.extend(images)
                continue

            if pending_images:
                for image in pending_images:
                    lines.append(f"![]({image})")
                    lines.append("")
                pending_images.clear()

            if text == "Instalación:":
                lines.extend(["## Instalación", ""])
                continue

            if text.startswith("OpenInNukeX v"):
                lines.extend([f"# {text}", ""])
                continue

            if text.startswith("Lega | "):
                lines.extend([text, ""])
                continue

            if text.startswith("* "):
                lines.append(f"- {text[2:]}")
                continue

            if text.startswith(("Copiar la carpeta ", "Con un editor de texto", "Ejecutar ")):
                lines.append(f"- {text}")
                continue

            lines.extend([text, ""])

        if pending_images:
            for image in pending_images:
                lines.append(f"![]({image})")
                lines.append("")

    OUTPUT_MD.write_text("\n".join(lines).strip() + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
