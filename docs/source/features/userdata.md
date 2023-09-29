User Data
=========
```{note}
One of the main goals of RSS Guard is to have portable/relocatable user data folder so that it can be used across all supported operating systems.
```

RSS Guard can run in two modes:
* **Non-portable**: The default mode, where user data folder is placed in user-wide "config directory" (`C:\Users\<user>\AppData\Local` on Windows).  
  If the file `C:\Users\<user>\AppData\Local\RSS Guard 4\data\config\config.ini` exists, then this folder is used.  
* **Portable mode**: This mode allows storing user data folder in a subfolder **data4** in the same directory as RSS Guard binary (`rssguard.exe` on Windows). This mode is used automatically if non-portable mode detection fails.

Check `Help -> About application -> Resources` dialog tab to find more info on paths used.

User data folder can store your custom icon themes in `icons` subfolder, and custom skins in `skins` subfolder.

## `%data%` placeholder
RSS Guard stores its data and settings in a single folder. How to find out the exact path, see here. RSS Guard allows using the folder programmatically in some special contexts via `%data%` placeholder. You can use this placeholder in following contexts:
* Contents of your [article filters](filters) - you can, therefore, place some scripts under your user data folder and include them via JavaScript into your article filter.
* Contents of each file included in your custom [skins](skins). Note that in this case, the semantics of `%data%` are little changed and `%data%` points directly to base folder of your skin.
* `source` and `post-process script` attributes for feed [scraping](scraping) - you can use the placeholder to load scripts to generate/process the feed from user's data folder.
* [Notifications](notifications) also support the placeholder in path to audio files which are to be played when some event happens. For example, you could place audio files in your data folder and then use them in a notification with `%data%\audio\new-messages.wav`. See more about notifications.