# Toaster Plugin Options

Add a plugin measure to your skin, then send it the `Show` command from a
meter action or a measure action such as `OnChangeAction` or `IfTrueAction`.

```ini
[MeasureToast]
Measure=Plugin
Plugin=Toaster
Title=Timer finished
Text=Your tea is ready.
DynamicVariables=1

[MeterSend]
Meter=String
Text=Send notification
LeftMouseUpAction=[!CommandMeasure MeasureToast "Show"]
```

Put the options below in the plugin measure. Toaster reads them when it
receives a command. Use Rainmeter's `DynamicVariables=1` when the options
reference values that change while the skin is running.

## Measure options

Options default to blank unless a default is shown. For switches, use `1` to
enable them or `0` to disable them. Image and WAV paths can use `#@#` to
refer to your skin's resources.

| Category | Option | What it does |
| --- | --- | --- |
| Text | `Title` | First line of text, typically the notification heading. |
| Text | `Text` | Main message. |
| Text | `Text2` | Additional message text, if Windows has room to display it. |
| Text | `Attribution` | Small attribution line for a source or other credit. |
| Images | `Icon` | Image inside the toast. The small sender icon in the header is always the Rainmeter icon. |
| Images | `IconCircle` | Crop the content icon to a circle. Default: `0`. |
| Images | `IconAlt` | Alternative text for `Icon`. |
| Images | `HeroImage` | Large image at the top of the notification content. |
| Images | `HeroImageAlt` | Alternative text for `HeroImage`. |
| Images | `Image` | Inline image in the notification body. |
| Images | `ImageAlt` | Alternative text for `Image`. |
| Actions | `LaunchURI` | URI Windows opens when the notification body is clicked. |
| Actions | `Button1`–`Button5` | Labels for up to five buttons or menu entries. Omit the label to omit that action. |
| Actions | `ButtonNURI` | URI to open for button `N`. Takes precedence over `ButtonNAction`. |
| Actions | `ButtonNAction` | Windows action for button `N`: `Snooze` or `Dismiss`. Default: `Dismiss`. |
| Actions | `ButtonNIcon` | Image to display on button `N`. |
| Actions | `ButtonNContextMenu` | Put button `N` in the notification's menu instead of its body. Default: `0`. |
| Sound | `Silent` | Mute the generated toast's Windows sound and disable `SoundFile` playback. Default: `0`. |
| Sound | `Audio` | Choose a Windows sound URI, such as `ms-winsoundevent:Notification.Mail`. Windows uses its default sound if this is omitted and the toast isn't muted. |
| Sound | `AudioLoop` | Ask Windows to loop the sound selected in `Audio` for a supported scenario. Default: `0`. |
| Sound | `SoundFile` | Local WAV to play once after a successful `Show`. It replaces the Windows sound and is disabled by `Silent=1`, `SuppressPopup=1`, or `ToastXML`. Because Rainmeter plays the WAV, it can still play when Windows Do Not Disturb hides the banner. |
| Display | `Duration` | Request a `short` or `long` banner. Windows controls how long it stays visible. |
| Display | `Scenario` | Request Windows behavior for `default`, `reminder`, `alarm`, `incomingCall`, or `urgent`. Support depends on Windows. |
| Display | `Timestamp` | Display time in ISO 8601 format, such as `2026-11-12T15:00:00Z`. |
| Delivery | `ExpiresAfter` | Seconds before the notification expires, up to 31,536,000 (one year). Default: `0`, which leaves expiration to Windows. |
| Delivery | `Tag` | ID used to replace, update, or remove a notification. Use distinct IDs across unrelated skins. |
| Delivery | `Group` | Optional second part of the notification ID. Use the same tag and group to replace or target the same item. |
| Delivery | `SuppressPopup` | Deliver to Notification Center without a banner. Requires `Tag`. Default: `0`. |
| Progress | `Progress` | Add a progress bar. Default: `0`. |
| Progress | `ProgressTitle` | Label above the progress bar. |
| Progress | `ProgressValue` | Number or Rainmeter formula from `0` to `1` (for example, `0.5` for 50%), or `indeterminate` when the amount is unknown. Default: `0`. |
| Progress | `ProgressValueText` | Text to show in place of the default percentage, such as `3 of 10 files`. |
| Progress | `ProgressStatus` | Status text for the progress bar. Default: `In progress`. |
| Repeat limits | `Cooldown` | Seconds to suppress an identical repeat of the last successful `Show` on this measure. Default: `5`. Use `0` to disable. |
| Repeat limits | `MinimumInterval` | Minimum seconds between successful `Show` commands on this measure, even if the content changes. Default: `0`. Suppressed requests are discarded. |
| Advanced | `ToastXML` | Replaces the generated payload with single-line [Windows Toast XML](https://learn.microsoft.com/en-us/uwp/schemas/tiles/toastschema/schema-root) with a `<toast>` root. Tag, group, expiration, popup suppression, repeat limits, and progress data still apply. Put visual, sound, and action settings in the XML itself. |

For button options, replace `N` with the number in the label option. For
example, pair `Button2=Open notes` with `Button2URI=https://example.com/`.
Toaster supports URI links and Windows-managed actions. Text replies and
callbacks into Rainmeter require a Windows activation server.

## Commands

Use `[!CommandMeasure MeasureToast "Show"]` with the name of your measure and
one of the commands below.

| Command | What it does |
| --- | --- |
| `Show` | Submit a notification. Windows controls its appearance and visibility. |
| `UpdateProgress` | Apply the current progress values to an existing toast. Set `Progress=1` before the initial `Show` and use the same `Tag` and `Group` for both commands. |
| `Remove` | Clear the notification with the specified `Tag` and optional `Group`. |

The measure value is `0` before any command, `1` after success, `2` when
`Show` is suppressed by a repeat limit, and `-1` on error. For errors, check
Rainmeter's **About → Log** tab.

## Windows sender registration

Toaster registers **Rainmeter Notifications** for the current Windows user
when it sends a toast. All skins using Toaster share that sender and its
Rainmeter icon. If a banner doesn't appear, check the sender's settings
under **Settings → System → Notifications** and look in Notification Center.

To remove the registration, exit Rainmeter and delete the
`HKEY_CURRENT_USER\Software\Classes\AppUserModelId\Rainmeter.PluginToaster`
key in Registry Editor. To remove the cached sender icon, delete
`%LOCALAPPDATA%\Rainmeter\Toaster\Rainmeter.png`. Toaster recreates both
the next time it sends a notification.
