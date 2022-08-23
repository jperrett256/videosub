#define UNICODE

#include <assert.h>
#include <windows.h>

LRESULT CALLBACK main_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	// TODO
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
		// NOTE COLOR_BTNFACE is supposedly not supported on Windows 10 and up (see GetSysColor MSDN API reference), but empirically it works fine
		.hbrBackground = (HBRUSH) GetSysColorBrush(COLOR_BTNFACE),
		// .hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1), // TODO use this simpler solution (also API reference doesn't say "not supported")
		.lpszClassName = L"MainWinClass"
	};

	RegisterClass(&main_wc);

	HWND main_window = CreateWindowEx(
		0, main_wc.lpszClassName,
		L"VideoSub",
		WS_OVERLAPPEDWINDOW,
		// TODO set minimum dimensions (approx 400x250)
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
		NULL, NULL, instance, NULL
	);

	HWND hash_search_button = CreateWindow(
		L"BUTTON",
		L"Search by hash",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, 10, 100, 20, // TODO x y width height
		main_window, NULL, instance, NULL
	);

	HWND name_search_button = CreateWindow(
		L"BUTTON",
		L"Search by name",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
		10, 40, 100, 20, // TODO x y width height
		main_window, NULL, instance, NULL
	);

	EnumChildWindows(main_window, (WNDENUMPROC) set_font, (LPARAM) GetStockObject(DEFAULT_GUI_FONT));

	// // recommended method, but I don't like the font
	// NONCLIENTMETRICS metrics =
	// {
	// 	.cbSize = sizeof(NONCLIENTMETRICS)
	// };
	// SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
	// EnumChildWindows(main_window, (WNDENUMPROC) set_font, (LPARAM) CreateFontIndirect(&metrics.lfCaptionFont));

	ShowWindow(main_window, show_code);

	// TODO tab through controls? Why doesn't TABSTOP solve everything?
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
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return 0;
}
