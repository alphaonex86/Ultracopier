# Plugins

Plugins can be used to customize most parts of the software.

The following plugin types are available:
- Copy engine
- Listener
- Plugin loader
- Session loader
- Themes

They must implement the corresponding interface contained in the`interface` folder.

Plugin files
- `informations.xml`
- `MyPlugin.pro`, `MyPlugin.h`, `MyPlugin.cpp`
- `Environment.h` -> `Variable.h`, `StructEnumDefinition.h`, `DebugEngineMacro.h`
- `plugin.json`
- `documentation.dox`
- `Languages/XX/translation.ts`
- `resources/` -> `MyPlugin.qrc`

## Installation

Distribute the following files:
- `informations.xml`
- `libMyPlugin.so`
- `Languages/XX/translation.qm`
