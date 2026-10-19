"""Runs when the Editor starts: the EditorPythonBindings gem runs Editor/Scripts/bootstrap.py
of every active gem (and adds that folder to sys.path).

All bootstrap.py files of all gems run one after another in the same __main__ namespace,
so a global variable here could be overwritten by another gem's bootstrap. That is why
the code lives in its own module: names inside a module belong to that module only.
"""

import sys
from pathlib import Path

# Python also runs in other tools (Material Editor, Asset Processor...): the menu is for the Editor only.
if __name__ == "__main__" and Path(sys.executable).stem.lower() == "editor":
    import localization_gem_editor

    localization_gem_editor.register_menu()
