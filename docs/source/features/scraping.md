Scraping Websites
=================
```{warning}
Only proceed if you consider yourself a power user, and you know what you are doing!
```

RSS Guard offers additional advanced feature inspired by [Liferea](https://lzone.de/liferea/).

Goal of this feature is to allow advanced users to use RSS Guard with data sources which do not provide regular feed. So you can use the feature to generate one.

You can select source type of each feed. If you select URL, then RSS Guard simply downloads feed file from given location and behaves like everyone would expect.

However, if you choose `Script` option, then you cannot provide URL of your feed, and you rely on custom script to generate feed file and provide its contents to [**standard output** (stdout)](https://en.wikipedia.org/wiki/Standard_streams#Standard_output_(stdout)). Data written to standard output should be valid feed file, for example RSS or ATOM XML file.

`Fetch it now` button also works with `Script` option. Therefore, if your source script and (optional) post-process script in cooperation deliver a valid feed file to the output, then all important metadata, like title or icon of the feed, can be discovered :sparkles: automagically :sparkles:.

<img alt="alt-img" src="images/scrape-source-type.png" width="350px">

Any errors in your script must be written to [**error output** (stderr)](https://en.wikipedia.org/wiki/Standard_streams#Standard_error_(stderr)).

```{warning}
As of RSS Guard 4.2.0, you cannot separate your arguments with `#`. If your argument contains spaces, then enclose it with DOUBLE quotes, for example `"my argument"`. DO NOT use SINGLE quotes to do that.
```

Format of post-process script execution line can be seen on picture below.

<img alt="alt-img" src="images/scrape-post.png" width="350px">

If everything goes well, script must return `0` as the process exit code, or a non-zero exit code if some error happened.

Executable file must be always be specified, while arguments do not. Be very careful when quoting arguments. Tested examples of valid execution lines are:

| Command       | Explanation   |
| :---          | ---           |
| `bash -c "curl 'https://github.com/martinrotter.atom'"`   | Download ATOM feed file using Bash and Curl. |
| `Powershell Invoke-WebRequest 'https://github.com/martinrotter.atom' \| Select-Object -ExpandProperty Content` | Download ATOM feed file with Powershell. |
| `php tweeper.php -v 0 https://twitter.com/NSACareers`     | Scrape Twitter RSS feed file with [Tweeper](https://git.ao2.it/tweeper.git). Tweeper is the utility that produces RSS feed from Twitter and other similar social platforms. |

Note that the above examples are cross-platform. You can use exactly the same command on Windows, Linux or macOS, if your operating system is properly configured.

RSS Guard offers placeholder `%data%` which is automatically replaced with full path to RSS Guard user data folder, allowing you to make your configuration fully portable. You can, therefore, use something like this as a source script line: `bash %data%/scripts/download-feed.sh`.

Also, working directory of process executing the script is set to point to RSS Guard user's data folder.

There are [examples of website scrapers](https://github.com/martinrotter/rssguard/tree/master/resources/scripts/scrapers). Most of them are written in Python 3, so their execution line is similar to `python script.py`. Make sure to examine each script for more information on how to use it.

After your source feed data is downloaded either via URL or custom script, you can optionally post-process it with one more custom script, which will take **raw source data as input**. It must produce valid feed data to [**standard output** (stdout)] while printing all error messages to [**error output** (stderr)].

Here is little flowchart explaining where and when scripts are used:

```{mermaid}
flowchart TB
  src{{"What kind of source was used?"}}
  url["Download the (feed) data from given URL"]
  scr["Generate the (feed) data with given script"]
  pstd{{"Is any post-process script set?"}}
  pst["Take previously obtained data and feed it to post-process script"]
  fin["Handover resulting feed data to RSS Guard for more processing - saving to DB etc."]

  src-->|URL|url
  src-->|Script|scr
  url-->pstd
  scr-->pstd
  pstd-->|Yes|pst
  pstd-->|No|fin
  pst-->fin
```

Typical post-processing filter might do things like CSS formatting, localization of content to another language, downloading of complete articles, some kind of filtering, or removing ads.

It's completely up to you if you decide to only use script as `Source` of the script or separate your custom functionality between `Source` script and `Post-process` script. Sometimes you might need different `Source` scripts for different online sources and the same `Post-process` script and vice versa.

Third-party tools for scraping made to work with RSS Guard:
* [CSS2RSS](https://github.com/Owyn/CSS2RSS) - can be used to scrape websites with CSS selectors.
* [RSSGuardHelper](https://github.com/pipiscrew/RSSGuardHelper) - another CSS selectors helper.

Make sure to give credit to authors that they deserve.