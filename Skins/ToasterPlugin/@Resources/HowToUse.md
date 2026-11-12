# How to use ToasterTest and GMailNotify 1.1

GMailNotify checks the Gmail Atom inbox feed and shows the unread count in
Rainmeter. When the feed reports a newer message, the skin uses Toaster.dll
to send a native Windows notification. The Gmail icon opens the selected
account's inbox.

## 1. Install and send a test

1. Open `ToasterPlugin-1.1.rmskin` with Rainmeter Skin Installer. The package
   includes both skin variants in the `ToasterPlugin` folder and both
   architectures of `Toaster.dll`. Rainmeter 4.5 or later is required.
2. The installer loads `ToasterPlugin\ToasterTest.ini` by default. If you turned
   off loading in the installer, open that skin in Rainmeter Manage.
3. Click **Click to type a message**, type your text, and press **Enter**.
   Windows receives a toast with the message and toast icon, and Rainmeter
   plays the original Gmail WAV. Press **Esc** to cancel editing.

ToasterTest works without a Gmail account. For unread mail alerts, load
`ToasterPlugin\GMailNotify.ini` from Rainmeter Manage. Rainmeter loads one
variant at a time. Both variants use the same `@Resources` folder. Native toast
delivery was tested on Windows 11.

## 2. Register and allow Windows notifications

The first send from ToasterTest registers **Rainmeter Notifications** for the current Windows user with the Rainmeter icon. GMailNotify also has a **Send test notification** menu item. Run Rainmeter in your normal Windows
desktop session. You can repeat the test after five seconds.

If the toast banner does not appear, open **Settings → System → Notifications**.
Enable notifications, find **Rainmeter Notifications** in the sender list,
and check its banner and Notification Center settings. Press **Win+N** to
look for the test toast in Notification Center. Do Not Disturb can send it
there without showing a banner. For a plugin error, open Rainmeter's **About**
window and check the **Log** tab for `MeasureToast` in ToasterTest or `MeasureTestToast` in GMailNotify.

### Remove the Windows sender registration

Exit Rainmeter. In Registry Editor, go to
`HKEY_CURRENT_USER\Software\Classes\AppUserModelId\Rainmeter.PluginToaster`
and delete the `Rainmeter.PluginToaster` key. It stores `DisplayName`,
`CustomActivator`, and `IconUri` for the shared sender. The next notification
from Toaster recreates the key.

To remove the cached Rainmeter icon as well, delete
`%LOCALAPPDATA%\Rainmeter\Toaster\Rainmeter.png`. Toaster recreates it the next
time Toaster sends a notification.

## 3. Configure Gmail

1. Right-click the skin and choose **Open @Resources folder**.
2. Edit `GMailNotify.inc`. Fill in the first account's ID, app password, and
   display label. Keep your edited file private because it stores the
   configured feed credentials.
3. Fill in the second set of keys if you use a second account. Leave the
   placeholders in place for an unused account. The skin waits for
   configured values before it attempts to poll that account.
4. Refresh the skin from Rainmeter Manage or its **More actions ...** menu.

```ini
[Variables]
GMailID1=YOUR_GMAIL_ADDRESS
GMailPassword1=YOUR_GMAIL_APP_PASSWORD
GMailLabel1=Primary Gmail
GMailID2=YOUR_GMAIL_ADDRESS
GMailPassword2=YOUR_GMAIL_APP_PASSWORD
GMailLabel2=Secondary Gmail
RecentMinutes=15
```

Use an account that can access the Gmail Atom feed with the existing
credential-based setup. Google documents the Atom feed for Google Workspace
accounts and recommends OAuth 2.0. This skin uses the original skin's
credential-based polling. If your account or organization requires OAuth,
the test toast still works and the Gmail feed will need a different
authentication implementation. [Google's Gmail inbox feed documentation](https://developers.google.com/workspace/gmail/gmail_inbox_feed)

If you get **Feed error**, check the selected account's ID and app password,
then use **Check for mail**. The skin checks the selected account about once
per minute while loaded. The first successful check for each account
establishes a baseline. A later message can produce a toast once, with the
latest message tracked separately for each account across refreshes.

Use the right-click menu to switch accounts or check immediately. The skin
opens browser slots `/u/0/` and `/u/1/` for accounts 1 and 2. If your Gmail
sign-in order differs, change `InboxURL1` and `InboxURL2` in
`GMailNotify.ini`. Set `ShowMessageDetails=0` there to omit the sender and
subject from the Windows toast. The skin's own hover tooltip still shows
mail details. Refresh after changing the INI.

## Choose a Gmail icon

Open `GMailNotify.ini` and set `GMailIcon` in the `[Variables]` section to one of these values.

| Setting | Icon |
| --- | --- |
| `GMailIcon=GMailModern` | Flat-color Gmail M (default) |
| `GMailIcon=GMailGradient` | Gradient Gmail M |
| `GMailIcon=GMail` | Original envelope |

Save and refresh the skin. The selection applies to the desktop meter, its tooltip, and new Windows toasts. The desktop icon stays in full color while the inbox has unread mail and switches to grayscale when the unread count reaches zero.


## Use Toaster.dll in another skin

```ini
[MeasureToast]
Measure=Plugin
Plugin=Toaster
Title=Timer finished
Text=Your tea is ready.
Icon=#@#Images\Timer.png
Button1=Open notes
Button1URI=https://example.com/
DynamicVariables=1

[MeterSend]
Meter=String
Text=Send notification
LeftMouseUpAction=[!CommandMeasure MeasureToast "Show"]
```

Remove the `Icon` line if your skin has no image. Use the same
`[!CommandMeasure MeasureToast "Show"]` command from a measure action such as
`OnChangeAction` or `IfTrueAction`. The plugin reads the current INI options
when it receives the command.

- `Title`, `Text`, `Text2`, and `Attribution` set the visible text.
- `Icon`, `Image`, and `HeroImage` add local images. `IconCircle=1` crops the
  content icon to a circle.
- `LaunchURI` runs when the toast body is clicked. `Button1` through `Button5`
  set labels. Add `Button1URI` to open a link or `Button1Action=Snooze` or
  `Dismiss` for a Windows action. Use the matching number for each button.
- `Silent=1` disables toast sound. Set `Audio` to a system sound URI such as
  `ms-winsoundevent:Notification.Mail` to choose a different sound.
- `SoundFile=#@#Sounds\MyAlert.wav` plays a local WAV once after a successful
  `Show` and mutes the toast's built-in sound. `Silent=1`, `SuppressPopup=1`,
  and `ToastXML` disable this playback. This is Rainmeter audio, so the sound
  can play when Windows Do Not Disturb hides the toast banner.
- `Tag` and optional `Group` identify an item for replacement, updating, or
  removal. For progress, set `Progress=1` and set `ProgressValue` from 0 to 1
  or to `indeterminate`. Run `UpdateProgress` with the same tag and group
  after changing the progress options. Run `Remove` to clear the tagged
  notification.
- `Cooldown=5` is the default. It suppresses a repeat of the last identical
  toast from that measure within five seconds. Use `Cooldown=0` to disable
  that check. Set `MinimumInterval` in seconds to also limit changing
  notifications from the same measure. Suppressed requests are discarded.
- `ToastXML` accepts a single-line Windows Toast XML payload for advanced
  layouts. The standalone DLL supports URI links and Windows-managed
  actions. Returning text replies or foreground callbacks to Rainmeter
  requires a Windows activation server. [Windows Toast XML schema](https://learn.microsoft.com/en-us/uwp/schemas/tiles/toastschema/schema-root)

The measure value is 0 before any command, 1 after a successful request,
2 after suppression, or -1 on error. Windows controls banner placement,
appearance, and visibility.
