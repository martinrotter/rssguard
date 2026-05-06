Command Line Interface (CLI)
============================
RSS Guard offers a CLI. For an overview of its features, run `rssguard --help` in your terminal.

```
Usage: rssguard [options] [url-1 ... url-n]
RSS Guard

Options:
  -h, --help                     Displays overview of CLI.
  -v, --version                  Displays version of the application.
  -d, --data <user-data-folder>  Use custom folder for user data and disable
                                 single instance application mode.
  -s, --no-single-instance       Allow running of multiple application
                                 instances.
  -g, --debug                    Enable "debug" CLI output.
  -l, --log <log-file-name>      Log application standard/error output to file.
                                 When empty string is provided as argument, then
                                 the log file will be stored in user data
                                 folder.
  -t, --style <style-name>       Force some application style.
  -u, --user-agent <user-agent>  User custom User-Agent HTTP header for all
                                 network requests. This option takes precedence
                                 over User-Agent set via application settings.
  --threads <count>              Specify number of threads. Note that number
                                 cannot be higher than 32.

Arguments:
  urls                           List of URL addresses pointing to individual
                                 online feeds which should be added.
```

You can also add feeds to RSS Guard by passing URLs as command-line parameters. The feed [URI scheme](https://en.wikipedia.org/wiki/Feed_URI_scheme) is supported, so you can call RSS Guard like this:

```powershell
rssguard.exe "feed://archlinux.org/feeds/news"
rssguard.exe "feed:https//archlinux.org/feeds/news"
rssguard.exe "https://archlinux.org/feeds/news"
```

To add a feed directly from your web browser of choice without copying and pasting the URL manually, you have to open RSS Guard with the feed URL passed as an argument. There are [browser extensions](https://addons.mozilla.org/en-US/firefox/addon/open-with/) that allow you to do this.

## Logging
You can log all important events to a text file which is then useful when [reporting](contrib/bugs) bugs. Here is how to do that:

1. Make sure that debugging logging is not disabled in `Settings -> General`.
2. Launch RSS Guard from command line with `--log "log.txt"` parameter.