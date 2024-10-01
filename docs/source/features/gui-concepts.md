General GUI Concepts & Manipulating Accounts & Adding Feeds
===========================================================
* Feed list displays all your feeds and other items such as recycle bin.
* Article list displays all your articles depending on what is selected in feed list.
* Article preview displays details of selected article or information about selected item from feed list if no article is selected.
* Titlebar of RSS Guard display number of unread articles in square brackets.
* There are two toolbars available, separate toolbar for feed list and for article list.

----
When you start RSS Guard for the very first time, you are greeted with `Add account` dialog where you select which account you want to operate with. If you want to have classic `RSS/ATOM` feed reader, then select `RSS/RDF/ATOM/JSON` option.

<img alt="alt-img" src="images/accounts.png">

Each "account" offers account-specific actions which are accessible in relevant submenu.

<img alt="alt-img" src="images/account-menu.png">

To add new feed into the account you simply use `Feeds -> Add item -> Add a new feed` menu item. You do not need to know direct URL address of your feed file, it is enough to enter just the URL of the website you are interested in, for example `https://www.hltv.org`, and RSS Guard will find all available feed sources.

<img alt="alt-img" src="images/discover-feeds.png">

You can also switch to more advanced dialog for adding individual feeds.

<img alt="alt-img" src="images/feed-details.png">

In 99% of cases, you only need to insert feed URL into `Source` field and then hit `Fetch it now` button which will download feed metadata and fill all other boxes.

```{note}
Some feeds are only accessible when user is logged-in the website which provides the feed.

RSS Guard has two features to aid with login-protected feeds:
1. Network-based authentication (rarely used) -> user can setup credentials on `Network` tab in feed details dialog.
2. Website-based authentication (often used) -> user has to login to the website via RSS Guard's embedded [web browser](browseradblock):
    1. In RSS Guard, open new web browser tab.
    2. Navigate to feed's website and login.
    3. RSS Guard will now remember login cookies and will automatically use them to authenticate when feed is fetching.
```