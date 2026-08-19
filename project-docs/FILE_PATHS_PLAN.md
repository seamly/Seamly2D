I recommend using **`Seamly` as the suite-level directory**, with separate subdirectories for each application and a `Shared` directory when genuinely needed.

## Application data

- Notes:
  - Application data includes settings, configuration, caches, logs, downloaded resources, recovery files, and other files managed internally by the software. Internal configuration and caches should remain in the operating system’s application-data locations.
  - User Data means files the user creates, opens, saves, manages, backs up, or transfers: patterns, measurement files, layouts, exports, templates, and projects. Users should see and manage `Documents/SeamlyData` or similar.

For the transition, the new build should detect the legacy `seamly2d` directory and migrate or adopt it automatically. It should not simply rename the folder, because users may need to roll back to an earlier release. A copy-and-verify migration, followed by leaving the legacy directory intact or clearly marking it as migrated, would be safer.

### Windows

- Configuration files:

  ```text
  %APPDATA%\seamly\
  ├── seamly2D\
  ├── seamlyMe\
  ├── seamlyLayout\
  └── shared\
  ```

  - Use `%APPDATA%` for configuration that should follow the user if Windows roaming profiles are enabled: `C:\Users\<username>\AppData\Roaming\seamly\`

  - Use `%LOCALAPPDATA%` for caches, logs, temporary data, recovery data, and machine-specific state:

  ```text
  C:\Users\<username>\AppData\Local\seamly\
  ├── seamly2d\
  │   ├── cache\
  │   ├── logs\
  │   └── recovery\
  ├── seamlyme\
  │   ├── cache\
  │    ├── logs\
  │   └── recovery\
  ├── seamlylayout\
  │   ├── cache\
  │   ├── logs\
  │   └── recovery\
  └── shared\
  ```

- Recommended **Windows** mapping:
  
  | Data                 | Directory                                       |
  | -------------------- | ----------------------------------------------- |
  | Configuration        | `%APPDATA%\seamly\<application>\`               |
  | Cache                | `%LOCALAPPDATA%\seamly\<application>\cache\`    |
  | Logs                 | `%LOCALAPPDATA%\seamly\<application>\logs\`     |
  | Recovery/autosave    | `%LOCALAPPDATA%\seamly\<application>\recovery\` |
  | Shared internal data | `%LOCALAPPDATA%\seamly\shared\`                 |
  | User data            | `%HOME\Documents\SeamlyData\                                 |

### Linux

- Follow the XDG Base Directory specification. The application must honor the XDG environment variables when users override these defaults.

  - `$XDG_CONFIG_HOME` Default path: `~/.config/`; Purpose: Stores user-specific configuration files, settings, and preferences. Top level folder for configuration data for seamly would be `$XDG_CONFIG_HOME/seamly/`, e.g. `~/.config/seamly/`

  - `$XDG_DATA_HOME` Default path: `~/.local/share/`; Purpose: Stores user-specific data files like save files, media databases, or local app states. Top level folder for persistent internal data for seamly would be `$XDG_DATA_HOME/seamly/`, e.g. `~/.local/share/seamly/`

  - `$XDG_CACHE_HOME` Default path: `~/.cache/` Purpose: Stores non-essential data like web browser caches or thumbnail previews. Top level tree For cache data for seamly would be  `$XDG_CACHE_HOME/seamly`, e.g. `~/.cache/seamly/`

  - `$XDG_LOGS_HOME` Default path: `~/.local/state/`.The top level folder for logs for seamly would be `$XDG_LOGS_HOME/seamly`, e.g. `~/.local/state/seamly/`

- Recommended **Debian Linux** mapping:

| Data                 | Directory                                        |
| -------------------- | ------------------------------------------------ |
| Configuration        | `~/.config/seamly/<application>/`                |
| Cache                | `~/.cache/seamly/<application>/`                 |
| Logs                 | `~/.local/state//seamly/<application>/logs`      |
| Recovery/autosave    | `~/.local/state//seamly/<application>/recovery/` |
| Shared internal data | `~/.local/share/seamly/`                         |
| User data            | `~/Documents/SeamlyData/`                        |

### macOS

- Follow the Apple Standard Directory specification. The application must honor the Apple Standard Directory environment variables when users override these defaults.
- Preference Files (.plist): Instead of raw text files (like .conf or .yaml), macOS applications store configuration settings inside ~/Library/Preferences/ as binary or XML files called property lists (.plist).


| Data                 | Directory                                                      |
| -------------------- | -------------------------------------------------------------- |
| Configuration        | `~/Library/Application/seamly/<application>/`                  |
| Native preferences   | `~/Library/Preferences/org.seamly.<application>.plist`         |
| Cache                | `~/Library/Caches/seamly/<application>/`                       |
| Logs                 | `~/Library/Logs/seamly/<application>/`                         |
| Recovery/autosave    | `~/Library/Application/seamly/<application>/recovery/`         |
| Shared internal data | `~/Library/Application/seamly/shared/`                         |
| User data            | `~/Documents/SeamlyData/`                                      |

Persistent application data:

```text
~/Library/Application/seamly/
├── seamly2d/
├── seamlyme/
├── seamlylayout/
└── shared/
```

Caches:

```text
~/Library/Caches/seamly/
├── seamly2d/
├── seamlyme/
└── seamlylayout/
```

Logs:

```text
~/Library/Logs/seamly/
```

Preferences may be stored as standard macOS property-list (.plist) files:

```text
~/Library/Preferences/org.seamly.Seamly2D.plist
~/Library/Preferences/org.seamly.SeamlyMe.plist
~/Library/Preferences/org.seamly.SeamlyLayout.plist
```
