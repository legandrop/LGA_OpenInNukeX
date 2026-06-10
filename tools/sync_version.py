#!/usr/bin/env python3
"""Sincroniza las superficies de version de LGA_OpenInNukeX."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
QT_DIR = ROOT_DIR / "QtClient"
CHANGELOG_MD = ROOT_DIR / "docs" / "ChangeLog.md"
VERSION_FILE = ROOT_DIR / "VERSION"
CMAKE_FILE = QT_DIR / "CMakeLists.txt"
README_FILE = ROOT_DIR / "README.md"
INIT_FILE = ROOT_DIR / "init.py"
RC_FILE = QT_DIR / "LGA_OpenInNukeX.rc"
MANIFEST_FILE = QT_DIR / "LGA_OpenInNukeX.exe.manifest"
INSTALLER_FILE = QT_DIR / "installer.iss"


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _write(path: Path, content: str, original: str) -> None:
    if content == original:
        return
    newline = "\r\n" if "\r\n" in original else "\n"
    path.write_text(content, encoding="utf-8", newline=newline)


def _parse_version(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in version.split("."))


def _display_version(version: str) -> str:
    parts = list(_parse_version(version))
    while len(parts) > 2 and parts[-1] == 0:
        parts.pop()
    return ".".join(str(part) for part in parts)


def _cmake_version(version: str) -> str:
    parts = list(_parse_version(version))
    while len(parts) < 3:
        parts.append(0)
    return ".".join(str(part) for part in parts[:3])


def _windows_version(version: str) -> str:
    parts = list(_parse_version(version))
    while len(parts) < 4:
        parts.append(0)
    return ".".join(str(part) for part in parts[:4])


def _windows_version_csv(version: str) -> str:
    return ",".join(_windows_version(version).split("."))


def _extract(pattern: str, content: str, source: str) -> str:
    match = re.search(pattern, content, flags=re.MULTILINE)
    if not match:
        raise ValueError(f"No se pudo detectar la version en {source}")
    return match.group(1)


def _replace(
    content: str,
    pattern: str,
    replacement,
    source: str,
    *,
    count: int = 1,
) -> str:
    updated, replacements = re.subn(
        pattern,
        replacement,
        content,
        count=count,
        flags=re.MULTILINE,
    )
    if replacements == 0:
        raise ValueError(f"No se pudo actualizar la version en {source}")
    return updated


def main() -> int:
    changelog = _read(CHANGELOG_MD)
    version_file = _read(VERSION_FILE)
    cmake = _read(CMAKE_FILE)

    changelog_version = _extract(
        r"^\s*v([0-9]+(?:\.[0-9]+)+)\s*:",
        changelog,
        "docs/ChangeLog.md",
    )
    file_version = version_file.strip()
    cmake_current = _extract(
        r"project\(\s*LGA_OpenInNukeX\s+VERSION\s+([0-9]+(?:\.[0-9]+)+)",
        cmake,
        "QtClient/CMakeLists.txt",
    )

    resolved = max(
        (changelog_version, file_version, cmake_current),
        key=_parse_version,
    )
    display = _display_version(resolved)
    cmake_value = _cmake_version(resolved)
    windows_value = _windows_version(resolved)
    windows_csv = _windows_version_csv(resolved)

    updates: list[tuple[Path, str, str]] = []

    new_changelog = _replace(
        changelog,
        r"(^\s*v)([0-9]+(?:\.[0-9]+)+)(\s*:)",
        lambda match: f"{match.group(1)}{display}{match.group(3)}",
        "docs/ChangeLog.md",
    )
    updates.append((CHANGELOG_MD, new_changelog, changelog))

    updates.append((VERSION_FILE, f"{display}\n", version_file))

    new_cmake = _replace(
        cmake,
        r"(project\(\s*LGA_OpenInNukeX\s+VERSION\s+)([0-9]+(?:\.[0-9]+)+)",
        lambda match: f"{match.group(1)}{cmake_value}",
        "QtClient/CMakeLists.txt",
    )
    updates.append((CMAKE_FILE, new_cmake, cmake))

    readme = _read(README_FILE)
    new_readme = _replace(
        readme,
        r"(Lega\s*\|\s*v)([0-9]+(?:\.[0-9]+)+)",
        lambda match: f"{match.group(1)}{display}",
        "README.md",
    )
    updates.append((README_FILE, new_readme, readme))

    init_content = _read(INIT_FILE)
    new_init = _replace(
        init_content,
        r"(LGA_OpenInNukeX\s+v)([0-9]+(?:\.[0-9]+)+)(\s*\|\s*Lega)",
        lambda match: f"{match.group(1)}{display}{match.group(3)}",
        "init.py",
    )
    updates.append((INIT_FILE, new_init, init_content))

    rc_content = _read(RC_FILE)
    new_rc = _replace(
        rc_content,
        r"^(FILEVERSION|PRODUCTVERSION)\s+[0-9,]+",
        lambda match: f"{match.group(1)} {windows_csv}",
        "QtClient/LGA_OpenInNukeX.rc",
        count=0,
    )
    new_rc = _replace(
        new_rc,
        r'(VALUE "(?:FileVersion|ProductVersion)", ")[0-9.]+(")',
        lambda match: f"{match.group(1)}{windows_value}{match.group(2)}",
        "QtClient/LGA_OpenInNukeX.rc",
        count=0,
    )
    updates.append((RC_FILE, new_rc, rc_content))

    manifest = _read(MANIFEST_FILE)
    new_manifest = _replace(
        manifest,
        r'(<assemblyIdentity\s+version=")[0-9.]+(")',
        lambda match: f"{match.group(1)}{windows_value}{match.group(2)}",
        "QtClient/LGA_OpenInNukeX.exe.manifest",
    )
    updates.append((MANIFEST_FILE, new_manifest, manifest))

    installer = _read(INSTALLER_FILE)
    new_installer = _replace(
        installer,
        r"(LGA OpenInNukeX v)[0-9.]+(\s+Installer)",
        lambda match: f"{match.group(1)}{display}{match.group(2)}",
        "QtClient/installer.iss",
    )
    new_installer = _replace(
        new_installer,
        r'(#define MyAppVersion ")[0-9.]+(")',
        lambda match: f"{match.group(1)}{display}{match.group(2)}",
        "QtClient/installer.iss",
    )
    updates.append((INSTALLER_FILE, new_installer, installer))

    for path, updated, original in updates:
        _write(path, updated, original)

    print(f"[sync_version] ChangeLog: {changelog_version}")
    print(f"[sync_version] VERSION:   {file_version}")
    print(f"[sync_version] CMake:     {cmake_current}")
    print(f"[sync_version] Resolved:  {display}")
    print("[sync_version] Superficies sincronizadas correctamente.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # pylint: disable=broad-except
        print(f"[sync_version] ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
