Reporting Bugs or Feature Requests
==================================

Please report bugs and feature requests on the [Issues page](https://github.com/martinrotter/rssguard/issues).

For general questions, ideas, or discussion, use the [Discussions page](https://github.com/martinrotter/rssguard/discussions).

## What to Include in a Bug Report

Please include as much of this information as possible:

* What happened.
* What you expected to happen instead.
* Steps needed to reproduce the problem.
* Your RSS Guard version.
* Your operating system.
* Whether the problem happens every time or only sometimes.
* The application debug log. This is needed for almost every bug report.
* A crash dump file, but only when RSS Guard crashes.

## Application Debug Log

If you report a bug, please almost always attach the RSS Guard application debug log.

The debug log is useful for crashes, broken feed downloads, account problems, network errors, GUI glitches, startup problems, and many other issues. If you are not sure whether a log is needed, attach it.

```{warning}
The application log can contain private information, such as feed URLs, article titles, account names, server addresses, and network error details.
Check the log before attaching it publicly if you use private feeds or private services.
```

### Create a Log File

The best log for bug reports is usually a file created from the command line.

1. Open `Settings -> General` and make sure `Disable CLI debug output (but keep more serious warnings enabled)` is not checked.
2. Close RSS Guard.
3. Start RSS Guard from a terminal with the `--log` option.
4. Reproduce the problem.
5. Close RSS Guard, so the log file is fully written.
6. Attach the generated log file to your issue.

The setting above can intentionally hide debug and informational messages from command-line output and log files.
Warnings and errors are still logged, but the resulting file may miss details needed for harder bugs.
If you cannot change the setting, or if you want to be sure that debug messages are included for one run, start RSS Guard with both `--debug` and `--log`.

If RSS Guard is available in your `PATH`, this is enough:

```bash
rssguard --log "rssguard.log"
```

If you need a more detailed log, add `--debug`:

```bash
rssguard --debug --log "rssguard.log"
```

On Windows, you can run RSS Guard from its installation folder or use the full path. Adjust the path if you installed RSS Guard somewhere else:

```batch
"%ProgramFiles%\RSS Guard\rssguard.exe" --log "%USERPROFILE%\Desktop\rssguard.log"
```

For a portable Windows package, open Command Prompt in the unpacked RSS Guard folder and run:

```batch
rssguard.exe --log "%USERPROFILE%\Desktop\rssguard.log"
```

On Linux, run:

```bash
rssguard --log "$HOME/rssguard.log"
```

On macOS, if RSS Guard is installed as an application bundle, run the executable inside the bundle:

```bash
/Applications/RSSGuard.app/Contents/MacOS/rssguard --log "$HOME/Desktop/rssguard.log"
```

You can also pass an empty file name. In that case, RSS Guard stores the log file in its user data folder:

```bash
rssguard --log ""
```

If RSS Guard does not start normally, start it with logging enabled first, then reproduce the startup problem as far as possible and attach the created log.

```{attention}
The application log can also be displayed directly in RSS Guard via the `Help > Display application log` menu item.
Log messages are written to that window only while it is open, although the window itself can be minimized.
This window can negatively affect RSS Guard performance, so the log file from command-line logging is usually better for bug reports.
```

## Crash Dump on Windows

A crash dump is different from the debug log. It is needed when RSS Guard crashes, closes unexpectedly, or disappears without showing a normal error message.

If RSS Guard does not crash, you usually do not need a dump file. In that case, the debug log is normally enough.

If RSS Guard does crash, a crash dump file can help find the exact place where the crash happened.

On Windows, you can create such a file with Microsoft's `ProcDump` utility. `ProcDump` is part of Microsoft Sysinternals.

```{warning}
Crash dump files can contain private information, such as URLs, feed contents, article titles, account names, or authentication-related data.
Do not attach a dump file publicly if you are not comfortable sharing that information.
If needed, ask first how to provide the file privately.
```

### Download ProcDump

1. Open the official Microsoft page: [ProcDump - Sysinternals](https://learn.microsoft.com/en-us/sysinternals/downloads/procdump).
2. Click **Download ProcDump**.
3. Save the ZIP file, for example to your `Downloads` folder.
4. Open the ZIP file in File Explorer.
5. Click **Extract all**.
6. Extract it to an easy location, for example:

   ```text
   C:\Users\YOUR_USER_NAME\Downloads\Procdump
   ```

After extraction, you should see files such as `procdump.exe` and `procdump64.exe`.

### Prepare a Folder for Dump Files

1. Go to your Desktop.
2. Create a new folder named:

   ```text
   rssguard-dumps
   ```

This folder will receive the crash dump file.

### Start ProcDump

1. Close RSS Guard if it is already running.
2. Open the folder where you extracted ProcDump.
3. Click the address bar in File Explorer.
4. Type:

   ```text
   cmd
   ```

5. Press `Enter`.

A Command Prompt window should open directly in the ProcDump folder.

Now paste this command into the Command Prompt window and press `Enter`:

```batch
procdump64.exe -accepteula -e -mm -n 1 -w rssguard.exe "%USERPROFILE%\Desktop\rssguard-dumps"
```

If Windows says that `procdump64.exe` cannot be found, use this command instead:

```batch
procdump.exe -accepteula -e -mm -n 1 -w rssguard.exe "%USERPROFILE%\Desktop\rssguard-dumps"
```

The Command Prompt window should now wait for RSS Guard to start.

The command means:

* `-accepteula` accepts the Sysinternals license prompt.
* `-e` creates a dump when RSS Guard crashes with an unhandled error.
* `-mm` creates a small mini dump file.
* `-n 1` creates one dump file and then stops.
* `-w rssguard.exe` waits until RSS Guard is started.

### Reproduce the Crash

1. Start RSS Guard normally.
2. Use RSS Guard until the crash happens.
3. When RSS Guard crashes, ProcDump should write a `.dmp` file to:

   ```text
   Desktop\rssguard-dumps
   ```

4. Wait until ProcDump finishes writing the file.
5. Compress the `.dmp` file into a ZIP file.
6. Attach the ZIP file to your bug report, or ask how to provide it privately.

### If No Dump File Is Created

If RSS Guard crashes but no dump file appears:

* Make sure ProcDump was still running before you started RSS Guard.
* Make sure the command contains `-w rssguard.exe`.
* Make sure the dump folder exists on your Desktop.
* Try again, but start Command Prompt as administrator.

To start Command Prompt as administrator:

1. Open the Windows Start menu.
2. Type:

   ```text
   cmd
   ```

3. Right-click **Command Prompt**.
4. Select **Run as administrator**.
5. Go to the ProcDump folder manually, for example:

   ```batch
   cd /d "%USERPROFILE%\Downloads\Procdump"
   ```

6. Run the ProcDump command again.
