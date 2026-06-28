General GUI Concepts & Manipulating Accounts & Adding Feeds
===========================================================
* The feed list displays all your feeds and other items such as the recycle bin.
* The article list displays all your articles depending on what is selected in the feed list.
* The article preview displays details of the selected article or information about the selected item from the feed list if no article is selected.
* The same embedded viewer is also used for built-in web browsing. Depending on the package you installed, RSS Guard uses either the `web` viewer based on Qt WebEngine or the lighter `text` viewer based on QTextBrowser.
* The title bar of RSS Guard displays the number of unread articles in square brackets.
* There are two toolbars available: a separate toolbar for the feed list and another for the article list.

```{note}
Package names contain `web` or `text` to show which article/web viewer is included. The `web` viewer is more browser-like and feature-complete. The `text` viewer is lighter, but some browser actions and complex page rendering features are not available. See more [here](article-display).
```

----
When you start RSS Guard for the first time, the `Add account` dialog lets you choose a service. You can add more accounts later through `Accounts -> Add account`.

<img alt="alt-img" src="images/accounts.png">

Each "account" offers account-specific actions that are accessible in the relevant submenu.

<img alt="alt-img" src="images/account-menu.png">

Available actions and setup fields depend on the selected service. See [Supported Feed Readers](../supported-readers) for the available account types. To keep subscriptions directly in RSS Guard, start with [Standard RSS, Atom, JSON and other feeds](../services/standard).
