#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <roapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Notifications.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

#include "../../API/RainmeterAPI.h"

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winmm.lib")

namespace
{
using namespace winrt;
using namespace winrt::Windows::UI::Notifications;
using winrt::Windows::Data::Xml::Dom::XmlDocument;
using Clock = std::chrono::steady_clock;

constexpr wchar_t AppId[] = L"Rainmeter.PluginToaster";

struct Apartment
{
    Apartment()
    {
        auto result = RoInitialize(RO_INIT_SINGLETHREADED);
        if (result == RPC_E_CHANGED_MODE)
        {
            result = RoInitialize(RO_INIT_MULTITHREADED);
        }
        check_hresult(result);
    }
    ~Apartment() { RoUninitialize(); }
};

struct Measure
{
    void* rm;
    std::wstring lastPayload;
    Clock::time_point lastShow{};
    double result = 0.0;
};

std::wstring Read(void* rm, const wchar_t* key, const wchar_t* fallback = L"")
{
    return RmReadString(rm, key, fallback);
}

bool Equals(const std::wstring& value, const wchar_t* expected)
{
    return _wcsicmp(value.c_str(), expected) == 0;
}

std::wstring Escape(std::wstring_view value)
{
    std::wstring escaped;
    for (wchar_t character : value)
    {
        switch (character)
        {
        case L'&': escaped += L"&amp;"; break;
        case L'<': escaped += L"&lt;"; break;
        case L'>': escaped += L"&gt;"; break;
        case L'"': escaped += L"&quot;"; break;
        case L'\'': escaped += L"&apos;"; break;
        default: escaped += character; break;
        }
    }
    return escaped;
}

void Attribute(std::wstring& xml, const wchar_t* name, std::wstring_view value)
{
    if (value.empty()) return;
    xml += L' ';
    xml += name;
    xml += L"=\"";
    xml += Escape(value);
    xml += L'"';
}

std::wstring Source(void* rm, const wchar_t* key)
{
    const auto value = Read(rm, key);
    if (value.empty() || (PathGetDriveNumberW(value.c_str()) < 0 && UrlIsW(value.c_str(), URLIS_URL)))
    {
        return value;
    }
    if (UrlIsW(value.c_str(), URLIS_FILEURL)) return value;

    const std::wstring path = RmPathToAbsolute(rm, value.c_str());
    std::wstring uri(path.size() * 12 + 16, L'\0');
    DWORD size = static_cast<DWORD>(uri.size());
    check_hresult(UrlCreateFromPathW(path.c_str(), uri.data(), &size, 0));
    uri.resize(size);
    return uri;
}

void AddText(std::wstring& xml, const std::wstring& value, bool attribution = false)
{
    if (value.empty()) return;
    xml += attribution ? L"<text placement=\"attribution\">" : L"<text>";
    xml += Escape(value);
    xml += L"</text>";
}

void AddImage(std::wstring& xml, const std::wstring& source, const wchar_t* placement,
    const std::wstring& alt, bool circle = false)
{
    if (source.empty()) return;
    xml += L"<image";
    Attribute(xml, L"src", source);
    Attribute(xml, L"placement", placement);
    Attribute(xml, L"alt", alt);
    if (circle) Attribute(xml, L"hint-crop", L"circle");
    xml += L"/>";
}

void AddButtons(std::wstring& xml, void* rm)
{
    std::wstring actions;
    for (int index = 1; index <= 5; ++index)
    {
        const auto prefix = L"Button" + std::to_wstring(index);
        const auto label = Read(rm, prefix.c_str());
        if (label.empty()) continue;

        const auto uri = Read(rm, (prefix + L"URI").c_str());
        const auto action = Read(rm, (prefix + L"Action").c_str(), L"Dismiss");
        actions += L"<action";
        Attribute(actions, L"content", label);
        if (!uri.empty())
        {
            Attribute(actions, L"activationType", L"protocol");
            Attribute(actions, L"arguments", uri);
        }
        else if (Equals(action, L"Snooze") || Equals(action, L"Dismiss"))
        {
            Attribute(actions, L"activationType", L"system");
            Attribute(actions, L"arguments", Equals(action, L"Snooze") ? L"snooze" : L"dismiss");
        }
        else
        {
            throw hresult_invalid_argument(L"ButtonAction must be Snooze or Dismiss. Set ButtonURI to open a link.");
        }

        Attribute(actions, L"imageUri", Source(rm, (prefix + L"Icon").c_str()));
        if (RmReadInt(rm, (prefix + L"ContextMenu").c_str(), 0))
        {
            Attribute(actions, L"placement", L"contextMenu");
        }
        actions += L"/>";
    }
    if (!actions.empty()) xml += L"<actions>" + actions + L"</actions>";
}

std::wstring BuildXml(void* rm, bool progress, bool muteNativeAudio)
{
    const auto customXml = Read(rm, L"ToastXML");
    if (!customXml.empty()) return customXml;

    const auto title = Read(rm, L"Title");
    const auto text = Read(rm, L"Text");
    const auto text2 = Read(rm, L"Text2");
    const auto icon = Source(rm, L"Icon");
    const auto hero = Source(rm, L"HeroImage");
    const auto image = Source(rm, L"Image");
    if (title.empty() && text.empty() && text2.empty() && icon.empty() &&
        hero.empty() && image.empty() && !progress)
    {
        throw hresult_invalid_argument(L"Set notification text, an image, Progress=1, or ToastXML.");
    }

    std::wstring xml = L"<toast";
    const auto uri = Read(rm, L"LaunchURI");
    if (!uri.empty())
    {
        Attribute(xml, L"activationType", L"protocol");
        Attribute(xml, L"launch", uri);
    }
    Attribute(xml, L"duration", Read(rm, L"Duration"));
    Attribute(xml, L"scenario", Read(rm, L"Scenario"));
    Attribute(xml, L"displayTimestamp", Read(rm, L"Timestamp"));
    xml += L"><visual><binding template=\"ToastGeneric\">";
    AddText(xml, title);
    AddText(xml, text);
    AddText(xml, text2);
    AddText(xml, Read(rm, L"Attribution"), true);
    AddImage(xml, icon, L"appLogoOverride", Read(rm, L"IconAlt"), RmReadInt(rm, L"IconCircle", 0) != 0);
    AddImage(xml, hero, L"hero", Read(rm, L"HeroImageAlt"));
    AddImage(xml, image, L"", Read(rm, L"ImageAlt"));
    if (progress)
    {
        xml += L"<progress title=\"{progressTitle}\" value=\"{progressValue}\" "
            L"valueStringOverride=\"{progressValueText}\" status=\"{progressStatus}\"/>";
    }
    xml += L"</binding></visual>";
    AddButtons(xml, rm);

    if (muteNativeAudio)
    {
        xml += L"<audio silent=\"true\"/>";
    }
    else
    {
        const auto sound = Source(rm, L"Audio");
        if (!sound.empty())
        {
            xml += L"<audio";
            Attribute(xml, L"src", sound);
            if (RmReadInt(rm, L"AudioLoop", 0)) Attribute(xml, L"loop", L"true");
            xml += L"/>";
        }
    }
    return xml + L"</toast>";
}

NotificationData ProgressData(void* rm, std::wstring* signature = nullptr)
{
    const auto title = Read(rm, L"ProgressTitle");
    auto value = Read(rm, L"ProgressValue", L"0");
    if (Equals(value, L"indeterminate"))
    {
        value = L"indeterminate";
    }
    else
    {
        double number = RmReadFormula(rm, L"ProgressValue", -1.0);
        if (value == L"0") number = 0.0;
        if (!std::isfinite(number) || number < 0.0 || number > 1.0)
        {
            throw hresult_invalid_argument(L"ProgressValue must be between 0 and 1, or indeterminate.");
        }
        value = std::to_wstring(number);
        const auto decimal = value.find(L',');
        if (decimal != std::wstring::npos) value[decimal] = L'.';
    }
    const auto label = Read(rm, L"ProgressValueText");
    const auto status = Read(rm, L"ProgressStatus", L"In progress");

    NotificationData data;
    auto values = data.Values();
    values.Insert(L"progressTitle", title);
    values.Insert(L"progressValue", value);
    values.Insert(L"progressValueText", label);
    values.Insert(L"progressStatus", status);
    if (signature)
    {
        Attribute(*signature, L"title", title);
        Attribute(*signature, L"value", value);
        Attribute(*signature, L"label", label);
        Attribute(*signature, L"status", status);
    }
    return data;
}

std::wstring RainmeterSenderIcon()
{
    PWSTR appData = nullptr;
    check_hresult(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appData));
    const auto freeAppData = std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)>(appData, CoTaskMemFree);
    const auto directory = std::filesystem::path(appData) / L"Rainmeter" / L"Toaster";
    const auto path = directory / L"Rainmeter.png";
    if (std::filesystem::is_regular_file(path)) return path.wstring();

    HMODULE module = nullptr;
    check_bool(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, AppId, &module));
    const auto resource = check_pointer(FindResourceW(module, MAKEINTRESOURCEW(101), RT_RCDATA));
    const auto bytes = static_cast<const char*>(check_pointer(LockResource(
        check_pointer(LoadResource(module, resource)))));
    const DWORD size = SizeofResource(module, resource);
    if (!size) throw_last_error();

    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
    {
        throw hresult_error(HRESULT_FROM_WIN32(error.value()), L"Could not create the sender icon directory.");
    }

    wchar_t temporary[MAX_PATH]{};
    check_bool(GetTempFileNameW(directory.c_str(), L"rmt", 0, temporary));
    std::ofstream output(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
    output.write(bytes, size);
    output.close();
    if (!output)
    {
        DeleteFileW(temporary);
        throw hresult_error(E_FAIL, L"Could not write the default sender icon.");
    }
    if (!MoveFileExW(temporary, path.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        const auto result = HRESULT_FROM_WIN32(GetLastError());
        DeleteFileW(temporary);
        throw hresult_error(result, L"Could not install the default sender icon.");
    }
    return path.wstring();
}

void RegisterSender()
{
    const auto icon = RainmeterSenderIcon();
    const auto registryPath = std::wstring(L"Software\\Classes\\AppUserModelId\\") + AppId;
    constexpr wchar_t displayName[] = L"Rainmeter Notifications";
    constexpr wchar_t activatorId[] = L"{1229C6CD-D356-4822-B59A-03BA37DA3F35}";
    HKEY rawKey = nullptr;
    check_win32(RegCreateKeyExW(HKEY_CURRENT_USER, registryPath.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &rawKey, nullptr));
    const auto key = std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(&RegCloseKey)>(rawKey, RegCloseKey);
    check_win32(RegSetValueExW(key.get(), L"DisplayName", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(displayName), sizeof(displayName)));
    check_win32(RegSetValueExW(key.get(), L"CustomActivator", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(activatorId), sizeof(activatorId)));
    check_win32(RegSetValueExW(key.get(), L"IconUri", 0, REG_EXPAND_SZ,
        reinterpret_cast<const BYTE*>(icon.c_str()),
        static_cast<DWORD>((icon.size() + 1) * sizeof(wchar_t))));
}

void Show(Measure& measure, const std::wstring& tag, const std::wstring& group)
{
    auto* rm = measure.rm;
    const bool progress = RmReadInt(rm, L"Progress", 0) != 0;
    const bool suppressPopup = RmReadInt(rm, L"SuppressPopup", 0) != 0;
    if (suppressPopup && tag.empty())
    {
        throw hresult_invalid_argument(L"SuppressPopup=1 requires a Tag.");
    }

    const bool silent = RmReadInt(rm, L"Silent", 0) != 0;
    auto soundFile = Read(rm, L"SoundFile");
    if (silent || suppressPopup || !Read(rm, L"ToastXML").empty()) soundFile.clear();
    if (!soundFile.empty()) soundFile = RmPathToAbsolute(rm, soundFile.c_str());
    const auto xml = BuildXml(rm, progress, silent || !soundFile.empty());
    std::wstring signature;
    Attribute(signature, L"sound", soundFile);
    Attribute(signature, L"tag", tag);
    Attribute(signature, L"group", group);
    Attribute(signature, L"popup", suppressPopup ? L"0" : L"1");
    const auto expires = RmReadDouble(rm, L"ExpiresAfter", 0.0);
    if (!std::isfinite(expires) || expires < 0 || expires > 31536000.0)
    {
        throw hresult_invalid_argument(L"ExpiresAfter must be 0 or a positive number of seconds up to one year.");
    }
    Attribute(signature, L"expires", std::to_wstring(expires));
    signature += xml;
    Apartment apartment;
    const auto data = progress ? ProgressData(rm, &signature) : NotificationData{nullptr};

    const auto cooldown = RmReadDouble(rm, L"Cooldown", 5.0);
    const auto minimumInterval = RmReadDouble(rm, L"MinimumInterval", 0.0);
    const double elapsed = std::chrono::duration<double>(Clock::now() - measure.lastShow).count();
    if (!measure.lastPayload.empty() &&
        (elapsed < minimumInterval || (signature == measure.lastPayload && elapsed < cooldown)))
    {
        measure.result = 2.0;
        return;
    }

    XmlDocument document;
    winrt::Windows::Data::Xml::Dom::XmlLoadSettings settings;
    settings.ProhibitDtd(true);
    settings.ResolveExternals(false);
    document.LoadXml(xml, settings);
    const auto root = document.DocumentElement();
    if (!root || root.TagName() != L"toast")
    {
        throw hresult_invalid_argument(L"ToastXML must have a <toast> root element.");
    }
    ToastNotification toast(document);
    if (!tag.empty()) toast.Tag(tag);
    if (!group.empty()) toast.Group(group);
    if (suppressPopup) toast.SuppressPopup(true);
    if (data) toast.Data(data);
    if (expires > 0)
    {
        toast.ExpirationTime(clock::now() + std::chrono::duration_cast<clock::duration>(
            std::chrono::duration<double>(expires)));
    }

    try
    {
        RegisterSender();
    }
    catch (const hresult_error& error)
    {
        throw hresult_error(error.code(), L"Could not register the Rainmeter Notifications sender.");
    }

    ToastNotifier notifier{nullptr};

    try
    {
        notifier = ToastNotificationManager::CreateToastNotifier(AppId);

    }
    catch (const hresult_error& error)
    {
        throw hresult_error(error.code(), L"Could not create the Windows toast notifier.");
    }
    try
    {
        notifier.Show(toast);
    }
    catch (const hresult_error& error)
    {
        throw hresult_error(error.code(), L"Windows rejected the notification.");
    }
    measure.lastPayload = std::move(signature);
    measure.lastShow = Clock::now();
    measure.result = 1.0;
    if (!soundFile.empty())
    {
        if (PlaySoundW(soundFile.c_str(), nullptr, SND_ASYNC | SND_FILENAME | SND_NODEFAULT | SND_SYSTEM))
        {
            RmLog(rm, LOG_DEBUG, L"Toaster: started WAV playback.");
        }
        else
        {
            RmLog(rm, LOG_WARNING, L"Toaster: could not play SoundFile.");
        }
    }
}
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
    *data = new Measure{rm, {}, {}, 0.0};
}

PLUGIN_EXPORT void Reload(void*, void*, double* maxValue)
{
    *maxValue = 2.0;
}

PLUGIN_EXPORT double Update(void* data)
{
    return static_cast<Measure*>(data)->result;
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
    auto& measure = *static_cast<Measure*>(data);
    auto* rm = measure.rm;
    std::wstring command(args);
    const auto first = command.find_first_not_of(L" \t");
    if (first != std::wstring::npos)
    {
        command = command.substr(first, command.find_last_not_of(L" \t") - first + 1);
    }

    try
    {
        const auto tag = Read(rm, L"Tag");
        const auto group = Read(rm, L"Group");
        if (Equals(command, L"Show"))
        {
            Show(measure, tag, group);
            return;
        }

        if (!Equals(command, L"UpdateProgress") && !Equals(command, L"Remove"))
        {
            throw hresult_invalid_argument(L"Expected Show, UpdateProgress, or Remove.");
        }
        if (tag.empty()) throw hresult_invalid_argument(L"This command requires a Tag.");

        Apartment apartment;
        if (Equals(command, L"UpdateProgress"))
        {
            const auto notifier = ToastNotificationManager::CreateToastNotifier(AppId);
            const auto progress = ProgressData(rm);
            const auto result = group.empty() ? notifier.Update(progress, tag) : notifier.Update(progress, tag, group);
            if (result == NotificationUpdateResult::NotificationNotFound)
            {
                throw hresult_error(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), L"The tagged toast has expired or was dismissed.");
            }
            if (result != NotificationUpdateResult::Succeeded)
            {
                throw hresult_error(E_FAIL, L"Windows could not update the tagged toast.");
            }
        }
        else
        {
            ToastNotificationManager::History().Remove(tag, group, AppId);
            measure.lastPayload.clear();
        }
        measure.result = 1.0;
    }
    catch (const hresult_error& error)
    {
        measure.result = -1.0;
        RmLogF(rm, LOG_ERROR, L"Toaster: %s (0x%08X)", error.message().c_str(),
            static_cast<unsigned int>(error.code().value));
    }
    catch (...)
    {
        measure.result = -1.0;
        RmLog(rm, LOG_ERROR, L"Toaster: unexpected error.");
    }
}

PLUGIN_EXPORT void Finalize(void* data)
{
    delete static_cast<Measure*>(data);
}
