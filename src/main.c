#define UNICODE

#include <assert.h>
#include <windows.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#include "common.h"

#define WINDOW_MIN_WIDTH    420
#define WINDOW_MIN_HEIGHT   300


enum control_id_t {
    ID_BTN_HASH_SEARCH,
    ID_BTN_NAME_SEARCH,
    ID_BTN_HELP,
    ID_BTN_CONFIG,
    ID_BTN_LOAD,
    ID_BTN_SAVE_AS,
    ID_STATIC_SUB_LANG,
    ID_STATIC_TITLE,
    ID_STATIC_SEASON,
    ID_STATIC_EPISODE,
    ID_EDIT_TITLE,
    ID_EDIT_SEASON,
    ID_EDIT_EPISODE,
    ID_COMBO_SUB_LANG,
    ID_LIST_BOX_SUBS
};

typedef struct win_controls_t win_controls_t;
struct win_controls_t
{
    HWND button_hash_search;
    HWND button_name_search;
    HWND button_help;
    HWND button_config;
    HWND button_load;
    HWND button_save_as;

    HWND static_sub_lang;
    HWND static_title;
    HWND static_season;
    HWND static_episode;
    // TODO create control for displaying errors/warnings

    HWND edit_title;
    HWND edit_season;
    HWND edit_episode;

    HWND combo_sub_lang;

    HWND list_box_subs;
};

#define PATH_LENGTH_LIMIT 512 // TODO can do sth dynamic later if you want

typedef struct config_t config_t;
struct config_t
{
    char curl_location[PATH_LENGTH_LIMIT]; // TODO default should be C:\Windows\System32\curl.exe
};

global_variable win_controls_t win_controls = {0};

void api_request(config_t * config, char * url, char * method, char * other_args)
{
    char * api_key = "Y4NKks8ItQc9ZunAMUVlS1iUmVdk81Pk";
    char * user_agent = "videosub 0.1.0";

    char all_arguments[2048];
    int bytes_written = 0;
    bytes_written += stbsp_snprintf(
        &all_arguments[bytes_written], sizeof(all_arguments) - bytes_written,
        "--location --url \"%s\" --request \"%s\" -H \"Api-Key: %s\" -H \"User-Agent: %s\""
        " -H \"Content-Type: application/json\" -H \"Accept: */*\""
        , url, method, api_key, user_agent
    );

    assert(bytes_written < sizeof(all_arguments));

    if (other_args)
    {
        // TODO should think about all the ways api_request() will be called, and whether other_args makes sense
        bytes_written += stbsp_snprintf(
            &all_arguments[bytes_written], sizeof(all_arguments) - bytes_written,
            " %s", other_args
        );
    }

    assert(bytes_written < sizeof(all_arguments));

    STARTUPINFOA startup_info =
    {
        .cb = sizeof(STARTUPINFO)
    };
    // TODO alternative to manual initialisation: GetStartupInfo(&startup_info);

    PROCESS_INFORMATION process_info = {0};

    // TODO probably want to use CreateProcessW, but need to convert string arguments first
    BOOL success = CreateProcessA(
        config->curl_location,
        all_arguments,
        NULL,
        NULL,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT, // TODO other process creation flags
        NULL,
        NULL,
        &startup_info,
        &process_info
    );

    if (!success)
    {
        // TODO replace with warning/error system
        assert(0);
    }
}

// TODO config window stuff
// BOOL save_config(config_t * config, config_controls_t * controls)
// {
//     // try saving everything
//     // if we have an issue, throw up a message dialog box and return FALSE (indicates not to close config dialog)
//     return TRUE;
// }

void set_win_layout(win_controls_t * win_controls, int client_width, int client_height)
{
    #define MARGIN_SIZE     10
    #define COLUMN_GAP      5
    #define ROW_GAP         5
    #define ROW_HEIGHT      22 // height of most controls
    #define NUM_COLUMNS     4

    #define BUTTON_HEIGHT   ROW_HEIGHT
    #define STATIC_HEIGHT   20
    #define COMBO_HEIGHT    20
    #define EDIT_HEIGHT     20

    #define LIST_BOX_GAP    25

    int current_x, current_y;
    int column_width = (client_width - 2 * MARGIN_SIZE - (NUM_COLUMNS - 1) * COLUMN_GAP) / NUM_COLUMNS;

    current_x = MARGIN_SIZE;
    current_y = MARGIN_SIZE;

    SetWindowPos(win_controls->static_sub_lang, NULL, current_x, current_y, column_width, STATIC_HEIGHT, SWP_NOZORDER);
    current_x += column_width + COLUMN_GAP;
    SetWindowPos(win_controls->combo_sub_lang, NULL, current_x, current_y, column_width * 2 + COLUMN_GAP, COMBO_HEIGHT, SWP_NOZORDER);
    current_x += column_width * 2 + COLUMN_GAP * 2;
    SetWindowPos(win_controls->button_hash_search, NULL, current_x, current_y, column_width, BUTTON_HEIGHT, SWP_NOZORDER);

    current_x = MARGIN_SIZE;
    current_y += ROW_HEIGHT + ROW_GAP;

    SetWindowPos(win_controls->static_title, NULL, current_x, current_y, column_width, STATIC_HEIGHT, SWP_NOZORDER);
    current_x += column_width + COLUMN_GAP;
    SetWindowPos(win_controls->edit_title, NULL, current_x, current_y, column_width * 2 + COLUMN_GAP, EDIT_HEIGHT, SWP_NOZORDER);
    current_x += column_width * 2 + COLUMN_GAP * 2;
    SetWindowPos(win_controls->button_name_search, NULL, current_x, current_y, column_width, BUTTON_HEIGHT, SWP_NOZORDER);

    current_x = MARGIN_SIZE;
    current_y += ROW_HEIGHT + ROW_GAP;

    SetWindowPos(win_controls->static_season, NULL, current_x, current_y, column_width, STATIC_HEIGHT, SWP_NOZORDER);
    current_x += column_width + COLUMN_GAP;
    SetWindowPos(win_controls->edit_season, NULL, current_x, current_y, column_width * 2 + COLUMN_GAP, EDIT_HEIGHT, SWP_NOZORDER);

    current_x = MARGIN_SIZE;
    current_y += ROW_HEIGHT + ROW_GAP;

    SetWindowPos(win_controls->static_episode, NULL, current_x, current_y, column_width, STATIC_HEIGHT, SWP_NOZORDER);
    current_x += column_width + COLUMN_GAP;
    SetWindowPos(win_controls->edit_episode, NULL, current_x, current_y, column_width * 2 + COLUMN_GAP, EDIT_HEIGHT, SWP_NOZORDER);

    current_x = MARGIN_SIZE;
    current_y += ROW_HEIGHT + ROW_GAP;

    int list_box_height = client_height - (current_y + LIST_BOX_GAP + ROW_HEIGHT + ROW_GAP + MARGIN_SIZE);

    SetWindowPos(win_controls->list_box_subs, NULL, current_x, current_y, column_width * 4 + COLUMN_GAP * 3, list_box_height, SWP_NOZORDER);

    current_x = MARGIN_SIZE;
    current_y += list_box_height + LIST_BOX_GAP;

    SetWindowPos(win_controls->button_help, NULL, current_x, current_y, column_width, BUTTON_HEIGHT, SWP_NOZORDER);
    current_x += column_width + COLUMN_GAP;
    SetWindowPos(win_controls->button_config, NULL, current_x, current_y, column_width, BUTTON_HEIGHT, SWP_NOZORDER);
    current_x += column_width + COLUMN_GAP;
    SetWindowPos(win_controls->button_load, NULL, current_x, current_y, column_width, BUTTON_HEIGHT, SWP_NOZORDER);
    current_x += column_width + COLUMN_GAP;
    SetWindowPos(win_controls->button_save_as, NULL, current_x, current_y, column_width, BUTTON_HEIGHT, SWP_NOZORDER);

    current_x = MARGIN_SIZE;
    current_y += ROW_HEIGHT + ROW_GAP;

    assert(current_y == client_height - MARGIN_SIZE);
}

LRESULT CALLBACK main_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO min_max_info = (LPMINMAXINFO) l_param;
            min_max_info->ptMinTrackSize.x = WINDOW_MIN_WIDTH;
            min_max_info->ptMinTrackSize.y = WINDOW_MIN_HEIGHT;

            return 0;
        } break;

        case WM_SIZE:
        {
            if (w_param == SIZE_RESTORED || w_param == SIZE_MAXIMIZED)
            {
                set_win_layout(&win_controls, LOWORD(l_param), HIWORD(l_param));
                return 0;
            }
        } break;

        case WM_COMMAND:
        {
            WORD button_identifier = LOWORD(w_param);
            WORD notification_code = HIWORD(w_param);

            assert((HWND)l_param != NULL);

            if (notification_code == BN_CLICKED)
            {
                // DEBUG
                char buf[1024];
                stbsp_snprintf(buf, sizeof(buf), "button_identifier: %hu, notification_code: %hu\n", button_identifier, notification_code);
                OutputDebugStringA(buf);
            }
        } break;

        // default:
        // {

        // } break;
    }

    return DefWindowProc(window, message, w_param, l_param);
}

BOOL CALLBACK set_font(HWND window, LPARAM font)
{
    assert(sizeof(LPARAM) == sizeof(WPARAM));
    SendMessage(window, WM_SETFONT, (WPARAM) font, (LPARAM) MAKELONG(TRUE, 0));
    return TRUE;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR command_line, int show_code)
{
    WNDCLASS main_wc =
    {
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = main_window_proc,
        .hInstance = instance,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1),
        .lpszClassName = L"MainWinClass"
    };

    RegisterClass(&main_wc);

    HWND main_window = CreateWindowEx(
        0, main_wc.lpszClassName,
        L"VideoSub",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_MIN_WIDTH, WINDOW_MIN_WIDTH,
        NULL, NULL, instance, NULL
    );

    // TODO define layout in more systematic fashion (e.g. margin_width, control_padding, control_height)
    /* TODO is there a way of finding out how much width some text of a certain font takes up? (alternative to setting hard minimum window width)
     * (Maybe: DrawTextEx with DT_CALCRECT) */

    win_controls.static_sub_lang = CreateWindow(
        L"STATIC",
        L"Subtitles language:",
        // TODO maybe SS_ENDELLIPSIS instead of just SS_LEFTNOWORDWRAP (on all static controls)?
        WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_LEFTNOWORDWRAP,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_STATIC_SUB_LANG,
        instance, NULL
    );

    win_controls.combo_sub_lang = CreateWindow(
        L"COMBOBOX",
        L"", // TODO
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, // TODO CBS_SORT?
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_COMBO_SUB_LANG,
        instance, NULL
    );

    win_controls.button_hash_search = CreateWindow(
        L"BUTTON",
        L"Search by hash",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_BTN_HASH_SEARCH,
        instance, NULL
    );

    win_controls.static_title = CreateWindow(
        L"STATIC",
        L"Title:",
        WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_LEFTNOWORDWRAP,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_STATIC_TITLE,
        instance, NULL
    );

    win_controls.edit_title = CreateWindow(
        L"EDIT",
        L"",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_EDIT_TITLE,
        instance, NULL
    );

    win_controls.button_name_search = CreateWindow(
        L"BUTTON",
        L"Search by name",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_BTN_NAME_SEARCH,
        instance, NULL
    );

    win_controls.static_season = CreateWindow(
        L"STATIC",
        L"Season:",
        WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_LEFTNOWORDWRAP,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_STATIC_TITLE,
        instance, NULL
    );

    win_controls.edit_season = CreateWindow(
        L"EDIT",
        L"",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_EDIT_TITLE,
        instance, NULL
    );

    win_controls.static_episode = CreateWindow(
        L"STATIC",
        L"Episode:",
        WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_LEFTNOWORDWRAP,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_STATIC_EPISODE,
        instance, NULL
    );

    win_controls.edit_episode = CreateWindow(
        L"EDIT",
        L"",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_EDIT_EPISODE,
        instance, NULL
    );

    win_controls.list_box_subs = CreateWindow(
        L"ListBox", // TODO could also use WC_LISTBOX from "CommCtrl.h"
        L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_LIST_BOX_SUBS,
        instance, NULL
    );

    win_controls.button_help = CreateWindow(
        L"BUTTON",
        L"Help",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_BTN_HELP,
        instance, NULL
    );

    win_controls.button_config = CreateWindow(
        L"BUTTON",
        L"Config",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_BTN_CONFIG,
        instance, NULL
    );

    win_controls.button_load = CreateWindow(
        L"BUTTON",
        L"Load",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_BTN_LOAD,
        instance, NULL
    );

    win_controls.button_save_as = CreateWindow(
        L"BUTTON",
        L"Save As",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        main_window,
        (HMENU) ID_BTN_SAVE_AS,
        instance, NULL
    );


    LOGFONT font_attributes =
    {
        .lfHeight = -12,
        .lfWeight = FW_NORMAL,
        .lfFaceName = L"MS Shell Dlg"
    };
    EnumChildWindows(main_window, (WNDENUMPROC) set_font, (LPARAM) CreateFontIndirect(&font_attributes));


    // gets rid of ugly dotted lines on button focus
    // TODO does this need to be sent each time focus is changed with tab key?
    SendMessage(main_window, WM_CHANGEUISTATE, (WPARAM) MAKELONG(UIS_SET, UISF_HIDEFOCUS), 0);

    RECT client_rect;
    GetClientRect(main_window, &client_rect);
    set_win_layout(&win_controls, client_rect.right, client_rect.bottom);

    ShowWindow(main_window, show_code);
    SetFocus(win_controls.button_hash_search); // set focus on first child (gets tabbing to work correctly)

    // TODO handle button presses

    /* TODO could be worth trying modeless dialog boxes (CreateDialog/CreateDialogIndirect)
     * Can apply fonts more easily with DS_SHELLFONT or DS_SETFONT dialog box styles.
     * Will have to use the IsDialogMessage function when handling messages.
     *
     * Can the main window be a dialog box?
     * Could also try giving in and creating a resource file containing dialog templates, as they suggest.
     * Could (if really motivated) creating the dialog template in memory and using CreateDialog
     */


    MSG message;
    while (GetMessage(&message, main_window, 0, 0) > 0)
    {
        if (IsDialogMessage(main_window, &message)) continue;

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
