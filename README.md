# Toaster 1.1 for Rainmeter

[Download the Rainmeter skin package](https://github.com/jsmorley/PluginToaster/releases/latest).
The installer opens **ToasterTest**. Click its input field, type a message, and
press Enter to see a toast and hear the test sound. See the [plugin options guide](Skins/ToasterPlugin/@Resources/HowToUse.md).

![Illustrated preview of a ToasterTest notification](docs/ToasterTest.webp)

Toaster.dll posts native Windows notifications. Clone this repository into
`C++\PluginToaster` in the [Rainmeter Plugin SDK](https://github.com/rainmeter/rainmeter-plugin-sdk).
Build `PluginToaster.vcxproj` for Win32 or x64 and install the matching DLL
in Rainmeter's Plugins folder. The first `Show` registers
**Rainmeter Notifications** in the current user's registry. The sender always uses the icon from the [Rainmeter project](https://github.com/rainmeter/rainmeter/blob/master/Application/Rainmeter.ico).

```ini
[MeasureToast]
Measure=Plugin
Plugin=Toaster
Title=Timer finished
Text=Your tea is ready.
Button1=Open timer notes
Button1URI=https://example.com/
DynamicVariables=1

[MeterTest]
Meter=String
Text=Send test notification
LeftMouseUpAction=[!CommandMeasure MeasureToast "Show"]
```

Call `Show` from a click, `OnChangeAction`, or an `IfTrueAction`. The plugin
reads the measure's current options when the command runs, including Rainmeter
variables and measure references. Nothing is posted on an ordinary update.

## Measure options

| Options | Behavior |
| --- | --- |
| `Title`, `Text`, `Text2`, `Attribution` | Main text and optional attribution. Windows determines how much fits. |
| `Icon`, `IconCircle=1`, `HeroImage`, `Image` | Content icon, optional circular crop, large hero image, and inline image. Use local paths, including `#@#` paths. Set `IconAlt`, `HeroImageAlt`, or `ImageAlt` for alternative text. |
| `LaunchURI` | Opens a URI when the toast body is selected. |
| `Button1` through `Button5` | Button labels. For each button, set `Button1URI` to open a URI or `Button1Action=Snooze` or `Dismiss` for a Windows action. A button defaults to Dismiss. Use `Button1Icon` for an image or `Button1ContextMenu=1` to place it in the toast menu. Use the corresponding number for each button. |
| `Silent=1`, `Audio`, `AudioLoop=1` | Silence the toast, select a Windows sound event such as `ms-winsoundevent:Notification.Mail`, or request looping for a supported scenario. |
| `SoundFile` | Local WAV to play once after the toast is submitted. The plugin mutes the native toast sound when it plays the WAV. `Silent=1`, `SuppressPopup=1`, and `ToastXML` disable this playback. Windows Do Not Disturb can hide the banner while Rainmeter still plays the WAV. |
| `Duration`, `Scenario`, `Timestamp` | Duration is `short` or `long`. Scenario is `default`, `reminder`, `alarm`, `incomingCall`, or `urgent`. Timestamp is ISO 8601, such as `2026-11-12T15:00:00Z`. Windows controls scenario availability and presentation. |
| `ExpiresAfter` | Seconds until Windows expires the notification. The default of 0 leaves expiration to Windows. |
| `Tag`, `Group` | Identify the notification within the shared sender. Use distinct identifiers for unrelated skins. Posting with the same tag and group replaces the previous item. |
| `SuppressPopup=1` | Send to Notification Center without a banner. Requires a tag. |
| `Progress=1` | Show a progress bar with `ProgressTitle`, `ProgressValue`, `ProgressValueText`, and `ProgressStatus`. The value is 0–1 or `indeterminate`. |
| `Cooldown`, `MinimumInterval` | Cooldown suppresses repeats of the last successful, identical `Show` on that measure for five seconds by default. Set it to 0 to disable it. MinimumInterval limits any `Show` from that measure for the given number of seconds, even when the message changes. It defaults to 0. Suppressed requests are discarded. |
| `ToastXML` | Replaces the generated payload with a single-line Windows Toast XML payload. Tag, group, expiration, popup, cooldown, and progress data still apply. This gives access to supported XML features such as headers, adaptive layouts, and a Windows-managed snooze picker. |

`UpdateProgress` updates a tagged toast in place from the current progress
options, and `Remove` removes a tagged toast. Both use the measure's current
`Tag` and `Group`. For custom XML, use the bindings `{progressTitle}`,
`{progressValue}`, `{progressValueText}`, and `{progressStatus}` with
`Progress=1` to initialize progress when showing the toast.

The measure's numeric result is 0 before the first command, 1 after Windows
accepts it, 2 when a repeat is suppressed, or -1 on error. Details go to the
Rainmeter log. A successful `Show` submits the toast to Windows, which decides
whether and where to display it.

This DLL uses protocol links and Windows-managed actions. An application COM
activation server would be needed to return text replies or foreground button
callbacks to Rainmeter. Use a registered URI handler for actions that need to
open another application. Standard Windows notification settings apply.

See the [Windows Toast XML schema](https://learn.microsoft.com/en-us/uwp/schemas/tiles/toastschema/schema-root).

### Remove the Windows sender registration

Exit Rainmeter. In Registry Editor, go to
`HKEY_CURRENT_USER\Software\Classes\AppUserModelId\Rainmeter.PluginToaster`
and delete the `Rainmeter.PluginToaster` key. It stores `DisplayName`,
`CustomActivator`, and `IconUri` for the shared sender. The next notification
from Toaster recreates the key.

To remove the cached Rainmeter icon as well, delete
`%LOCALAPPDATA%\Rainmeter\Toaster\Rainmeter.png`. Toaster recreates it the next
time Toaster sends a notification.

## Windows 10 testing

The current build has been verified on Windows 11. Windows 10 22H2 is
expected to work, and feedback from Windows 10 testers is welcome.

1. Install `ToasterPlugin-1.1.rmskin`. The installer loads **ToasterTest**.
2. Click the input field, type a message, and press **Enter**.
3. Check for a toast banner and listen for the original email sound.
   Check Notification Center (**Win+N** on Windows 11) or Action Center (**Win+A** on Windows 10) even if no banner appears.

If the test fails, report the Windows version and OS build from `winver`,
whether the banner appeared, whether the toast reached Action Center,
and whether the WAV played. Include any `MeasureToast` error from
Rainmeter's **About → Log** tab.
