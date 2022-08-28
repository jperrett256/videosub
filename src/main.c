#define UNICODE

#include <assert.h>
#include <windows.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#include "common.h"

#define ID_BTN_HASH_SEARCH 1
#define ID_BTN_NAME_SEARCH 2

LRESULT CALLBACK main_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	switch (message)
	{
		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO min_max_info = (LPMINMAXINFO) l_param;
			min_max_info->ptMinTrackSize.x = 400;
			min_max_info->ptMinTrackSize.y = 250;
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
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
		NULL, NULL, instance, NULL
	);

	HWND hash_search_button = CreateWindow(
		L"BUTTON",
		L"Search by hash",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, 10, 84, 22,
		main_window,
		(HMENU) ID_BTN_HASH_SEARCH,
		instance, NULL
	);

	HWND name_search_button = CreateWindow(
		L"BUTTON",
		L"Search by name",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
		10, 40, 84, 22,
		main_window,
		(HMENU) ID_BTN_NAME_SEARCH,
		instance, NULL
	);

	EnumChildWindows(main_window, (WNDENUMPROC) set_font, (LPARAM) GetStockObject(DEFAULT_GUI_FONT));


	ShowWindow(main_window, show_code);
	SetFocus(hash_search_button); // set focus on first child (TODO will need updating when layout is updated)
	// TODO do all applications really have to explicitly set focus on the first element for things to behave correctly?

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
