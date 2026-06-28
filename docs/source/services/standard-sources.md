Scripts, Local Files and Post-Processing
========================================
```{warning}
These standard-plugin features are intended for advanced users. A normal feed subscription only needs the `URL` source type.
```

The standard plugin can receive feed data from a network URL, a local file or a command. It can then optionally pass that data through a second command before parsing it.

This is useful when a website does not publish a normal feed, when another application already generates one, or when feed data needs a small conversion step.

## Source Types
### URL
RSS Guard downloads the address and treats the response as feed data. This is the normal choice for RSS, Atom, JSON Feed and similar sources.

### Local File
RSS Guard reads feed data from a file on your computer. Another application or scheduled task can update that file independently.

### Script
RSS Guard runs a command and reads its standard output as feed data.

The command should:

* write valid feed data to standard output
* write diagnostics to standard error
* exit with code `0` when successful

Commands run in the RSS Guard [user-data folder](../features/userdata). The `%data%` [placeholder](../features/userdata.md#data-placeholder) expands to that folder.

## Post-Processing
The optional post-processing command receives the original source data through standard input and must write the final feed data to standard output.

For example, the source can be an API response while the post-processing command converts its JSON into JSON Feed.

```{mermaid}
flowchart LR
  source["URL, local file or script"]
  post{"Post-processing configured?"}
  command["Run post-processing command"]
  parser["Parse resulting feed"]

  source-->post
  post-->|Yes|command
  post-->|No|parser
  command-->parser
```

## Command Examples
Quoting rules depend on your operating system and command interpreter. Paths and arguments containing spaces normally need quotes.

```text
bash "%data%/scripts/download-feed.sh"

python "%data%/scripts/build-feed.py"

%data%\jq.exe "{ version: \"1.1\", title: \"Stars\", items: . }"
```

If **Fetch metadata** fails, check that:

* the executable exists
* quoting is correct
* standard output contains only valid feed data
* diagnostics are written to standard error
* the command exits successfully

## Related Feed Options
Scripted and transformed feeds can also use:

* custom HTTP headers and authentication for URL sources
* a custom proxy
* full-article extraction
* comment fetching
* a published-versus-updated date preference

The standalone [article extractor](../features/extractor) can be used from custom scripts when you need readable article HTML or plain text.

## Examples and Tools
RSS Guard includes [example scraper scripts](https://github.com/martinrotter/rssguard/tree/master/resources/scripts/scrapers). Inspect a script before using it so you understand its inputs and output.

Useful third-party projects include:

* [CSS2RSS](https://github.com/Owyn/CSS2RSS)
* [RSSGuardHelper](https://github.com/pipiscrew/RSSGuardHelper)

Scripts and selectors can break when a website changes. If an existing setup suddenly fails, check the source website or API before changing RSS Guard settings.
