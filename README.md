# Localization Gem

Key-based localization of player-visible text for [O3DE](https://o3de.org): *code and prefabs store a key and a default text, the gem turns the key into text in the player's language*.

Texts live in plain CSV tables, one file per language, that you can edit in Excel or LibreOffice. The gem loads them, picks the starting language from the operating system, falls back to English for anything that is not translated, and switches languages at runtime. Other gems do not depend on this gem: they ask for text through the header-only [LocalizationContract](https://github.com/Dmytro-Personal-O3DE-Gems/LocalizationContract) gem, which this gem implements.

- One CSV table per language (`en.csv`, `es.csv`, `uk.csv`, …) plus a list of languages for the in-game menu (`languages.csv`).
- English fallback: an untranslated key shows the English text instead of disappearing.
- Runtime language switching, with a notification so UI can refresh its texts.
- The starting language follows the operating system's display language.
- `LOCALIZATION_KEY("door.open")` marks keys in C++ so a tool can find them.
- A checking tool, from the command line or from the Editor menu **Localization → Check Localization**: keys missing in English, untranslated keys, unused keys, broken table rows.

## Contents

- [What is inside](#what-is-inside)
- [Requirements](#requirements)
- [Installation](#installation)
- [String tables](#string-tables)
- [How a text is found](#how-a-text-is-found)
- [Starting language](#starting-language)
- [Using it from C++](#using-it-from-c)
- [Console commands](#console-commands)
- [Checking the localization](#checking-the-localization)
- [Tests](#tests)
- [Status and roadmap](#status-and-roadmap)
- [License](#license)

## What is inside

```
LocalizationGem/
├── Code/
│   ├── Include/LocalizationGem/
│   │   └── LocalizationGemBus.h                  # the gem's own API: SetLanguage, GetAvailableLanguages, LanguageInfo
│   ├── Source/
│   │   ├── Clients/LocalizationGemSystemComponent.*  # loads the tables, answers LocalizationContract, switches languages
│   │   ├── StringTableCsvParser.*                # CSV text -> key/text table + list of problems (no files, no engine)
│   │   ├── SystemLanguages.*                     # asks the OS for its languages; picks the starting language
│   │   └── Tools/                                # Editor module (template default)
│   ├── Platform/
│   │   ├── Windows/, Linux/                      # SystemLanguages_<platform>.cpp: how each OS reports its languages
│   │   ├── Mac/, iOS/, Android/                  # use Common/Unimplemented for now (English)
│   │   └── Common/Unimplemented/                 # fallback for platforms that cannot ask the OS yet
│   └── Tests/                                    # unit tests: CSV parser, starting language
├── Editor/Scripts/
│   ├── bootstrap.py                              # run by the Editor at startup
│   └── localization_gem_editor.py                # the "Localization" menu
├── Tools/
│   └── check_localization.py                     # the checking tool (command line and Editor menu)
├── Registry/assetprocessor_settings.setreg       # Asset Processor scan folders of the gem
└── gem.json
```

Your game provides the texts: they live in **your project**, not in this gem (see [String tables](#string-tables)).

## Requirements

| | |
|---|---|
| Engine | O3DE **26.05** (developed and tested against tag `2605.0`) |
| Gems | **LocalizationContract** (declared dependency) |
| Optional | **EditorPythonBindings** — for the *Localization* menu in the Editor |
| Fonts | a font that has the letters of every language you ship. LyShine's default font has no Cyrillic, for example; Noto Sans covers Latin and Cyrillic |
| Checking tool | Python **3.10** or newer, standard library only. The engine's own Python works |

## Installation

1. Clone this repository and [LocalizationContract](https://github.com/Dmytro-Personal-O3DE-Gems/LocalizationContract) somewhere outside the engine, for example `D:\O3DE\UserGems\`.
2. Register both gems with the engine:
   ```
   o3de register --gem-path D:\O3DE\UserGems\LocalizationContract
   o3de register --gem-path D:\O3DE\UserGems\LocalizationGem
   ```
3. Enable **Localization Gem** in your project: Project Manager → *Configure Gems*, or add `"LocalizationGem"` to `gem_names` in `project.json`. LocalizationContract comes with it as a dependency.
4. Add the string tables to your project (next section).
5. Rebuild the project **with the Editor closed** (otherwise the linker cannot overwrite the gem's DLL).

To use it from your own C++ code, link the API target you need and add the gem to the `dependencies` of your `gem.json`:

| You want to | Link | `gem.json` dependency |
|---|---|---|
| show translated text (any gem, any UI) | `Gem::LocalizationContract.API` | `"LocalizationContract"` |
| switch the language, build a language menu | `Gem::LocalizationGem.API` | `"LocalizationGem"` |

```cmake
BUILD_DEPENDENCIES
    PRIVATE   # PUBLIC if one of your headers derives from a bus handler of the gem
        Gem::LocalizationContract.API
```

Most gems only need the contract. Then they keep working with any other implementation of it, or with none at all.

## String tables

Put the tables into your project's **`Assets/Strings/`** folder:

```
<YourProject>/Assets/Strings/
├── languages.csv     # the languages the player can choose
├── en.csv            # English: the fallback, required
├── es.csv
└── uk.csv
```

Asset Processor copies them to the cache unchanged; the gem reads `@products@/assets/strings/<code>.csv`.

**A language table** — one file per language, file name = language code:

```csv
key,text,comment
door.open,Open,Press label on a closed door
door.close,Close,Press label on an open door
item.shader_ball.name,Shader Ball,Name of the test object
```

```csv
key,text,comment
door.open,Відкрити,
door.close,Закрити,
item.shader_ball.name,Тестова куля,
```

- `key` and `text` are required, `comment` is a note for translators. Columns are found by name (case-insensitive), so their order does not matter, and other columns are ignored.
- Keys: short and symbolic, `area.thing` — `door.open`, `item.shader_ball.name`. Not the English text.
- `en.csv` is the reference: every key the game uses must be there.

**The list of languages** — `languages.csv`, the same format: `key` = language code (= the table's file name), `text` = the language's name **in that language**, so a player can find their language even when the current one is unreadable to them:

```csv
key,text,comment
en,English,Fallback language: must always be listed
es,Español,
uk,Українська,
```

The menu (`GetAvailableLanguages`) contains only languages whose table file exists: English first, the others sorted by code. The order of rows in the file does not matter.

**What the loader accepts** — what Excel and LibreOffice produce:

- **UTF-8**, with or without BOM. Save as *CSV UTF-8* in Excel or with character set *Unicode (UTF-8)* in LibreOffice. A file in another encoding is rejected as a whole and reported in the log.
- `,` or `;` as the separator, detected from the header row (Excel uses `;` in many regional settings).
- Quoted fields with separators, line breaks and doubled quotes (`""`) inside; CRLF or LF; empty rows.
- A bad row is **skipped** and reported in the log with its file and line: empty key, missing or empty text, duplicate key (the first one wins). The rest of the file still loads.
- No `key` or `text` column → nothing is loaded. An unclosed quote → the rows before it load, the rest is dropped.

> The tables are read **once**, when the gem starts. A table added or edited while the Editor is running is seen after an Editor restart. Problems found at startup are printed before the Editor console exists: look in DebugView.

## How a text is found

A consumer (for example, the interaction prompt of [InteractionSystemUI](https://github.com/Dmytro-Personal-O3DE-Gems/InteractionSystemUI)) stores a **key** and a **default text** and asks the contract for the key:

1. the table of the current language;
2. not there → the English table (a warning is logged once per key);
3. not there either → "no text" (a warning is logged once per key), and the consumer shows its own default text.

Switching the language resets the "already warned" list.

## Starting language

When the gem starts, it asks the operating system for the player's display languages, most preferred first, and takes the first one that is in the menu: an exact match first (`pt-br`), then the language part of the tag (`es-ES` → `es`). If nothing matches, the game starts in English. With Windows set to Catalan, then Spanish, and no Catalan table, the game starts in Spanish. The choice is logged:

```
System languages: [es-ES, en-US]. Starting language: 'es'.
```

| Platform | Source | Status |
|---|---|---|
| Windows | display languages (`GetUserPreferredUILanguages`) | tested |
| Linux | `LANGUAGE`, then `LC_ALL` / `LC_MESSAGES` / `LANG` | builds, not run on Linux yet |
| Mac, iOS, Android | not implemented yet | always English |

The gem does **not** remember the player's choice. That is the job of the game's settings code: save the code the player picked, and after loading the settings call `SetLanguage` with it (see below).

## Using it from C++

### Showing a text (any gem, through LocalizationContract)

```cpp
#include <LocalizationContract/LocalizationContractBus.h>

AZStd::string GetDisplayText(const AZStd::string& key, const AZStd::string& defaultText)
{
    // nullptr when no localization gem is active: then the default text is all there is.
    // Do not keep the pointer: ask again each time.
    if (auto* localization = LocalizationContract::LocalizationContractInterface::Get())
    {
        if (AZStd::optional<AZStd::string> text = localization->GetLocalizedString(key))
        {
            return *text;
        }
    }
    return defaultText;
}
```

The same calls are available as `LocalizationContractRequestBus` (initialize the result to "not found" before `BroadcastResult`). `GetCurrentLanguage()` returns the current code, for example `"uk"`.

### Writing keys in code: `LOCALIZATION_KEY`

```cpp
#include <LocalizationContract/LocalizationKey.h>

label.m_key = LOCALIZATION_KEY("door.open");
label.m_defaultText = "Open";
```

For the compiler it is just the string `"door.open"`. The checking tool finds keys in source code by this marker. Only a string literal compiles inside it, so a key the tool could not read (a variable, a key built at runtime) is rejected at compile time. Write every key in C++ through it.

### Reacting to a language change

UI that sets its text once (a menu, a static label) should listen to `OnLocalizedStringsChanged` and request its texts again. It is sent after every successful `SetLanguage`. UI that asks for its text every frame (like the interaction prompt) does not need it.

```cpp
#include <LocalizationContract/LocalizationContractNotificationBus.h>

// A sketch: AZ_COMPONENT, Reflect() and the rest of the component are left out.
class MyMenuComponent
    : public AZ::Component
    , public LocalizationContract::LocalizationContractNotificationBus::Handler
{
    void Activate() override { LocalizationContract::LocalizationContractNotificationBus::Handler::BusConnect(); }
    void Deactivate() override { LocalizationContract::LocalizationContractNotificationBus::Handler::BusDisconnect(); }

    void OnLocalizedStringsChanged() override
    {
        // Request every text this menu shows again.
    }
};
```

### Switching the language and building a language menu

```cpp
#include <LocalizationGem/LocalizationGemBus.h>

if (auto* localization = LocalizationGem::LocalizationGemInterface::Get())
{
    for (const LocalizationGem::LanguageInfo& language : localization->GetAvailableLanguages())
    {
        // language.m_code: "uk"; language.m_displayName: "Українська" (show it as is)
    }

    localization->SetLanguage("uk");
}
```

`SetLanguage`:
- does nothing for the current language;
- rejects codes that are not in the menu (warning);
- if the table cannot be loaded, keeps the previous language (warning);
- on success, swaps the texts and sends `OnLocalizedStringsChanged`.

The same calls are on `LocalizationGemRequestBus`. This API is C++ only for now (not reflected to Script Canvas or Lua).

## Console commands

For testing, in the Editor console or the game's console:

| Command | Does |
|---|---|
| `LocalizationGemSystemComponent.SetLanguageCommand uk` | `SetLanguage("uk")` |
| `LocalizationGemSystemComponent.ListLanguagesCommand` | prints the language menu and marks the current language |

Type the class name in front: just `SetLanguageCommand` is an unknown command.

## Checking the localization

`Tools/check_localization.py` compares what the project uses with what the tables contain.

**From the Editor:** **Localization → Check Localization** in the menu bar (needs EditorPythonBindings). The report goes to the Console. The script is reloaded on every click, so edits to it apply without restarting the Editor.

**From the command line:**

```
<engine>\python\python.cmd <gem>\Tools\check_localization.py <project folder> [--strict]
```

The engine's `python.cmd` needs `cmake` in `PATH`. If cmake only comes with Visual Studio, run the command from *Developer PowerShell for VS 2022*. Any other Python 3.10+ works too: `python check_localization.py <project folder>`.

Exit code: `0` — no errors (with `--strict`: no warnings either), `1` — otherwise, `2` — wrong arguments.

What it reads: `Assets/Strings/*.csv` (by the same rules as the gem), C++ files (`.cpp`, `.h`, `.hpp`, `.inl`) for `LOCALIZATION_KEY("…")` outside comments, and prefabs (`.prefab`) for key/default-text pairs, including overrides of nested prefabs. It skips `Cache`, `build`, `user`, `_Gems` and hidden folders.

| Error | Warning |
|---|---|
| a key is used but missing in `en.csv` | a key in `en.csv` is not used anywhere |
| a broken row in a table | keys not translated in another language |
| `languages.csv`: missing, no `en`, a listed language without its table | a key in another language that is not in `en.csv` (a typo?) |
| `LOCALIZATION_KEY` the tool cannot read; an empty key | a language table that is not listed in `languages.csv` |
| a prefab that is not valid JSON | a default text without a key: it is never translated |

Example:

```
Localization check: D:\O3DE\Projects\TestProject
Languages: en, es, uk | keys in en.csv: 5 | keys used: 4

WARNING  Assets\Strings\en.csv:4: key 'door.lock' is not used in the project's prefabs or code
WARNING  Assets\Strings\es.csv: 1 key(s) not translated, shown in English: door.force

0 error(s), 2 warning(s)
```

Limits: it looks only inside the project folder (keys used only in a gem's own code are reported as unused), and it does not understand `#if 0`, raw string literals or `.uicanvas` files.

## Tests

`LocalizationGem.Tests` (GoogleTest, run with AzTestRunner):

- the CSV parser: encodings, separators, quotes, bad rows, headers;
- `PickStartLanguage`: exact codes, regions, case, the player's order, fallback (filter `*PickStartLanguage*`).

## Status and roadmap

Tested in the Editor (O3DE 26.05, Windows): loading the tables with the English fallback, Cyrillic text with a suitable font, switching languages at runtime, the language menu, the starting language from Windows, the checking tool from the command line and from the Editor menu. Used by [InteractionSystemUI](https://github.com/Dmytro-Personal-O3DE-Gems/InteractionSystemUI) for the interaction prompt.

Planned or not yet verified:
- **Packaged game**: reading the tables from `.pak` files is not tested yet.
- **`OnLocalizedStringsChanged`**: sent, but no listener has been tested yet (waits for an in-game settings menu).
- **Placeholders and plurals** (`"Open {door}"`, plural forms).
- **System language on Mac, iOS and Android** (English for now) and a test run on Linux.
- **Script Canvas / Lua** access to switching languages.
- **Checking tool**: extra folders, so keys in gems' own code are found.

## License

[MIT](LICENSE) © 2026 Dmytro Shyshkin
