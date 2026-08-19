#!/usr/bin/env python3
"""Sincroniza las superficies de version de LGA_OpenInNukeX."""

from __future__ import annotations

import argparse
import re
import subprocess
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
INSTALLER_BAT = QT_DIR / "instalador.bat"

EXPECTED_GIT_EMAIL = "176236735+legandrop@users.noreply.github.com"

# Solo estos paths se stagean en el commit de version; nunca `git add -A`.
COMMITTABLE_PATHS = (
    "docs/ChangeLog.md",
    "VERSION",
    "README.md",
    "init.py",
    "QtClient/CMakeLists.txt",
    "QtClient/LGA_OpenInNukeX.rc",
    "QtClient/LGA_OpenInNukeX.exe.manifest",
    "QtClient/installer.iss",
)


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _write(path: Path, content: str, original: str) -> None:
    if content == original:
        return
    newline = "\r\n" if "\r\n" in original else "\n"
    # open() y no Path.write_text(newline=...): ese parametro existe recien en Python 3.10, y
    # el python3 que trae macOS es 3.9. El bug estaba latente porque solo salta cuando hay
    # algo que escribir de verdad; con las versiones ya sincronizadas nunca se llegaba aca.
    with open(path, "w", encoding="utf-8", newline=newline) as handle:
        handle.write(content)


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


def _read_single_key() -> str | None:
    """Lee UNA tecla sin esperar Enter. None si la consola no lo permite (pipe, CI)."""
    if not sys.stdin.isatty():
        return None
    try:
        import msvcrt  # type: ignore[import-not-found]  # Windows
    except ImportError:
        pass
    else:
        return msvcrt.getwch()

    try:
        import termios
        import tty
    except ImportError:
        return None

    fd = sys.stdin.fileno()
    saved = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        return sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, saved)


def _prompt_yes_no(prompt: str) -> bool:
    """Pregunta Y/N con una sola tecla, igual que `choice /C YN` en instalador.bat."""
    question = f"{prompt} [Y,N]?"
    while True:
        print(question, end="", flush=True)
        key = _read_single_key()
        if key is None:
            raw = input(" ").strip().lower()
            if raw in {"y", "s", "si", "yes"}:
                return True
            if raw in {"n", "no"}:
                return False
            print("Respuesta invalida. Use Y o N.")
            continue

        if key in ("\x03", "\x04"):
            print()
            raise KeyboardInterrupt

        answer = key.upper()
        if answer in ("Y", "S"):
            print("Y")
            return True
        if answer == "N":
            print("N")
            return False
        print()


def _git(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        cwd=ROOT_DIR,
        check=check,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def _changed_version_files() -> list[str]:
    existing = [path for path in COMMITTABLE_PATHS if (ROOT_DIR / path).exists()]
    if not existing:
        return []
    result = _git("status", "--porcelain", "--", *existing, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"git status fallo: {result.stderr.strip()}")
    changed = []
    for line in result.stdout.splitlines():
        path = line[3:].strip().strip('"')
        if path:
            changed.append(path)
    return changed


def _maybe_commit(version: str, *, mode: str) -> bool:
    """Ofrece commitear (y pushear) el bump. `mode`: ask | yes | no."""
    if mode == "no":
        return False

    if _git("rev-parse", "--is-inside-work-tree", check=False).returncode != 0:
        print("No es un repo de Git: se omite el commit.")
        return False

    changed = _changed_version_files()
    if not changed:
        print("No hay cambios de version para commitear.")
        return False

    print("\nArchivos de version modificados:")
    for path in changed:
        print(f"  {path}")

    if mode == "ask" and not _prompt_yes_no(f"Commitear el bump a v{version}?"):
        print("Commit omitido. Los cambios quedan en el working tree.")
        return False

    email = _git("config", "user.email", check=False).stdout.strip()
    if email != EXPECTED_GIT_EMAIL:
        print(f"ERROR: git user.email es {email!r} y se esperaba {EXPECTED_GIT_EMAIL!r}.")
        print("Corregir la identidad de Git antes de commitear. Commit cancelado.")
        return False

    _git("add", "--", *changed)
    message = f"Version - Bump a v{version}"
    commit = _git("commit", "-m", message, check=False)
    print(commit.stdout.strip() or commit.stderr.strip())
    if commit.returncode != 0:
        print("El commit fallo. Los cambios quedan stageados.")
        return False

    if mode == "ask" and not _prompt_yes_no("Pushear a origin?"):
        print("Push omitido. El commit queda local.")
        return True

    push = _git("push", check=False)
    print(push.stdout.strip() or push.stderr.strip())
    if push.returncode != 0:
        print("El push fallo. Revisar credenciales y reintentar a mano.")
    return True


def _maybe_run_installer(*, mode: str) -> None:
    """Ofrece correr `QtClient/instalador.bat` despues del commit. `mode`: ask | yes | no."""
    if mode == "no":
        return

    if sys.platform != "win32":
        if mode == "yes":
            print("`instalador.bat` es solo de Windows: se omite.")
        return

    if not INSTALLER_BAT.exists():
        print(f"No se encontro {INSTALLER_BAT.name}: se omite la instalacion.")
        return

    if mode == "ask" and not _prompt_yes_no("Correr instalador.bat ahora?"):
        print("Instalador omitido.")
        return

    print(f"\nEjecutando {INSTALLER_BAT.name}...\n")
    result = subprocess.run(["cmd", "/c", str(INSTALLER_BAT)], cwd=QT_DIR, check=False)
    if result.returncode != 0:
        print(f"El instalador termino con codigo {result.returncode}.")


def _build_updates(display: str, cmake_value: str, windows_value: str, windows_csv: str) -> list[tuple[Path, str, str]]:
    changelog = _read(CHANGELOG_MD)
    version_file = _read(VERSION_FILE)
    cmake = _read(CMAKE_FILE)

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

    return updates


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Sincroniza las superficies de version de LGA_OpenInNukeX.",
    )
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Solo verifica; no escribe ni pregunta commit/instalador.",
    )
    parser.add_argument(
        "--interactive",
        action="store_true",
        help="Al terminar, preguntar commit y instalador (lo usa bump_version.bat).",
    )
    commit_group = parser.add_mutually_exclusive_group()
    commit_group.add_argument(
        "--commit",
        dest="commit_mode",
        action="store_const",
        const="yes",
        help="Commit y push del bump sin preguntar.",
    )
    commit_group.add_argument(
        "--no-commit",
        dest="commit_mode",
        action="store_const",
        const="no",
        help="No ofrecer commit (default de sync_version.bat).",
    )
    install_group = parser.add_mutually_exclusive_group()
    install_group.add_argument(
        "--install",
        dest="install_mode",
        action="store_const",
        const="yes",
        help="Correr instalador.bat tras commitear, sin preguntar (solo Windows).",
    )
    install_group.add_argument(
        "--no-install",
        dest="install_mode",
        action="store_const",
        const="no",
        help="No ofrecer instalador.",
    )
    parser.set_defaults(commit_mode="no", install_mode="no")
    args = parser.parse_args()
    if args.interactive:
        args.commit_mode = "ask"
        args.install_mode = "ask"
    return args


def main() -> int:
    args = _parse_args()

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

    updates = _build_updates(display, cmake_value, windows_value, windows_csv)

    print(f"[sync_version] ChangeLog: {changelog_version}")
    print(f"[sync_version] VERSION:   {file_version}")
    print(f"[sync_version] CMake:     {cmake_current}")
    print(f"[sync_version] Resolved:  {display}")

    # --check-only: NO escribe nada, solo reporta si alguna superficie quedaria distinta.
    # Lo usa github_release_mac.sh antes de publicar. Sin este modo, el release corria el
    # sync completo: como el chequeo de working tree limpio ya habia pasado, el script podia
    # MODIFICAR archivos y despues taggear un commit que no coincide con lo empaquetado.
    if args.check_only:
        stale = [
            str(path.relative_to(ROOT_DIR))
            for path, updated, original in updates
            if updated != original
        ]
        if stale:
            print("[sync_version] DESINCRONIZADO. Quedarian distintos:")
            for name in stale:
                print(f"  - {name}")
            return 1
        print("[sync_version] Superficies sincronizadas correctamente (check-only).")
        return 0

    for path, updated, original in updates:
        _write(path, updated, original)

    print("[sync_version] Superficies sincronizadas correctamente.")

    # El instalador se ofrece solo si el bump quedo commiteado: instalador.bat valida el
    # estado del repo y publica en GitHub; correrlo sobre un working tree sucio no sirve.
    if _maybe_commit(display, mode=args.commit_mode):
        _maybe_run_installer(mode=args.install_mode)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # pylint: disable=broad-except
        print(f"[sync_version] ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
