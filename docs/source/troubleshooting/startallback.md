StartAllBack and Taskbar Thumbnail Buttons
=========================================

On Windows, RSS Guard's taskbar thumbnail buttons have a known incompatibility with StartAllBack that can cause RSS Guard to crash. The reporter of [issue #2462](https://github.com/martinrotter/rssguard/issues/2462#issuecomment-6325505692) confirmed that the crashes stopped when thumbnail buttons were disabled while StartAllBack remained installed.

## Workaround

1. Open `Settings -> User interface`.
2. Select the **Tray area & taskbar & window** tab.
3. Under **Taskbar**, uncheck **Display thumbnail buttons**.
4. Save the settings and restart RSS Guard.

If you cannot change the setting in the application, close RSS Guard and edit `config.ini` in its [user-data folder](../features/userdata.md). Set `show_taskbar_thumbnail_buttons=false` in the existing `[gui]` section, adding the key if it is missing, then start RSS Guard again.

If crashes continue with thumbnail buttons disabled, follow the [bug-reporting instructions](../contrib/bugs.md) and include a debug log and crash dump.
