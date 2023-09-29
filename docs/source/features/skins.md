Skins
=====
RSS Guard is a skinable application. With [Qt stylesheets](https://doc.qt.io/qt-5/stylesheet.html), the GUI can be changed almost entirely.

<img alt="alt-img" src="images/gui-dark.png" width="600px">

```{warning}
Note that as of RSS Guard `4.1.3`, old skins `vergilius` and `dark` were removed and replaced with `nudus-*` skins. For now, only `nudus` skins are maintained by RSS Guard developers.  
```

```{note}
The skin `API` (see below) is very extensible and allows tweaking the visual part of RSS Guard in many ways without much work.
```

You can select style and skin in `Settings -> User interface` dialog section.

Try to play around with various combinations of styles and skins to achieve the UI you like.

Creating a custom UI is possible with `skins`. Each skin should be placed in its own root folder and must contain specific files. The [built-in skins](https://github.com/martinrotter/rssguard/tree/master/resources/skins) are stored in folder together with RSS Guard executable, but you can place your own custom skins in a `skins` subfolder in [user data](userdata) folder. Create the folder manually, if it does not exist.

<img alt="alt-img" src="images/about-skins.png" width="600px">

For example, if your new skin is called `greenland`, the folder path should be as follows:

```
<user-data-path>\skins\greenland
```

As stated above, there are specific files that each skin folder must contain:
* `metadata.xml` - XML file with basic information about the skin's name, author etc.
* `qt_style.qss` - [Qt stylesheet](https://doc.qt.io/qt-5/stylesheet.html) file
* `html_*.html`  - HTML files which are dynamically put together to create a complete HTML pages for various things, like newspaper view, article viewer, or error page

Note that not all skins have to provide a full-blown theming for every UI component of RSS Guard. Skin can provide just a custom HTML/CSS setup for article viewer and a minimal Qt CSS styling for UI controls.

Skins usually define custom palette of colors which is yet another mechanism to change look of RSS Guard. This skin subfeature is enabled with `Use skin colors` checkbox on `Settings -> User interface` dialog section.

Also, user can define custom CSS styles independently in file `<user-data-folder>/web/user-styles.css`. The file would be then loaded by RSS Guard and its styles applied to internal web browser.