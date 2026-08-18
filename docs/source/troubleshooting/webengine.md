Qt WebEngine Graphics Problems
==============================

The `web` variant of RSS Guard uses Qt WebEngine to display articles and web pages. On some systems, incompatibilities between Qt WebEngine and the graphics driver can cause visual corruption, a frozen window, or a crash when a particular article is opened.

This does not apply to the `text` variant, which does not use Qt WebEngine.

## Before Changing Anything

First update Qt WebEngine, Mesa and your graphics driver to the latest versions available for your operating system. Restart the system after updating the graphics stack.

Errors mentioning `GBM`, `NativePixmap`, `gbm_bo_import`, `GLX` or failure to import a graphics buffer usually indicate a problem in the Qt WebEngine/graphics-driver path rather than malformed feed data.

## Disable the GBM Path

On Linux, try starting RSS Guard with Qt WebEngine's GBM path disabled:

```bash
QTWEBENGINE_FORCE_USE_GBM=0 rssguard
```

This is the preferred first workaround for GBM-related crashes because it does not disable GPU acceleration completely.

```{important}
`QTWEBENGINE_FORCE_USE_GBM=0` is an environment variable. Do not paste it into the WebEngine **Flags** field in RSS Guard settings.
```

If this resolves the problem, add the environment variable to the application launcher or your desktop environment according to the instructions for your Linux distribution.

## Disable GPU Acceleration

If disabling GBM does not help, disable GPU acceleration for Qt WebEngine:

1. Open `Settings -> Network & web`.
2. Select `Web -> WebEngine`.
3. Enter `--disable-gpu` into **Flags**.
4. Save the settings and restart RSS Guard.

You can also test the same workaround from a terminal:

```bash
QTWEBENGINE_CHROMIUM_FLAGS="--disable-gpu" rssguard
```

Disabling GPU acceleration is a broader workaround. It can reduce rendering performance, so use it only when the narrower GBM workaround is ineffective or does not apply.

## Other Options

If neither workaround is suitable, use the `text` build of RSS Guard or open affected articles in an external browser. The `text` viewer has fewer browser features, but it does not depend on Qt WebEngine's graphics path.

If you report the crash, include the RSS Guard debug log, operating-system version, Qt WebEngine version, graphics-driver information and the URL of an article that reproduces the problem. See [Reporting Bugs or Feature Requests](../contrib/bugs.md) for logging and crash-report instructions.
