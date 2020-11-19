# Labels
RSS Guard supports labels/tags. Arbitrary number of tags can be assigned to each and every messages.

Also, note that tags are synchronizeable for account plugins which support it, currently Inoreader and TT-RSS. While labels are synchronized with these services, they sometimes cannot directly created via RSS Guard and you have to create them via web interface of the respective service and then perform `Sync in` which will download created labels too.

You can easily add labels via `Labels` root item.

<img src="images/label-menu.png" width="80%">

You can choose title and color of your new label.

<img src="images/label-dialog.png" width="200px">

You can easily (de)assign label to messages in message viewer.

<img src="images/label-assign.png" width="80%">

Note that (de)assignments of labels to messages are synchronized back to supported servers in regular intervals.

Also, [message filters](Message-filters.md) can assign or remove labels to/from messages.