Supported Feed Readers
======================

RSS Guard is multi-account application and supports many web-based feed readers via built-in plugins. One of the plugins, of course, provides the support for standard list of **RSS/ATOM/JSON** feeds with the set of features everyone would expect from classic feed reader.

I organized the supported web-based feed readers into an elegant table:

+-------------------+-------------------------+---------------------------------------+---------------------+-------+
| Service           | Two-way Synchronization | Intelligent Synchronization Algorithm | Synchronized Labels | OAuth |
+===================+=========================+=======================================+=====================+=======+
| Feedly            | ✅                      | ✅                                    | ✅                  | ✅    |
+-------------------+-------------------------+---------------------------------------+---------------------+-------+
| Gmail             | ✅                      | ✅                                    | ✅                  | ✅    |
+-------------------+-------------------------+---------------------------------------+---------------------+-------+
| Google Reader API | ✅                      | ✅                                    | ✅                  | ✅    |
+-------------------+-------------------------+---------------------------------------+---------------------+-------+
| Nextcloud News    | ✅                      | ❌                                    | ❌                  | ❌    |
+-------------------+-------------------------+---------------------------------------+---------------------+-------+
| Tiny Tiny RSS     | ✅                      | ✅                                    | ✅                  | ❌    |
+-------------------+-------------------------+---------------------------------------+---------------------+-------+

:superscript:`1` Some plugins support next-gen intelligent synchronization algorithm which has some benefits, as it usually offers superior synchronization speed, and transfers much less data over your network connection. RSS Guard only downloads articles which are new or were updated by remote server. The old algorithm usually always fetches all available articles, even if they are not needed, which leads to unnecessary overload of your network connection and the RSS Guard.

:superscript:`2` Note that labels are supported for all plugins, but for some plugins they are local-only, and are not synchronized with the service. Usually because service itself does not support the feature.

:superscript:`3` Tested services are:
 * Bazqux
 * FreshRSS
 * Inoreader
 * Miniflux
 * Reedah
 * TheOldReader