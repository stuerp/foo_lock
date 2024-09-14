
/** $VER: foo_lock.cpp (2024.09.14) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <foobar2000.h>

#include <wtsapi32.h>

#include "Resource.h"

#pragma hdrstop

namespace
{
    #pragma warning(disable: 4265 4625 4626 5026 5027 26433 26436 26455)
    DECLARE_COMPONENT_VERSION
    (
        STR_COMPONENT_NAME,
        STR_COMPONENT_VERSION,
        STR_COMPONENT_BASENAME " " STR_COMPONENT_VERSION "\n"
            STR_COMPONENT_COPYRIGHT "\n"
            "\n"
            STR_COMPONENT_DESCRIPTION "\n"
            "\n"
            "Built with foobar2000 SDK " TOSTRING(FOOBAR2000_SDK_VERSION) "\n"
            "on " __DATE__ " " __TIME__ "."
    )
    VALIDATE_COMPONENT_FILENAME(STR_COMPONENT_FILENAME)
}

#pragma region(Configuration)
static const GUID CfgIsEnabledGUID = { 0x79fc6de5, 0x291, 0x47ab, { 0xa8, 0x6, 0xb4, 0x76, 0x4b, 0x1c, 0x22, 0x79 } };

static const GUID CfgActionsGUID = { 0x33d8f2ba, 0xe737, 0x4d1e, { 0xbd, 0xf3, 0x8d, 0xb6, 0x32, 0x98, 0xee, 0x2 } };
static const GUID CfgLockActionsGUID = { 0x6d622351, 0xbdba, 0x4687, { 0xbe, 0xbf, 0x3e, 0x5b, 0xe9, 0x46, 0x66, 0xe9 } };
static const GUID CfgUnlockActionsGUID = { 0xc333a02d, 0xf8ac, 0x4f91, { 0x95, 0xc0, 0xce, 0x42, 0x37, 0x28, 0x54, 0xb1 } };

static const GUID guid_cfg_on_lock_nothing = { 0x26ad2444, 0x3960, 0x4732, { 0x9b, 0xbb, 0xc0, 0xdd, 0x4d, 0x6e, 0x81, 0x34 } };
static const GUID guid_cfg_on_lock_pause = { 0xd2a3ae2c, 0x5ece, 0x45de, { 0x90, 0x91, 0x49, 0x66, 0xd2, 0x7a, 0xb8, 0xa9 } };
static const GUID guid_cfg_on_lock_stop = { 0x420e0363, 0x7ca9, 0x48e0, { 0xbd, 0x22, 0x3c, 0x34, 0x97, 0xc6, 0x2f, 0x5b } };

static const GUID guid_cfg_on_unlock_nothing = { 0xdabcc7d7, 0x4194, 0x4bfc, { 0xbe, 0xe9, 0xa0, 0x17, 0x48, 0x14, 0x8, 0x7e } };
static const GUID guid_cfg_on_unlock_unpause = { 0x898bffd4, 0xa3ef, 0x4b70, { 0xa6, 0x61, 0xa4, 0xa1, 0x2c, 0x9e, 0x64, 0x28 } };
static const GUID guid_cfg_on_unlock_play = { 0x1727eee9, 0x6b88, 0x4e83, { 0x92, 0x5e, 0x5c, 0x63, 0x3f, 0x87, 0x40, 0x59 } };

static cfg_int CfgIsEnabled(CfgIsEnabledGUID, 0);

static advconfig_branch_factory CfgActions(STR_COMPONENT_BASENAME, CfgActionsGUID, advconfig_entry::guid_branch_playback, 0.0);
    static advconfig_branch_factory CfgLockActions("On lock", CfgLockActionsGUID, CfgActionsGUID, 0.0);
        static advconfig_radio_factory CfgLockDoNothing("Do nothing", guid_cfg_on_lock_nothing, CfgLockActionsGUID, 0.0, false);
        static advconfig_radio_factory CfgLockPausePlayback("Pause playback", guid_cfg_on_lock_pause, CfgLockActionsGUID, 1.0, true);
        static advconfig_radio_factory CfgLockStopPlayback("Stop playback", guid_cfg_on_lock_stop, CfgLockActionsGUID, 2.0, false);
    static advconfig_branch_factory CfgUnlockActions("On unlock", CfgUnlockActionsGUID, CfgActionsGUID, 1.0);
        static advconfig_radio_factory CfgUnlockDoNothing("Do nothing", guid_cfg_on_unlock_nothing, CfgUnlockActionsGUID, 0.0, false);
        static advconfig_radio_factory CfgUnlockResumePlayback("Resume playback", guid_cfg_on_unlock_unpause, CfgUnlockActionsGUID, 1.0, true);
        static advconfig_radio_factory CfgUnlockStartPlayback("Start playback", guid_cfg_on_unlock_play, CfgUnlockActionsGUID, 2.0, false);
#pragma endregion

typedef BOOL(WINAPI * WTSRegisterSessionNotificationPtr)(HWND hWnd, DWORD flags);
typedef BOOL(WINAPI * WTSUnRegisterSessionNotificationPtr)(HWND hWnd);

static bool _IsResuming = false;

static const WCHAR ClassName[] = L"870AC6B2-D141-46f4-A196-ADCB72B8AE4E";
static const WCHAR WindowTitle[] = TEXT(STR_COMPONENT_BASENAME) L" Session Notification Listener";

/// <summary>
/// Implements a listener for session notifications.
/// </summary>
class NotificationListener
{
public:
    NotificationListener()
    {
        _hWTSAPI = NULL;
        _hWnd = NULL;

        _IsConnected = true;
    }

    ~NotificationListener()
    {
        if (::IsWindow(_hWnd))
            ::DestroyWindow(_hWnd);

        if (_WindowClass)
            ::UnregisterClassW(ClassName, _hInstance);

        if (_hWTSAPI)
            ::FreeLibrary(_hWTSAPI);
    }

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    bool Initialize(HINSTANCE hInstance)
    {
        _hInstance = hInstance;

        {
            _hWTSAPI = ::LoadLibraryW(L"wtsapi32.dll");

            if (_hWTSAPI == NULL)
                return false;

            _RegisterSessionNotification = (WTSRegisterSessionNotificationPtr)(void *) ::GetProcAddress(_hWTSAPI, "WTSRegisterSessionNotification");

            if (_RegisterSessionNotification == nullptr)
                return false;

            _UnRegisterSessionNotification = (WTSUnRegisterSessionNotificationPtr)(void *) ::GetProcAddress(_hWTSAPI, "WTSUnRegisterSessionNotification");

            if (_UnRegisterSessionNotification == nullptr)
                return false;
        }

        {
            WNDCLASSW wc = { };

            wc.hInstance = hInstance;
            wc.lpfnWndProc = (WNDPROC) WndProc;
            wc.lpszClassName = ClassName;

            _WindowClass = ::RegisterClassW(&wc);

            if (_WindowClass == 0)
                return false;
        }

        _hWnd = ::CreateWindowExW(0, ClassName, WindowTitle, 0, 0, 0, 0, 0, 0, 0, hInstance, this);

        if (_hWnd == NULL)
            return false;

        bool Success = (bool) _RegisterSessionNotification(_hWnd, NOTIFY_FOR_THIS_SESSION);

        if (!Success)
            console::print(STR_COMPONENT_BASENAME " failed to register for session notifications.");

        return Success;
    }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        static NotificationListener * This = nullptr;

        if (msg == WM_CREATE)
            This = (NotificationListener *) ((CREATESTRUCT *) (lParam))->lpCreateParams;

        return This->WindowProc(hWnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
    {
        switch (uMessage)
        {
            case WM_DESTROY:
                _UnRegisterSessionNotification(hWnd);
                break;

            case WM_WTSSESSION_CHANGE:
            {
                switch (wParam)
                {
                    case WTS_SESSION_LOCK:
                    {
                        console::print(STR_COMPONENT_BASENAME, " is processing a session lock notification.");

                        OnLock();
                        break;
                    }

                    case WTS_SESSION_UNLOCK:
                    {
                        console::print(STR_COMPONENT_BASENAME, " is processing a session unlock notification.");

                        if (_IsConnected)
                            OnUnlock();
                        break;
                    }

                    case WTS_CONSOLE_CONNECT:
                    {
                        console::print(STR_COMPONENT_BASENAME, " is processing a connection to the console terminal.");

                        OnUnlock();
                        _IsConnected = true;
                        break;
                    }

                    case WTS_CONSOLE_DISCONNECT:
                    {
                        console::print(STR_COMPONENT_BASENAME, " is processing a disconnect from the console terminal.");

                        _IsConnected = false;
                        break;
                    }
                }
            }

            default:
                return ::DefWindowProcW(hWnd, uMessage, wParam, lParam);
        }

        return 0;
    }

    enum Actions
    {
        DoNothing = 0,
        PausePlayback,
        StopPlayback,
        ResumePlayback,
        StartPlayback
    };

    static void OnLock()
    {
        pfc::string8 Message("Workstation locked");

        static_api_ptr_t<playback_control> pbc;

        Actions Action = GetLockAction();

        #pragma warning(disable: 4062)
        switch (Action)
        {
            case Actions::PausePlayback:
            {
                if (pbc->is_playing() && !pbc->is_paused())
                {
                    standard_commands::main_pause();
                    Message += ". Paused playback";
                    _IsResuming = true;
                }
                break;
            }

            case Actions::StopPlayback:
            {
                if (pbc->is_playing())
                {
                    standard_commands::main_stop();
                    Message += ". Stopped playback";
                    _IsResuming = true;
                }
            }
        }
        #pragma warning(default: 4062)

        Message.add_byte('.');
        console::info(Message);
    }

    static void OnUnlock()
    {
        pfc::string8 Message("Workstation unlocked");

        static_api_ptr_t<play_control> pbc;

        Actions Action = GetUnlockAction();

        #pragma warning(disable: 4062)
        switch (Action)
        {
            case Actions::ResumePlayback:
            {
                if (_IsResuming)
                {
                    _IsResuming = false;

                    if (pbc->is_paused())
                    {
                        standard_commands::main_play();

                        Message += ". Resumed playback";
                    }
                }
                break;
            }

            case Actions::StartPlayback:
            {
                if (_IsResuming)
                {
                    _IsResuming = false;

                    standard_commands::main_play();

                    Message += ". Started playback";
                }
            }
        }
        #pragma warning(default: 4062)

        Message.add_byte('.');
        console::info(Message);
    }

    static Actions GetLockAction()
    {
        if (CfgLockDoNothing.get())
            return Actions::DoNothing;

        if (CfgLockPausePlayback.get())
            return Actions::PausePlayback;

        return Actions::StopPlayback;
    }

    static Actions GetUnlockAction()
    {
        if (CfgUnlockDoNothing.get())
            return Actions::DoNothing;

        if (CfgUnlockResumePlayback.get())
            return Actions::ResumePlayback;

        return Actions::StartPlayback;
    }

private:
    HINSTANCE _hInstance;
    HWND _hWnd;

    ATOM _WindowClass;

    HMODULE _hWTSAPI;
    WTSRegisterSessionNotificationPtr _RegisterSessionNotification;
    WTSUnRegisterSessionNotificationPtr _UnRegisterSessionNotification;

    bool _IsConnected;
};

#pragma region(InitQuit)
NotificationListener * _NotificationListener = nullptr;

static void Enable()
{
    _NotificationListener = new NotificationListener;

    _NotificationListener->Initialize(core_api::get_my_instance());
}

static void Disable()
{
    if (_NotificationListener)
    {
        delete _NotificationListener;
        _NotificationListener = nullptr;
    }
}

class InitQuitHandler : public initquit
{
    virtual void on_init()
    {
        if (CfgIsEnabled)
            Enable();
    }

    virtual void on_quit()
    {
        Disable();
    }
};

static initquit_factory_t <InitQuitHandler> _InitQuitFactory;
#pragma endregion

#pragma region(MainMenuCommands)
class mainmenu_command_foolock : public mainmenu_commands
{
    virtual t_uint32 get_command_count()
    {
        return 1;
    }

    virtual GUID get_command(t_uint32)
    {
        static const GUID guid = { 0xaf0e45ef, 0x3c49, 0x4d54, { 0xa3, 0xd2, 0xde, 0x81, 0x34, 0x81, 0x3f, 0xfa } }; // {AF0E45EF-3C49-4d54-A3D2-DE8134813FFA}

        return guid;
    }

    virtual void get_name(t_uint32, pfc::string_base & p_out)
    {
        p_out = "Pause on lock";
    }

    virtual bool get_description(t_uint32, pfc::string_base & p_out)
    {
        p_out = "Performs a playback action when the workstation is locked or unlocked or the session is disconnected or reconnected.";

        return true;
    }

    virtual GUID get_parent()
    {
        return mainmenu_groups::playback_etc;
    }

    virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags)
    {
        p_flags = (t_uint32) (CfgIsEnabled ? flag_checked : 0);

        get_name(p_index, p_text);

        return true;
    }

    virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
    {
        if (p_index == 0 && core_api::assert_main_thread())
        {
            if (CfgIsEnabled)
            {
                CfgIsEnabled = 0;
                Disable();
            }
            else
            {
                CfgIsEnabled = 1;
                Enable();
            }
        }
    }
};

static mainmenu_commands_factory_t <mainmenu_command_foolock> _MainMenuCommandsFactory;
#pragma endregion

/*
     © 2004 Kreisquadratur
     icq#56165405

     © 2004-2019 Christopher Snowhill

    2020-02-28 22:33 UTC - kode54
    - Set sensible defaults to match the old versions
    - Version is now 0.8

    2019-07-30 22:17 UTC - kode54
    - Remove Win9x code entirely
    - Add Advanced Configuration options to control default actions
    - Version is now 0.7

    2017-02-04 04:45 UTC - kode54
    - Add link to about string
    - Version is now 0.6

    2010-01-11 20:17 UTC - kode54
    - Added filename validator
    - Version is now 0.5

    2009-07-22 00:55 UTC - kode54
    - Removed Win9x detection since it's been a long time since foobar2000 supported Win9x
    - Changed capitalization of menu command name
    - Updated component version description notice and copyright

    2004-02-24 21:44 UTC - kode54
    - Disable function is now a little saner
    - Version is now 0.4

    2004-02-09 18:15 UTC - kode54
    - Fixed multi-user session monitoring ... see large comment block below :)
    - Version is now 0.3

    2004-02-09 15:59 UTC - kode54
    - Added new WTSAPI32-based window class for session monitoring in Windows XP and 2003 Server
    - Consolidated lock/unlock code into static functions
    - Safeguarded enable/disable functions with win_ver, set by initial call to GetVersion()
    - Changed cfg_waslocked/cfg_resume to static BOOL ... not sure these should persist across sessions
    - Version is now 0.2
*/
