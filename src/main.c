#define UNICODE

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include <windows.h>
#include <wininet.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define JDP_COMMON_IMPLEMENTATION
#include "common.h"

#define WINDOW_MIN_WIDTH    420
#define WINDOW_MIN_HEIGHT   300

/* NOTE control IDs are typically assigned at values starting at 100,
 * as the lower values are taken. This is never explicitly stated in
 * any MSDN documentation I found, but I encountered strange issues
 * when using IDs that were too low. (When I hit the escape key, it
 * triggered a WM_COMMAND message to be sent to my window procedure
 * that was indistiguishable from pressing a button with ID set to 2.
 * Even the lParam value indicated it came from the control I made!)
 *
 * Source is an article by Raymond Chen:
 * https://devblogs.microsoft.com/oldnewthing/20041214-00/?p=37013
 */
enum control_id_t {
    ID_BTN_HASH_SEARCH = 100,
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

// #define CONFIG_STR_LIMIT 512 // TODO can do sth dynamic later if you want

// typedef struct config_t config_t;
// struct config_t
// {
//     char username[CONFIG_STR_LIMIT];
//     char password[CONFIG_STR_LIMIT];
// };

global_variable win_controls_t win_controls = {0};

HINTERNET api_get_handle()
{
    char * user_agent = "videosub 0.1.0";

    HINTERNET internet_open_handle = InternetOpenA(
        user_agent,
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0 // TODO should use INTERNET_FLAG_ASYNC later
    );

    if (!internet_open_handle)
    {
        // TODO handle with error system
        assert(0);
        DWORD error_code = GetLastError();
        return NULL;
    }

    HINTERNET internet_connect_handle = InternetConnectA(
        internet_open_handle,
        "api.opensubtitles.com",
#ifdef VSUB_DEBUG_NETWORK
        INTERNET_DEFAULT_HTTP_PORT,
#else
        INTERNET_DEFAULT_HTTPS_PORT,
#endif
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0 // TODO does dwContext need to be set to sth for async requests?
    );

    if (!internet_connect_handle)
    {
        // TODO handle with error system
        assert(0);
        DWORD error_code = GetLastError();
        return NULL;
    }

    return internet_connect_handle;
}

static string_t string_from_unicode(arena_t * arena, wchar_t * str)
{
    // TODO could use WC_ERR_INVALID_CHARS to detect invalid code points
    int chars_required = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    char * buffer = arena_push_array(arena, char, chars_required);
    int chars_written = WideCharToMultiByte(CP_UTF8, 0, str, -1, buffer, chars_required, NULL, NULL);
    assert(chars_written == chars_required);

    string_t result = {0};
    result.ptr = buffer;
    result.size = chars_required - 1; // exclude null terminator
    assert(result.ptr[result.size] == '\0');

    return result;
}

// TODO wstring_t type?
static wchar_t * string_to_unicode(arena_t * arena, string_t str)
{
    assert(str.size >= 0 && str.size <= INT_MAX);
    int str_size = (int) str.size;

    // TODO could use MB_ERR_INVALID_CHARS to detect invalid input characters
    int chars_required = MultiByteToWideChar(CP_UTF8, 0, str.ptr, str_size, NULL, 0);
    assert(chars_required < INT_MAX);
    wchar_t * buffer = arena_push_array(arena, wchar_t, chars_required + 1);
    int chars_written = MultiByteToWideChar(CP_UTF8, 0, str.ptr, str_size, buffer, chars_required);
    assert(chars_written == chars_required);

    // NOTE if input to MultiByteToWideChar isn't null terminated, neither is output from MultiByteToWideChar
    // so we make sure to add it manually
    buffer[chars_required] = '\0';

    return buffer;
}

static string_t arena_sprintf(arena_t * arena, char * fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    // TODO somewhat inefficient, effectively prints string twice
    // TODO just create fixed size error buffer?

    // NOTE only works for strings < 2 GiB
    int str_len = stbsp_vsnprintf(NULL, 0, fmt, va);
    assert(str_len >= 0);

    va_end(va);
    va_start(va, fmt);

    char * str_buf = arena_push(arena, str_len + 1);
    int bytes_written = stbsp_vsnprintf(str_buf, str_len + 1, fmt, va);
    assert(bytes_written == str_len);

    va_end(va);

    // TODO string create function?
    string_t result = { str_buf, str_len };
    return result;
}

typedef struct hash_result_t hash_result_t;
struct hash_result_t
{
    union
    {
        string_t error; // TODO null terminated string type? cstring_t?
        string_t hash;
    } as;
    bool success;
};

static hash_result_t hash_error_result(string_t error)
{
    hash_result_t result = {0};
    result.success = false;
    result.as.error = error;

    return result;
}

hash_result_t hash_get(arena_t * arena, wchar_t * filename)
{
    // NOTE method here is loosely based on public domain code from:
    // https://trac.opensubtitles.org/projects/opensubtitles/wiki/HashSourceCodes

    int fseek_success;

    FILE * file = _wfopen(filename, L"rb");
    if (!file)
    {
        string_t filename_utf8 = string_from_unicode(arena, filename);
        return hash_error_result(arena_sprintf(arena,
            "ERROR: failed to open \"%.*s\" for calculating hash.\n", string_varg(filename_utf8)));
    }

    fseek_success = fseek(file, 0, SEEK_END);
    assert(fseek_success == 0);

    i64 file_size = _ftelli64(file);
    assert(file_size >= 0);

    fseek_success = fseek(file, 0, SEEK_SET);
    assert(fseek_success == 0);

    if (file_size < KILOBYTES(64))
    {
        return hash_error_result(arena_sprintf(arena,
            "ERROR: file is less than 64 KiB, hash unavailable.\n"));
    }

#define MOVIEHASH_BUFFER_SIZE (KILOBYTES(64)/sizeof(u64))

    // NOTE assuming system is little endian

    u64 hash = file_size;

    u64 buffer[MOVIEHASH_BUFFER_SIZE];
    size_t elements_read;

    elements_read = fread(buffer, sizeof(u64), MOVIEHASH_BUFFER_SIZE, file);
    assert(elements_read == MOVIEHASH_BUFFER_SIZE);

    for (i64 i = 0; i < MOVIEHASH_BUFFER_SIZE; i++)
        hash += buffer[i];

    fseek_success = fseek(file, -KILOBYTES(64), SEEK_END);
    assert(fseek_success == 0);

    elements_read = fread(buffer, sizeof(u64), MOVIEHASH_BUFFER_SIZE, file);
    assert(elements_read == MOVIEHASH_BUFFER_SIZE);

    for (i64 i = 0; i < MOVIEHASH_BUFFER_SIZE; i++)
        hash += buffer[i];

    string_t hash_str = arena_sprintf(arena, "%016llx", hash);

    hash_result_t result = {0};
    result.success = true;
    result.as.hash = hash_str;

#undef MOVIEHASH_BUFFER_SIZE

    return result;
}

/****************** api_request ******************
 * internet_handle: handle from api_get_handle()
 * method: "GET", "POST", etc. (NULL -> "GET")
 * path: path to send request to
 * data: utf-8 encoded payload data (can be NULL)
 * data_len: size of that payload data, in bytes
 *************************************************/
void api_request(HINTERNET internet_handle, char * method, char * path, void * data, u32 data_len)
{
    char * api_key = "Y4NKks8ItQc9ZunAMUVlS1iUmVdk81Pk";

    char * accept_types[] = { "*/*", NULL };

    HINTERNET internet_request_handle = HttpOpenRequestA(
        internet_handle,
        method,
        path,
        NULL,
        NULL,
        accept_types,
#ifdef VSUB_DEBUG_NETWORK
        0,
#else
        INTERNET_FLAG_SECURE,
#endif
        0 // TODO does dwContext need to be set to sth for async requests
    );

    if (!internet_request_handle)
    {
        // TODO handle with error system
        assert(0);
        DWORD error_code = GetLastError();
        return;
    }

    char headers[2048];
    int bytes_written = 0;
    bytes_written += stbsp_snprintf(
        &headers[bytes_written], sizeof(headers) - bytes_written,
        "Api-Key: %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n",
        api_key
    );
    /* NOTE testing with wireshark shows that it does not matter if you
     * add \r\n to the final header (if not there, wininet adds it for you)
     */

    assert(bytes_written < sizeof(headers));

    BOOL success;

    success = HttpSendRequestA(
        internet_request_handle,
        headers,
        -1,
        data,
        data_len
    );

    if (!success) // HttpSendRequestA
    {
        // TODO handle with error system
        assert(0);
    }

    char status_code_buf[6];
    DWORD status_code_len = sizeof(status_code_buf) - 1;

    success = HttpQueryInfoA(
        internet_request_handle,
        HTTP_QUERY_STATUS_CODE, // TODO can use HTTP_QUERY_RAW_HEADERS to get all headers
        status_code_buf,
        &status_code_len,
        0
    );

    assert(success != ERROR_INSUFFICIENT_BUFFER);

    if (!success) // HttpQueryInfoA
    {
        // TODO handle, use GetLastError
        assert(0);
    }

    assert(status_code_len < sizeof(status_code_buf));
    status_code_buf[status_code_len] = '\0';

    // TODO need to write strcmp function
    OutputDebugStringA(status_code_buf);
    OutputDebugStringA("\n");

    // NOTE InternetQueryDataAvailable returns the number of bytes _immediately_ available for reading, not the total response size

    // TODO move outside this function
    while (1)
    {
        // TODO note that the response is utf8 (will need converting before handling properly)
        char response_buffer[64];
        DWORD response_len;

        success = InternetReadFile(
            internet_request_handle,
            response_buffer,
            sizeof(response_buffer) - 1,
            &response_len
        );

        if (!success) // InternetReadFile
        {
            // TODO handle
            // TODO would InternetGetLastResponseInfoA be appropriate??
            assert(0);
        }

        if (response_len == 0) break;

        assert(response_len < sizeof(response_buffer));
        response_buffer[response_len] = 0;

        OutputDebugStringA(response_buffer);
        OutputDebugStringA("\n");
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
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;

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
            WORD control_identifier = LOWORD(w_param);
            WORD notification_code = HIWORD(w_param);

            HWND control_handle = (HWND)l_param;

            if (notification_code == BN_CLICKED
                && control_identifier == IDCANCEL
                && control_handle == NULL)
            {
                PostQuitMessage(0); // DEBUG code - escape quits
                break;
            }

            if (control_handle != NULL)
            {
                switch (notification_code)
                {
                    case BN_CLICKED:
                    {
                        dbg_print("button identifier: %hu, notification code: %hu, control handle: %p\n",
                            control_identifier, notification_code, control_handle);
                        // TODO
                    } break;
                    case EN_UPDATE:
                    {
                        // TODO proper scratch arenas
                        arena_t scratch = arena_alloc(MEGABYTES(8));

                        dbg_print("edit control identifier: %hu, notification code: %hu, control handle: %p\n",
                            control_identifier, notification_code, control_handle);

                        // TODO how are we able to send this within the window procedure?
                        LRESULT result;
                        result = SendMessage(control_handle, WM_GETTEXTLENGTH, 0, 0);
                        assert(result >= 0 && result < UINT32_MAX);
                        u32 length = (u32) result;

                        dbg_print("length: %d\n", length);
                        u32 buffer_length = length + 1;
                        wchar_t * buffer = arena_push_array(&scratch, wchar_t, buffer_length);

                        result = SendMessage(control_handle, WM_GETTEXT, buffer_length, (LPARAM) buffer);
                        assert(result >= 0 && result <= UINT32_MAX);
                        assert((u32) result == length);

                        // TODO make a dbg_print_utf8 function
                        dbg_print("text: ");
                        OutputDebugStringW(buffer);
                        dbg_print("\n");

                        arena_free(&scratch);
                    } break;
                }
            }
        } break;
    }

    return DefWindowProc(window, message, w_param, l_param);
}

BOOL CALLBACK set_font(HWND window, LPARAM font)
{
    assert(sizeof(LPARAM) == sizeof(WPARAM));
    SendMessage(window, WM_SETFONT, (WPARAM) font, (LPARAM) MAKELONG(TRUE, 0));
    return TRUE;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line_ascii, int show_code)
{
    int argc;
    LPWSTR command_line = GetCommandLineW();
    wchar_t ** argv = CommandLineToArgvW(command_line, &argc);

    // TODO support for starting without any filenames?
    // TODO support for multiple files at once?
    if (argc != 2)
    {
        assert(argc >= 1);
        // TODO message box instead
        dbg_print("ERROR: expected single argument, got %d.\n", argc);

        dbg_wprint(L"Usage: %s <file path>\n", argv[0]);
        ExitProcess(1);
    }

    wchar_t * media_filename = argv[1];

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


    // TODO use api to request the list, maybe also cache it
    string_t language_options[] =
    {
        string_lit("English"),
        string_lit("Chinese (simplified)"),
        string_lit("Japanese")
    };
    u32 num_language_options = array_count(language_options);

    for (i64 i = 0; i < num_language_options; i++)
    {
        // TODO add arena temps so that arenas can be shrunk again, instead of creating and freeing arenas all the time
        arena_t scratch = arena_alloc(MEGABYTES(8));

        string_t option = language_options[i];
        wchar_t * option_wstr = string_to_unicode(&scratch, option);

        SendMessage(win_controls.combo_sub_lang, CB_ADDSTRING, 0, (LPARAM) option_wstr);

        arena_free(&scratch);
    }

    u32 selected_index = 0;
    assert(selected_index < num_language_options);
    SendMessage(win_controls.combo_sub_lang, CB_SETCURSEL, selected_index, (LPARAM) 0);


    arena_t arena = arena_alloc(MEGABYTES(4));
    hash_result_t hash_result = hash_get(&arena, media_filename);
    if (hash_result.success)
    {
        dbg_print("hash: %.*s\n", string_varg(hash_result.as.hash));
    }
    else
    {
        // wchar_t * error_wstr = string_to_unicode(&arena, hash_result.as.error);
        // dbg_wprint(L"%s\n", error_wstr);
        dbg_print_utf8("%.*s\n", string_varg(hash_result.as.error));
    }


    // TODO handle button presses

    HINTERNET internet_handle = api_get_handle();

    // DEBUG
    {
        /* TODO need function to sanitize and correctly format data in query params
           We do this by:
           1. convert string to utf-8
           2. then we percent encode:
              - non-ASCII characters, i.e. 80-FF (this should allow for utf-8 support)
              - all reserved keywords from RFC 3986
                - reserved: ':', '/', '?', '#', '[', ']', '@', '!', '$', '&' '\'' '(', ')', '*', '+', ',', ';', '='
                  - potentially could get away with more limited set: '&', '+', '#', '=', ';' (maybe '[', ']' as well)
                - '%', since it is used for escaping
              - Characters mentioned by RFC 1738 (despite being obsolete)
                - ASCII control characters (00-1F, 7F)
                - "Unsafe characters"?
           3. replace spaces with '+' (can use "%20", but OpenSubtitles asks specifically for '+' over "%20")
        */

        // api_request(internet_handle, "GET", "/api/v1/infos/languages", NULL, 0);

        // api_request(internet_handle, "GET", "/api/v1/subtitles?query=bee+movie", NULL, 0);

        // string_t request_data = string_lit("{\"file_id\": 123}");
    }

    MSG message;
    while (GetMessage(&message, NULL, 0, 0) > 0)
    {
        if (IsDialogMessage(main_window, &message)) continue;

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}
