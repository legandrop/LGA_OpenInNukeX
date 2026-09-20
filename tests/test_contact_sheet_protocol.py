import ast
import types
import unittest
from pathlib import Path


INIT_PATH = Path(__file__).resolve().parents[1] / "init.py"


def _load_paste_function(namespace):
    tree = ast.parse(INIT_PATH.read_text(encoding="utf-8-sig"))
    function = next(
        node
        for node in ast.walk(tree)
        if isinstance(node, ast.FunctionDef)
        and node.name == "paste_clipboard_with_logging"
    )
    module = ast.Module(body=[function], type_ignores=[])
    exec(compile(module, str(INIT_PATH), "exec"), namespace)
    return namespace["paste_clipboard_with_logging"]


class FakeNode:
    def __init__(self, name, node_class, x=0, y=0):
        self._name = name
        self._class = node_class
        self._x = x
        self._y = y
        self.inputs = {}
        self.position = None
        self.selected = True

    def name(self):
        return self._name

    def Class(self):
        return self._class

    def setSelected(self, selected):
        self.selected = selected

    def setInput(self, index, node):
        self.inputs[index] = node

    def xpos(self):
        return self._x

    def ypos(self):
        return self._y

    def setXYpos(self, x, y):
        self.position = (x, y)


class FakeNuke:
    def __init__(self, selections, viewers=None):
        self._selections = list(selections)
        self._viewers = list(viewers or [])
        self.pastes = []
        self.nodes = [node for batch in selections for node in batch]

    def allNodes(self, node_class=None):
        if node_class == "Viewer":
            return self._viewers
        return self.nodes

    def nodePaste(self, path):
        self.pastes.append(path)
        return None

    def selectedNodes(self):
        return self._selections.pop(0) if self._selections else []


def _namespace(nuke, template_exists):
    fake_path = types.SimpleNamespace(
        expanduser=lambda value: value.replace("~", "C:/Users/Test"),
        exists=lambda _value: template_exists,
    )
    return {
        "nuke": nuke,
        "os": types.SimpleNamespace(path=fake_path),
        "traceback": types.SimpleNamespace(format_exc=lambda: "traceback"),
        "debug_print": lambda *_args, **_kwargs: None,
        "_flush_log": lambda: None,
        "_collect_nuke_environment": lambda: [],
        "activate_nuke_window_with_logging": lambda: None,
        "RuntimeError": RuntimeError,
    }


class ContactSheetProtocolTests(unittest.TestCase):
    def test_missing_reads_is_an_error(self):
        paste = _load_paste_function(_namespace(FakeNuke([[]]), True))
        with self.assertRaisesRegex(RuntimeError, "did not create Read nodes"):
            paste()

    def test_missing_toolset_is_an_error(self):
        read = FakeNode("Read1", "Read")
        paste = _load_paste_function(_namespace(FakeNuke([[read]]), False))
        with self.assertRaisesRegex(RuntimeError, "LGA_ContactSheet.nk was not found"):
            paste()

    def test_missing_contact_sheet_group_is_an_error(self):
        read = FakeNode("Read1", "Read")
        paste = _load_paste_function(_namespace(FakeNuke([[read], []]), True))
        with self.assertRaisesRegex(RuntimeError, "did not create the LGA_ContactSheet group"):
            paste()

    def test_success_requires_group_and_connects_reads(self):
        reads = [FakeNode("Read1", "Read", 10, 20), FakeNode("Read2", "Read", 30, 40)]
        contact_sheet = FakeNode("LGA_ContactSheet1", "Group")
        viewer = FakeNode("Viewer1", "Viewer")
        nuke = FakeNuke([reads, [contact_sheet]], [viewer])
        paste = _load_paste_function(_namespace(nuke, True))

        self.assertTrue(paste())
        self.assertEqual(contact_sheet.inputs, {0: reads[0], 1: reads[1]})
        self.assertEqual(contact_sheet.position, (20, 240))
        self.assertIs(viewer.inputs[0], contact_sheet)


if __name__ == "__main__":
    unittest.main()
