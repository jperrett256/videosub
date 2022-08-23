#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

// typedef struct control_template_t control_template_t;
// struct control_template_t
// {
// 	DLGTEMPLATE dlg_template;
// 	// TODO class name, title, creation datastuff
// };

// typedef struct main_dlg_template_t main_dlg_template_t;
// struct main_dlg_template_t
// {
// 	DLGTEMPLATE dlg_template;
// 	control_template_t control_templates[2];
// };

LRESULT CALLBACK main_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	// TODO
	return DefWindowProc(window, message, w_param, l_param);
}

BOOL CALLBACK set_font(HWND window, LPARAM font)
{
	SendMessage(window, WM_SETFONT, (WPARAM) font, (LPARAM) MAKELONG(TRUE, 0));
	return TRUE;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR command_line, int show_code)
{
	WNDCLASS main_wc =
	{
		.lpfnWndProc = main_window_proc,
		.hInstance = instance,
		.lpszClassName = L"MainWinClass"
	};

	RegisterClass(&main_wc);

	HWND main_window = CreateWindowEx(
		0, main_wc.lpszClassName,
		L"VideoSub",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, instance, NULL
	);

	HWND hash_search_button = CreateWindow(
		L"BUTTON",
		L"Search by hash",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
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
	// TODO make resizable (min dimensions approx 400x250)

	// Set font method #1
	// SendMessage(hash_search_button, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0); // (LPARAM) MAKELONG(TRUE, 0));
	// SendMessage(name_search_button, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0); // (LPARAM) MAKELONG(TRUE, 0));

	// Set font method #2
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


	/**** playing with dialog boxes ****/

	/* TODO could be worth trying modeless dialog boxes (CreateDialog/CreateDialogIndirect)
	 * Can apply fonts more easily with DS_SHELLFONT or DS_SETFONT dialog box styles.
	 * Will have to use the IsDialogMessage function when handling messages.
	 *
	 * Can the main window be a dialog box?
	 * Could try creating the dialog template in memory and using CreateDialog
	 * Could also try giving in and creating a resource file containing dialog templates, as they suggest.
	 */

	// MAIN_DLGTEMPLATE main_dlg_template =
	// {
	// 	.dlg_template =
	// 	{
	// 		.style = WS_CAPTION | WS_SYSMENU | DS_SHELLFONT,
	// 		.cdit = 1,
	// 		.x = 100,
	// 		.y = 100,
	// 		.cx = 300,
	// 		.cy = 200
	// 	},
	// 	.control_templates =
	// 	{
	// 		{
	// 			// TODO
	// 		},
	// 		{
	// 			// TODO
	// 		}
	// 	}
	// };

	/***********************************/


	MSG message;
	while (GetMessage(&message, main_window, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return 0;
}
