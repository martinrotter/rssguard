User Data
=========
```{note}
One of the main goals of RSS Guard is to have a portable and relocatable user-data folder so that it can be used across all supported operating systems.
```

RSS Guard can run in two modes:
* **Non-portable**: The default mode, where the user-data folder is placed in a user-wide config directory (`C:\Users\<user>\AppData\Local` on Windows).
  If the file `C:\Users\<user>\AppData\Local\RSS Guard 5\data\config\config.ini` exists, then this folder is used.
* **Portable mode**: This mode allows storing the user-data folder in a subfolder named **data5** in the same directory as the RSS Guard binary (`rssguard.exe` on Windows). This mode is used automatically if non-portable mode detection fails.

Check the `Help -> About application -> Resources` dialog tab to find more information about the paths in use.

The user-data folder can store your custom icon themes in the `icons` subfolder, and custom skins in the `skins` subfolder.

## `%data%` placeholder
RSS Guard stores its data and settings in a single folder. To find the exact path, see above. RSS Guard allows using this folder programmatically in some special contexts via the `%data%` placeholder. You can use this placeholder in the following contexts:
* Contents of your [article filters](filters) - you can therefore place some scripts under your user-data folder and include them via JavaScript in your article filter.
* Path to your custom configuration directory for the `libmpv`-based [media player](mediaplayer).
* Contents of each file included in your custom [skins](skins). Note that in this case, the semantics of `%data%` are slightly different and `%data%` points directly to the base folder of your skin.
* `source` and `post-process script` attributes for feed [scraping](scraping). You can use the placeholder to load scripts that generate or process the feed from the user's data folder.
* [Notifications](notifications) also support the placeholder in the path to audio files that should be played when some event happens. For example, you could place audio files in your data folder and then use them in a notification with `%data%\audio\new-messages.wav`.
