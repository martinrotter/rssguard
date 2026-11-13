Google Calendar
===============

The Google Calendar plugin lists individual events in RSS Guard, provides a native event editor, and shows configurable local reminders. It works independently of Gmail and uses a separate Google authorization.

## Setup

1. Enable the Google Calendar API in your Google Cloud project and create OAuth credentials for a **Desktop app**. Configure the consent screen and, if applicable, add your Google account as a test user.
2. Add a **Google Calendar** account in RSS Guard. Enter an account name and the desktop client's ID and secret. Builds with preconfigured Calendar credentials also accept empty credential fields.
3. Select **Test setup**, finish authorization in your external browser, and wait for the successful Calendar access check. Save the account to keep the tested authorization. You can also authorize a saved account using **Authorize Google Calendar** in its menu.
4. Choose **Refresh calendar list** to discover subscribed calendars. Refresh individual calendar items using the ordinary feed-update commands.

RSS Guard uses a local loopback redirect with an automatically allocated port and PKCE. It requests permission to read your calendar list and read/write events. Calendar creation, sharing and access-control management remain in Google Calendar. The account's proxy settings apply to API requests; the external browser uses its own network settings.

The setup test uses the entered credentials and proxy, reads the calendar list and checks event-reading access. It does not create or edit events. Its authorization stays in the dialog until you save; cancelling or a failed test preserves the live account. Changing credentials or proxy invalidates the test result.

Cached calendars and events remain readable offline. A missing or revoked grant is reported through account status; startup does not automatically open a sign-in browser. Credentials and tokens are stored in the existing RSS Guard database.

**Refresh calendar list on startup** is off by default. Existing accounts use their cached calendar list unless this option is enabled or you choose **Refresh calendar list** manually. Accounts without any cached calendars discover them automatically. Event fetching retains the existing global, per-calendar and startup feed-update settings; the reminder interval does not schedule network updates.

Background requests wait for pending authorization before contacting the Calendar API. An expired token is refreshed first. If the API rejects a token with HTTP 401, the plugin refreshes authorization and retries that request once; concurrent requests share the refresh. Failed authorization preserves cached data and event drafts. Ambiguous write failures are not automatically retried.

If a calendar-list download finishes during an active update, the plugin keeps the downloaded result and applies it when the update finishes. This prevents calendar removal from invalidating feeds being fetched.

## Event list and cache

Each calendar appears as a feed, and each occurrence of a recurring event appears as an article. **Starts** uses the built-in publication-date column; **Calendar** uses the feed-title column. Optional **Ends**, **Location** and **All day** columns use the normal column menu, including visibility, order, width and resize modes. These extra columns cannot be used for sorting. Built-in columns can still sort while hidden, and Calendar layouts are remembered separately from other providers.

Calendar items use colored icons generated from their Google Calendar colors and stored for offline use. Refreshing the calendar list updates these icons when colors change. Calendars without a valid color use the account icon.

The default cache covers the previous **30 days** and the next **365 days**. Change these bounds in account settings and refresh to reconcile the cache. Google expands recurring events within that window. Events leaving the window are removed locally; a newly saved event outside the window remains available briefly so it can be inspected.

The existing article storage format requires occurrence starts on or after 1 January 1970 UTC. Older occurrences must be managed in Google Calendar; they are rejected before remote creation/editing. A recurring series can start earlier because only its expanded occurrences enter the cache. If an older occurrence appears in a refresh, the refresh reports this limit and preserves the previous cache.

Ordinary article-retention and database-cleanup rules do not remove Calendar events. The plugin manages its own rolling cache. New calendars start with ordinary feed notifications and inclusion in global unread counts disabled; their feed settings can be changed normally.

Read, important and deleted states belong to RSS Guard. Synchronization preserves them. Deleting an event locally, emptying the recycle bin or removing the account **does not cancel or delete the event in Google Calendar**. Permanent local deletion suppresses that cached occurrence while its tombstone is retained. Cancelled remote occurrences move to the local recycle bin; later reinstatement preserves any earlier local deletion.

## Creating and editing

Choose **New event** from the account, calendar or preview. Choose **Edit event** to change the selected event. The editor supports titles, locations, descriptions, timed/all-day ranges, time zones and basic daily, weekly, monthly or yearly recurrence. All-day end dates shown in the editor are inclusive.

For a recurring occurrence, select **This occurrence** or **Whole series**. Existing complex recurrence is preserved unless explicitly replaced with a supported basic rule. Times in a daylight-saving gap are rejected; ambiguous times require selecting the intended occurrence. Calendar permissions, locked events and special Google event types can make events read-only.

Unexposed fields, including participants and meeting data, remain untouched. Edited descriptions are sent as escaped plain text; an unchanged description retains its original Google HTML. The editor provides an explicit attendee-notification choice for events with participants.

If Google reports that an event changed elsewhere, RSS Guard keeps the draft. Reload the current event or reapply the changed fields to the latest version, review, and save again. A failed or uncertain creation retains its remote ID for retries. If Google confirms a save but the local cache update fails, RSS Guard reports that distinction; refresh the calendar to recover the cache.

The preview shows cached event details, participants and safe meeting links. **Open in Google Calendar** provides access to unsupported operations such as attendee management, advanced recurrence or remote deletion.

## Local reminders

Reminders are disabled until enabled in the account's **Reminders** tab. The default check interval is **30 minutes**, configurable in minutes. Checks read cached events from the database independently of selection, article filters and network connectivity. Refresh the cache regularly to discover new events or remote changes.

Set local offsets in minutes before the start (`0` means at start), or use Google's popup-reminder offsets. Google's email reminders are ignored. Local rules can repeat until the event ends. Empty offsets disable the repeat control without clearing its value; `0` allows repetition. Account settings retain that repeat value after saving and reopening an empty offset list. Disabling account reminders preserves the controls' values and leaves help available. All-day local rules use the configured time in the calendar's time zone.

To customize a calendar, select **Edit** and open its **Reminders** tab. Choose **Inherit account rules**, **Disable for this calendar**, **Use local offsets**, or **Use Google's popup offsets**. Check interval, all-day time and notification templates still come from the account. Saving only common feed settings preserves the calendar's reminder configuration, including different repeat intervals on existing rules. You can configure calendar rules while account reminders are off, but notifications require the account master switch to be enabled.

When editing several calendars, **Apply reminder settings to all selected calendars** is unchecked initially. Leave it unchecked to preserve each calendar's rules while changing common feed settings. If their reminder configurations differ, the tab shows **Multiple values**. Check the apply selector and choose a mode to replace the selected calendars' reminder configuration; choosing **Inherit account rules** explicitly restores inheritance. Cancelling changes nothing. Common settings and reminder overrides are saved together for the whole selection; a failed save preserves the previous database and live settings and keeps the draft available for retry.

The future cache window must cover local offsets and at least 28 days when using Google offsets, so uncached events with Google's maximum offset can be fetched in time.

Notification title and body support `{title}`, `{start}`, `{end}`, `{calendar}`, `{location}` and `{url}`. Due rules for the same event are grouped into one notification with an **Open** action.

RSS Guard must be running. Delivery can be up to one check interval late; after a delay or restart, catch-up covers one interval. Enabling reminders does not replay older triggers. Reading or starring an event does not dismiss its reminders. Cancelled, locally deleted and disabled-calendar events are excluded.

Delivery history survives restart. If saving that history fails, the running process suppresses repeated delivery and retries the checkpoint. A crash between showing a notification and saving its history can still cause that notification to repeat after restart. There is no background OS alarm, snooze or remote reminder synchronization.
