"""The Localization menu in the Editor's menu bar: Localization > Check Localization runs
Tools/check_localization.py for the open project and prints the report to the Console.

How a menu item gets into the Editor (Action Manager):
  1. bootstrap.py calls register_menu() when the Editor starts, before the menus are built.
  2. The Editor then calls the registration hooks in a fixed order; we answer three of them:
     OnActionRegistrationHook - register the action (name + the function to call on click),
     OnMenuRegistrationHook   - register our menu,
     OnMenuBindingHook        - put the action into the menu, and the menu into the menu bar.
"""

import importlib
import sys
import traceback
from pathlib import Path

import azlmbr.action
import azlmbr.bus
import azlmbr.paths

# Identifiers of the Editor itself (EditorContextIdentifiers.h, EditorMenuIdentifiers.h).
EDITOR_ACTION_CONTEXT = "o3de.context.editor.mainwindow"
EDITOR_MENU_BAR = "o3de.menubar.editor.mainwindow"

# Our identifiers: any unique strings. The gem name in front keeps them from clashing with other gems.
LOCALIZATION_MENU = "localizationgem.menu.localization"
CHECK_ACTION = "localizationgem.action.check"

# Place in the menu bar: File 100, Edit 200, Game 300, Tools 400, View 500, Help 600 (EditorActionsHandler.cpp).
MENU_BAR_SORT_KEY = 450

# check_localization.py lives in <gem>/Tools; this file is <gem>/Editor/Scripts/localization_gem_editor.py.
TOOLS_FOLDER = Path(__file__).resolve().parents[2] / "Tools"

# The bus handler must stay referenced: a Python object nobody refers to is destroyed,
# and the hooks would never be called.
_registration_handler = None


def register_menu() -> None:
    """Subscribes to the Action Manager registration hooks. Called once, from bootstrap.py."""
    global _registration_handler
    if _registration_handler is not None:
        return

    handler = azlmbr.action.ActionManagerRegistrationNotificationBusHandler()
    handler.connect()
    handler.add_callback("OnActionRegistrationHook", _on_action_registration)
    handler.add_callback("OnMenuRegistrationHook", _on_menu_registration)
    handler.add_callback("OnMenuBindingHook", _on_menu_binding)
    _registration_handler = handler


def _on_action_registration(parameters) -> None:
    properties = azlmbr.action.ActionProperties()
    properties.name = "Check Localization"
    properties.description = "Checks the string tables, languages.csv and the keys used in prefabs and code. " \
                             "The report goes to the Console."
    properties.category = "Localization"
    azlmbr.action.ActionManagerPythonRequestBus(
        azlmbr.bus.Broadcast, "RegisterAction", EDITOR_ACTION_CONTEXT, CHECK_ACTION, properties, _on_check_localization)


def _on_menu_registration(parameters) -> None:
    properties = azlmbr.action.MenuProperties()
    properties.name = "Localization"
    azlmbr.action.MenuManagerPythonRequestBus(azlmbr.bus.Broadcast, "RegisterMenu", LOCALIZATION_MENU, properties)


def _on_menu_binding(parameters) -> None:
    azlmbr.action.MenuManagerPythonRequestBus(
        azlmbr.bus.Broadcast, "AddActionToMenu", LOCALIZATION_MENU, CHECK_ACTION, 100)
    azlmbr.action.MenuManagerPythonRequestBus(
        azlmbr.bus.Broadcast, "AddMenuToMenuBar", EDITOR_MENU_BAR, LOCALIZATION_MENU, MENU_BAR_SORT_KEY)


def _on_check_localization() -> None:
    """Called on click. Any error is printed here: an exception that escaped into the Editor's C++ code
    would not be reported properly."""
    try:
        if str(TOOLS_FOLDER) not in sys.path:
            sys.path.append(str(TOOLS_FOLDER))

        # Python imports a module only once. reload() reads the file again, so an edited script
        # is used right away, without restarting the Editor.
        checker = importlib.reload(importlib.import_module("check_localization"))

        checker.main([str(Path(azlmbr.paths.projectroot))])
    except (Exception, SystemExit):  # SystemExit: argparse exits on bad arguments
        traceback.print_exc()
