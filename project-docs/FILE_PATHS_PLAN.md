I recommend using **`Seamly` as the suite-level directory**, with separate subdirectories for each application and a `Shared` directory when genuinely needed.

## Application data

Application data includes settings, configuration, caches, logs, downloaded resources, recovery files, and other files managed internally by the software.

### Windows

```text
%APPDATA%\Seamly\
├── Seamly2D\
├── SeamlyMe\
├── SeamlyLayout\
└── Shared\
```

Use `%APPDATA%` for configuration that should follow the user if Windows roaming profiles are enabled:

```text
C:\Users\<username>\AppData\Roaming\Seamly\
```

Use `%LOCALAPPDATA%` for caches, logs, temporary data, recovery data, and machine-specific state:

```text
C:\Users\<username>\AppData\Local\Seamly\
├── Seamly2D\
│   ├── Cache\
│   ├── Logs\
│   └── Recovery\
├── SeamlyMe\
├── SeamlyLayout\
└── Shared\
```

Recommended Windows mapping:

| Data                 | Directory                                         |
| -------------------- | ------------------------------------------------- |
| Configuration        | `%APPDATA%\Seamly\<application>\`               |
| Cache                | `%LOCALAPPDATA%\Seamly\<application>\Cache\`    |
| Logs                 | `%LOCALAPPDATA%\Seamly\<application>\Logs\`     |
| Recovery/autosave    | `%LOCALAPPDATA%\Seamly\<application>\Recovery\` |
| Shared internal data | `%LOCALAPPDATA%\Seamly\Shared\`                 |

### Linux

Follow the XDG Base Directory specification.

```text
$XDG_CONFIG_HOME/Seamly/
```

Normally:

```text
/home/<username>/.config/Seamly/
```

Persistent application data:

```text
$XDG_DATA_HOME/Seamly/
```

Normally:

```text
/home/<username>/.local/share/Seamly/
```

Cache:

```text
$XDG_CACHE_HOME/Seamly/
```

Normally:

```text
/home/<username>/.cache/Seamly/
```

Recommended Linux mapping:

| Data                     | Directory                                         |
| ------------------------ | ------------------------------------------------- |
| Configuration            | `~/.config/Seamly/<application>/`               |
| Persistent internal data | `~/.local/share/Seamly/<application>/`          |
| Cache                    | `~/.cache/Seamly/<application>/`                |
| Logs                     | `~/.local/state/Seamly/<application>/Logs/`     |
| Recovery/autosave        | `~/.local/state/Seamly/<application>/Recovery/` |
| Shared internal data     | `~/.local/share/Seamly/Shared/`                 |

The application must honor the XDG environment variables when users override these defaults.

### macOS

Persistent application data:

```text
/Users/<username>/Library/Application Support/Seamly/
├── Seamly2D/
├── SeamlyMe/
├── SeamlyLayout/
└── Shared/
```

Caches:

```text
/Users/<username>/Library/Caches/Seamly/
├── Seamly2D/
├── SeamlyMe/
└── SeamlyLayout/
```

Logs:

```text
/Users/<username>/Library/Logs/Seamly/
```

Preferences may be stored as standard macOS property-list files:

```text
/Users/<username>/Library/Preferences/org.seamly.Seamly2D.plist
/Users/<username>/Library/Preferences/org.seamly.SeamlyMe.plist
/Users/<username>/Library/Preferences/org.seamly.SeamlyLayout.plist
```

Recommended macOS mapping:

| Data                 | Directory                                                        |
| -------------------- | ---------------------------------------------------------------- |
| Configuration        | `~/Library/Application Support/Seamly/<application>/`          |
| Native preferences   | `~/Library/Preferences/org.seamly.<application>.plist`         |
| Cache                | `~/Library/Caches/Seamly/<application>/`                       |
| Logs                 | `~/Library/Logs/Seamly/<application>/`                         |
| Recovery/autosave    | `~/Library/Application Support/Seamly/<application>/Recovery/` |
| Shared internal data | `~/Library/Application Support/Seamly/Shared/`                 |

## User data

User data means files the user creates, opens, saves, manages, backs up, or transfers: patterns, measurement files, layouts, exports, templates, and projects.

### Windows

Default root:

```text
C:\Users\<username>\Documents\Seamly\
```

Using the Windows known-folder API:

```text
%USERPROFILE%\Documents\Seamly\
```

Suggested structure:

```text
Documents\Seamly\
├── Projects\
├── Patterns\
├── Measurements\
├── Layouts\
├── Templates\
└── Exports\
```

### Linux

Default root:

```text
/home/<username>/Documents/Seamly/
```

Conceptually:

```text
$XDG_DOCUMENTS_DIR/Seamly/
```

Suggested structure:

```text
Documents/Seamly/
├── Projects/
├── Patterns/
├── Measurements/
├── Layouts/
├── Templates/
└── Exports/
```

The application should resolve `XDG_DOCUMENTS_DIR` rather than assuming the folder is literally named `Documents`, because localized Linux systems may use a different name.

### macOS

Default root:

```text
/Users/<username>/Documents/Seamly/
```

Suggested structure:

```text
Documents/Seamly/
├── Projects/
├── Patterns/
├── Measurements/
├── Layouts/
├── Templates/
└── Exports/
```

## Consolidated recommendation

| Platform | Configuration                                                           | Cache                          | User documents                      |
| -------- | ----------------------------------------------------------------------- | ------------------------------ | ----------------------------------- |
| Windows  | `%APPDATA%\Seamly\`                                                   | `%LOCALAPPDATA%\Seamly\`     | `%USERPROFILE%\Documents\Seamly\` |
| Linux    | `$XDG_CONFIG_HOME/Seamly/`              | `$XDG_CACHE_HOME/Seamly/` | `$XDG_DOCUMENTS_DIR/Seamly/` |                                     |
| macOS    | `~/Library/Application Support/Seamly/`                               | `~/Library/Caches/Seamly/`   | `~/Documents/Seamly/`             |

The important boundary is: **users should see and manage `Documents/Seamly`; internal configuration and caches should remain in the operating system’s application-data locations.**

For the transition, the new build should detect the legacy `seamly2d` directory and migrate or adopt it automatically. It should not simply rename the folder, because users may need to roll back to an earlier release. A copy-and-verify migration, followed by leaving the legacy directory intact or clearly marking it as migrated, would be safer.
