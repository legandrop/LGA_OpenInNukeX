"""
Temporary probe to inspect which Nuke callbacks fire during script loading.

Run this from Nuke's Script Editor or with:

    exec(open(r"C:/Users/leg4-pc/.nuke/LGA_OpenInNukeX/tools/nuke_script_load_probe.py").read(), globals())

Then call:

    run_probe(r"T:/path/to/script.nk")

The probe writes synchronously to:
    C:/Users/leg4-pc/.nuke/LGA_OpenInNukeX/logs/nuke_script_load_probe.log
"""

from __future__ import print_function

import os
import io
import sys
import time
import traceback
from contextlib import redirect_stdout

import nuke

try:
    import nukescripts
except Exception:
    nukescripts = None


def _resolve_base_dir():
    candidates = []

    file_value = globals().get("__file__")
    if file_value:
        candidates.append(os.path.abspath(file_value))

    # Common case when this probe is executed via exec(open(...).read(), globals()).
    candidates.append(
        os.path.join(
            os.path.expanduser("~"),
            ".nuke",
            "LGA_OpenInNukeX",
            "tools",
            "nuke_script_load_probe.py",
        )
    )

    for candidate in candidates:
        normalized = os.path.normpath(candidate)
        if normalized.endswith(
            os.path.normpath(
                os.path.join("LGA_OpenInNukeX", "tools", "nuke_script_load_probe.py")
            )
        ):
            return os.path.dirname(os.path.dirname(normalized))

    return os.path.join(os.path.expanduser("~"), ".nuke", "LGA_OpenInNukeX")


BASE_DIR = _resolve_base_dir()
LOG_PATH = os.path.join(BASE_DIR, "logs", "nuke_script_load_probe.log")


def _write_line(message):
    directory = os.path.dirname(LOG_PATH)
    if directory and not os.path.isdir(directory):
        os.makedirs(directory)

    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    line = "[%s] %s\n" % (timestamp, message)

    with open(LOG_PATH, "a", encoding="utf-8") as handle:
        handle.write(line)
        handle.flush()
        os.fsync(handle.fileno())

    try:
        nuke.tprint(message)
    except Exception:
        pass


def _reset_log():
    with open(LOG_PATH, "w", encoding="utf-8") as handle:
        handle.write("Probe started at %s\n" % time.strftime("%Y-%m-%d %H:%M:%S"))
        handle.flush()
        os.fsync(handle.fileno())


def _safe_node_label():
    try:
        node = nuke.thisNode()
    except Exception:
        return "<no thisNode>"

    try:
        return "%s '%s'" % (node.Class(), node.name())
    except Exception:
        return repr(node)


def _list_callback_apis():
    names = []
    for name in dir(nuke):
        if name.startswith(("addOn", "removeOn", "addBefore", "removeBefore", "addAfter", "removeAfter")):
            names.append(name)

    for name in sorted(names):
        _write_line("API %s" % name)


def _dump_registered_callbacks():
    if not nukescripts or not hasattr(nukescripts, "print_callback_info"):
        _write_line("nukescripts.print_callback_info unavailable")
        return

    capture = io.StringIO()
    try:
        with redirect_stdout(capture):
            result = nukescripts.print_callback_info()
    except Exception as exc:
        _write_line("print_callback_info failed: %s" % exc)
        return

    output = capture.getvalue().strip()
    if output:
        _write_line("Registered callbacks dump follows")
        for line in output.splitlines():
            _write_line("  CALLBACK %s" % line.rstrip())
    else:
        _write_line("print_callback_info produced no stdout")

    if result is not None:
        _write_line("print_callback_info returned: %r" % (result,))


def run_probe(script_path):
    if not script_path:
        raise ValueError("script_path is required")

    _reset_log()
    _write_line("Python %s" % sys.version.replace("\n", " "))
    _write_line("Nuke env: gui=%s studio=%s nukex=%s" % (
        nuke.env.get("gui"),
        nuke.env.get("studio"),
        nuke.env.get("nukex"),
    ))
    _write_line("Target script: %s" % script_path)
    _list_callback_apis()

    if not os.path.exists(script_path):
        _write_line("Target script does not exist on this machine")
        return False

    counters = {
        "on_create": 0,
        "on_script_load": 0,
        "on_script_close": 0,
        "on_destroy": 0,
    }

    def _on_create():
        counters["on_create"] += 1
        _write_line("[OnCreate #%d] %s" % (counters["on_create"], _safe_node_label()))

    def _on_script_load():
        counters["on_script_load"] += 1
        _write_line("[OnScriptLoad #%d] root=%s" % (
            counters["on_script_load"],
            nuke.root().name(),
        ))

    def _on_script_close():
        counters["on_script_close"] += 1
        _write_line("[OnScriptClose #%d] root=%s" % (
            counters["on_script_close"],
            nuke.root().name(),
        ))

    def _on_destroy():
        counters["on_destroy"] += 1
        _write_line("[OnDestroy #%d] %s" % (counters["on_destroy"], _safe_node_label()))

    _write_line("Registering callbacks")
    nuke.addOnCreate(_on_create)
    nuke.addOnScriptLoad(_on_script_load)
    nuke.addOnScriptClose(_on_script_close)
    nuke.addOnDestroy(_on_destroy)
    _dump_registered_callbacks()

    start = time.time()
    try:
        _write_line("Calling nuke.scriptClose()")
        nuke.scriptClose()
        _write_line("Calling nuke.scriptOpen(%r)" % script_path)
        nuke.scriptOpen(script_path)
        elapsed = time.time() - start
        _write_line("scriptOpen finished in %.3fs" % elapsed)
        _write_line("Final root name: %s" % nuke.root().name())
        _write_line("Node count after load: %d" % len(nuke.allNodes(recurseGroups=True)))
        return True
    except Exception as exc:
        _write_line("Probe failed: %s" % exc)
        _write_line(traceback.format_exc())
        return False
    finally:
        _write_line("Removing callbacks")
        for remover, callback in (
            (nuke.removeOnCreate, _on_create),
            (nuke.removeOnScriptLoad, _on_script_load),
            (nuke.removeOnScriptClose, _on_script_close),
            (nuke.removeOnDestroy, _on_destroy),
        ):
            try:
                remover(callback)
            except Exception as exc:
                _write_line("Callback removal failed: %s" % exc)

        _write_line("Counters: %s" % counters)
        _dump_registered_callbacks()
