Command Line Interface (CLI)
============================
RSS Guard offers CLI. For overview of its features, run `rssguard --help` in your terminal. You will see the overview of the interface.

```
Usage: rssguard [options] [url-1 ... url-n]
RSS Guard

Options:
  -h, --help                     Displays overview of CLI.
  -v, --version                  Displays version of the application.
  -l, --log <log-file>           Write application debug log to file. Note that
                                 logging to file may slow application down.
  -d, --data <user-data-folder>  Use custom folder for user data and disable
                                 single instance application mode.
  -s, --no-single-instance       Allow running of multiple application
                                 instances.
  -g, --no-debug-output          Disable just "debug" output.
  -n, --no-standard-output       Completely disable stdout/stderr outputs.
  -w, --no-web-engine            Force usage of simpler text-based embedded web
                                 browser.
  -t, --style <style-name>       Force some application style.
  -p, --adblock-port <port>      Use custom port for AdBlock server. It is
                                 highly recommended to use values higher than
                                 1024.

Arguments:
  urls                           List of URL addresses pointing to individual
                                 online feeds which should be added.
```

You can add feeds to RSS Guard by passing URLs as the command line parameters too. Feed [URI scheme](https://en.wikipedia.org/wiki/Feed_URI_scheme) is supported, so that you can call RSS Guard like this:

```powershell
rssguard.exe "feed://archlinux.org/feeds/news"
rssguard.exe "feed:https//archlinux.org/feeds/news"
rssguard.exe "https://archlinux.org/feeds/news"
```

In order to easily add the feed directly from your web browser of choice, without copying and pasting the URL manually, you have to open RSS Guard with feed URL passed as an argument. There are [browser extensions](https://addons.mozilla.org/en-US/firefox/addon/open-with/) which will allow you to do it.