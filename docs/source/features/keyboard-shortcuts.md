Keyboard Shortcuts
==================

Configure commands in Settings > Keyboard shortcuts. Changes remain staged until you choose Apply or save the settings dialog. Cancel discards edits that have not been applied. The undo button restores the binding present when the settings page was loaded, and the clear button removes a binding.

## Default Shortcuts

Saved assignments override these defaults, including empty assignments for shortcuts you cleared. Loading shortcuts does not rewrite saved settings. An action without a saved assignment uses its current default; an existing profile with only some assignments saved therefore keeps those assignments and uses defaults for the remaining actions.

The following table uses Windows/Linux key names. Commands run only while enabled and eligible in the main window. F5 fetches selected feed-list items and Shift+F5 fetches all, including when an embedded browser has focus. Ctrl+F remains the displayed browser's native text search; Ctrl+Shift+F focuses article-list search. The pane-focus and feed/article-search commands activate the Feeds tab before focusing their target, so they can be used from browser and media-player tabs too.

| Action | Shortcut |
| --- | --- |
| Fetch selected | `F5` |
| Fetch all | `Shift+F5` |
| Stop ongoing fetching | `Ctrl+Shift+F5` |
| Previous article | `Ctrl+Up` |
| Next article | `Ctrl+Down` |
| Previous item | `Ctrl+Shift+Up` |
| Next item | `Ctrl+Shift+Down` |
| Next unread article | `Ctrl+Shift+J` |
| Scroll up preview | `PgUp` |
| Scroll down preview | `PgDown` |
| Toggle articles read/unread | `Ctrl+Shift+M` |
| Mark articles read | `Ctrl+Shift+R` |
| Mark articles unread | `Ctrl+Shift+U` |
| Switch importance | `Ctrl+Shift+I` |
| Open in internal browser | `Ctrl+O` |
| Open in external browser | `Ctrl+Shift+O` |
| Play in media player | `Ctrl+Shift+P` |
| Fetch full contents | `Ctrl+Shift+D` |
| Copy article data (no dialog) | `Ctrl+Shift+C` |
| Focus article search box | `Ctrl+Shift+F` |
| Focus feed search box | `Ctrl+Shift+L` |
| Focus feed list | `Ctrl+1` |
| Focus article list | `Ctrl+2` |
| Focus article preview | `Ctrl+3` |
| Add new feed | `Ctrl+N` |
| Add new folder | `Ctrl+Shift+N` |
| Edit selected items | `F2` |
| New web browser tab | `Ctrl+T` |
| Close current tab | `Ctrl+W` |
| Go to next tab | `Ctrl+Tab` |
| Go to previous tab | `Ctrl+Shift+Tab` |
| Settings | `Ctrl+,` |
| Fullscreen | `F11` |
| Quit | `Ctrl+Q` |
| Open main menu | `F10` |

Secondary commands share the Ctrl+K prefix: press Ctrl+K, release it, then press the two indicated keys in order. For example, Ctrl+K, A, G reveals the selected article's feed. Ctrl+K has no standalone action. The categories are A (articles), F (feeds), V (view), T (tabs) and S (show/filter).

| After Ctrl+K | Action |
| --- | --- |
| `A, G` | Reveal feed |
| `A, H` | Open feed homepage |
| `A, E` | Edit feed of selected article |
| `A, C` | Copy article data... |
| `A, L` | Load all remaining articles |
| `A, F` | Article filters |
| `F, R` | Mark selected items read |
| `F, U` | Mark selected items unread |
| `F, C` | Copy URL of selected item |
| `F, E` | Enable/disable selected feeds |
| `F, P` | Pause automatic feed fetching |
| `F, T` | Fetch feeds with custom auto-download policy |
| `F, X` | Expand/collapse selected item recursively |
| `V, P` | Enable article preview |
| `V, F` | Feed list |
| `V, L` | Switch layout |
| `T, A` | Close all tabs |
| `T, O` | Close all tabs except current |
| `S, A` | Article filter: no extra filtering |
| `S, U` | Article filter: unread |
| `S, I` | Article filter: important |

Bulk cleanup, purge, recycle-bin operations, database/cache maintenance, account management, feed sorting/movement, child-feed editing, appearance switches, highlighting and the remaining specialized filters have no default assignment. They can still be configured in Settings > Keyboard shortcuts.

Delete retains the focused list's existing behavior: delete articles in the article list or items in the feed list. Backspace restores articles when the article list has focus. Ordinary arrows, Enter, Tab, Space and letters have no application-wide default assignment, retaining native widget and media-player behavior. Normal tree expansion remains available through the tree's native keys.

## Main Window Priority

Enabled, visible commands with an assigned shortcut take precedence in the main RSS Guard window and its embedded widgets, with an exception for standard operations in a focused editable input. This includes the feed list, article list, read-only article preview and embedded media player.

Editable inputs keep character entry, deletion, cursor movement, selection, clipboard operations, undo/redo and their normal Enter/Tab/Escape behavior. For example, assigning `E` to an application command does not steal `E` from a search or URL field, and assigning Ctrl+A does not prevent Select All. Qt's platform-specific editing bindings and alternate combinations are respected. Unrelated application shortcuts, such as F5, remain available while an input has focus.

This applies to editable QLineEdit, QTextEdit and QPlainTextEdit controls, their subclasses, internal line edits in combo/spin boxes and focused inline editors inside lists or trees. WebEngine text inputs are recognized through their delegated Qt input-method state. Ordinary list/tree/table navigation and read-only views retain application shortcut priority. A sequence starting with a protected editing key does not begin while that key belongs to an editable input.

Outside editable inputs, assigned plain letters, Space, navigation keys and editing combinations take precedence. Without an eligible configured match, the widget's own shortcuts and ordinary key handling remain available. Clearing a binding or disabling its command restores that fallback.

For example, assigning `W` to an application command suppresses the article list's search for items starting with W. Without an eligible assignment, the list's normal letter search and cycling remain available, subject to its existing keyboard settings. Assigning PageDown to preview scrolling scrolls the active tab's preview even while the article list has focus; clearing the assignment restores normal list navigation.

Other windows, dialogs, popup menus, completers and shortcut recorders retain their normal keyboard handling. Recording an existing application shortcut does not execute its command. Tab and Shift+Tab can be recorded; click elsewhere to leave the recorder. Recording normally finishes one second after the last key is released. A shortcut can contain up to four strokes.

## Conflicts

An assignment conflicts with another command when the sequences are identical or one complete sequence is a prefix of the other. For example, `A` conflicts with `A, B`. Two sequences such as `A, B` and `A, C` can coexist.

The replacement prompt lists all conflicting commands and their bindings. Accepting clears those assignments in the settings page. Declining restores the edited command's previous staged binding. Existing conflicts from older or manually edited settings are shown on the shortcuts page; resolve them by changing or clearing the affected bindings.

Qt handles sequence continuation and repeated keys. A swallowed sequence prefix is not replayed into a field if its continuation does not match.

## Preview Scrolling

The scroll-up and scroll-down commands scroll the browser belonging to the **currently active tab**. Separately opened browser tabs each retain their own scroll position. In the feed/article tab, scroll commands are available when its normal article browser is displayed; they are disabled for item details, a hidden preview, and custom service previewers without a supported scroll target.

Each command moves the main document by one live viewport/page step. The web viewer measures the viewport in CSS pixels so resize and zoom are respected; the text viewer uses its current scrollbar page step. Both the `web` and `text` viewers support these commands. The `web` viewer can scroll with page JavaScript disabled without enabling ordinary page scripts.

Commands target the main document. Independently scrolling elements and frames can need their own native navigation. See [Displaying articles and web pages](article-display.md) for viewer differences.

## Keyboard Layouts

Priority matching uses public Qt APIs. It recognizes direct Qt key combinations, keypad variants and Shift+Tab. A native shifted ASCII punctuation key can also match its symbol without Shift when the event already identifies that same symbol. Text alone does not turn a digit into a symbol binding, and synthetic events keep their direct combinations. Ctrl, Alt, Meta and other modifiers are preserved.

Qt's platform-specific shortcut matching can recognize additional keyboard-layout representations. AltGr, dead keys, non-Latin layouts and input methods can therefore behave differently in widgets that reserve keys. OS-reserved combinations are also subject to the operating system. This feature does not reproduce Qt's complete native keyboard-layout normalization.

An exact collision with a local widget shortcut is resolved to the unique eligible configured command when Qt emits an ambiguous shortcut event, subject to the same editable-input protection. Native shorter-prefix collisions are outside this resolution. In a native collision, Qt's chosen registration controls whether repeat events are emitted before the application can resolve the ambiguity.
