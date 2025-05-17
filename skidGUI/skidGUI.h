#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <iostream>
#include <list>
#include <memory>
#include <map>
#include <string>
#include <string_view>
#include <iterator>
using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

void print_map(std::string_view comment, const std::map<int, std::list<int>>& m)
{
	std::cout << comment;
	for (const auto& [key, value] : m) {
		std::cout << '[' << key << "] = ";
		for (const auto& v : value) {
			std::cout << v << ";";
		}
	}

	std::cout << '\n';
}

bool IsPointInsideRect(POINT point, int x, int y, int w, int h) {
	return (point.x >= x && point.x < x + w &&
		point.y >= y && point.y < y + h);
}

POINT GetMousePositionInWindow(HWND hwnd) {
	POINT pt;
	GetCursorPos(&pt);            // Get global screen coordinates
	ScreenToClient(hwnd, &pt);    // Convert to client-area coordinates
	return pt;
}

// __ Helper Functions

void FillRectangle(Graphics* graphics, Color color, int x, int y, int w, int h) {
	SolidBrush brush(color);
	graphics->FillRectangle(&brush, x, y, w, h);
}

void DrawRectangle(Graphics* graphics, Color color, int x, int y, int w, int h) {
	Pen pen(color);
	graphics->DrawRectangle(&pen, x, y, w, h);
}

// __ Drawing Functions

enum ButtonEventType {
	NoEvent, OnMouseEnter, OnMouseLeave, OnClick
};

enum WidgetLayoutType {
	None, HorizontalFill
};

struct ButtonEvent {
	ButtonEventType type = NoEvent;
	int mouseX = 0;
	int mouseY = 0;
};

class Button;

typedef void (*ButtonCallback)(Button* button, ButtonEvent event);

// __ Event Structs and Callbacks

class Widget {
public:
	int id = -1;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	virtual void draw(Graphics* graphics) = 0;
	virtual void physics(HWND hwnd) = 0;
	virtual ~Widget() {}
};

class Button : public Widget {
public:
	const WCHAR* label = L"";
	const WCHAR* fontName = L"Arial";
	int fontSize = 24;
	Color fontColor = 0;
	Color color = 0;
	Color borderColor = 0;
	int borderSize = 0;
	ButtonCallback clbk = NULL;
	bool mouseInside  = false;
	bool mousePressed = false;

	void draw(Graphics* graphics) override {
		FillRectangle(graphics, color, x, y, width, height);

		for (int i = 0; i < borderSize; i++) {
			DrawRectangle(graphics, borderColor, x-i, y-i, width+i*2, height+i*2);
		}

		SolidBrush fontBrush(fontColor);

		FontFamily fontFamily(fontName);
		Font font(&fontFamily, (REAL)fontSize, FontStyleRegular, UnitPixel);

		RectF layoutRect;
		PointF origin(0.0f, 0.0f);
		graphics->MeasureString(label, -1, &font, origin, &layoutRect);

		PointF pt(x + width / 2 - layoutRect.Width / 2, y + height / 2 - layoutRect.Height / 2);
		graphics->DrawString(label, -1, &font, pt, &fontBrush);
	}

	void CallEvent(ButtonEventType type, HWND hwnd) {
		if (clbk == NULL)
			return;

		POINT mouse_pos = GetMousePositionInWindow(hwnd);

		ButtonEvent e;
		e.mouseX = mouse_pos.x;
		e.mouseY = mouse_pos.y;
		e.type = type;

		clbk(this, e);
	}

	void physics(HWND hwnd) override {
		POINT mouse_pos = GetMousePositionInWindow(hwnd);

		if (mouseInside && !IsPointInsideRect(mouse_pos, x, y, width, height)) {
			mouseInside = false;
			CallEvent(OnMouseLeave, hwnd);
		}
		else if (!mouseInside && IsPointInsideRect(mouse_pos, x, y, width, height)) {
			mouseInside = true;
			CallEvent(OnMouseEnter, hwnd);
		}

		if (mouseInside && GetAsyncKeyState(MK_LBUTTON) < 0 && !mousePressed) {
			mousePressed = true;
			CallEvent(OnClick, hwnd);
		}
		else if (GetAsyncKeyState(MK_LBUTTON) >= 0 && mouseInside) {
			mousePressed = false;
		}

		mouseInside = IsPointInsideRect(mouse_pos, x, y, width, height);
	}
};

// __ Widget Classes

class UI {
private:
	static void OnUpdate(HDC hdc);
	static void OnPhysicsUpdate();
	static ULONG_PTR gdiplusToken;
	static HWND window_hwnd;
	static WNDCLASS window_class;
	static bool running;
	static bool debug_console;
	static int width;
	static int height;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	static std::list<std::shared_ptr<Widget>> widgets;
	static std::map<int, std::list<std::shared_ptr<Widget>>> layoutGroups;
	static std::map<int, WidgetLayoutType> layoutTypes;
public:
	static Color backgroundColor;
	static void Init(HINSTANCE instance, const wchar_t* title, int width, int height);
	static void Run();
	static void EnableDebugConsole();
	static void ReloadWindow();
	static void SetLayoutType(int layoutGroup, WidgetLayoutType type);
	template <class T> static void AddWidget(T button, int layoutGroup = 0);
};

std::list<std::shared_ptr<Widget>> UI::widgets{};
std::map<int, std::list<std::shared_ptr<Widget>>> UI::layoutGroups{};
std::map<int, WidgetLayoutType> UI::layoutTypes{};
ULONG_PTR UI::gdiplusToken = NULL;
HWND UI::window_hwnd = NULL;
WNDCLASS UI::window_class{};
bool UI::running = false;
bool UI::debug_console = false;
int UI::width = 0;
int UI::height = 0;
Color UI::backgroundColor = 0;

void moveWidget(std::list<std::shared_ptr<Widget>>& widgets, size_t from, size_t to) {
	if (from >= widgets.size() || to >= widgets.size() || from == to)
		return;

	auto fromIt = widgets.begin();
	std::advance(fromIt, from);

	auto toIt = widgets.begin();
	std::advance(toIt, to);

	widgets.splice(toIt, widgets, fromIt);
}

inline void UI::ReloadWindow()
{
	InvalidateRect(UI::window_hwnd, NULL, TRUE);
	UpdateWindow(UI::window_hwnd);
}

void UI::EnableDebugConsole() {
	AllocConsole();

	FILE* file{ nullptr };
	freopen_s(&file, "CONIN$", "r", stdin);
	freopen_s(&file, "CONOUT$", "w", stdout);
	freopen_s(&file, "CONOUT$", "w", stderr);

	UI::debug_console = true;
}

inline void UI::OnUpdate(HDC hdc) {
	int width = UI::width;
	int height = UI::height;

	HDC hdcMem = CreateCompatibleDC(hdc);

	HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

	Graphics graphics(hdcMem);

	FillRectangle(&graphics, UI::backgroundColor, 0, 0, width, height);

	for (const auto& [group, index_list] : layoutGroups) {
		for (const auto& widget : index_list) {
			widget->draw(&graphics);
		}
	}

	BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

	SelectObject(hdcMem, hbmOld);
	DeleteObject(hbmMem);
	DeleteDC(hdcMem);

	ReleaseDC(UI::window_hwnd, hdc);	
}

inline void UI::OnPhysicsUpdate() {
	for (const auto& [group, index_list] : layoutGroups) {
		int max = (int)index_list.size();
		WidgetLayoutType type = layoutTypes[group];
		int curr = 0;
		for (const auto& widget : index_list) {
			switch (type) {
			case HorizontalFill:
				widget->x = 10 + (width / max) * curr;
				widget->width = width / max - 20;
			}

			widget->physics(UI::window_hwnd);

			curr++;
		}
	}
}

template<class T>
void UI::AddWidget(T widget, int layoutGroup) {
	widget.id = (int)UI::layoutGroups[layoutGroup].size();
	UI::layoutGroups[layoutGroup].push_back(std::make_shared<T>(widget));
}

inline void UI::SetLayoutType(int layoutGroup, WidgetLayoutType type)
{
	layoutTypes[layoutGroup] = type;
}

// __ Widget Functions


LRESULT CALLBACK UI::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HDC          hdc;
	PAINTSTRUCT  ps;

	OnPhysicsUpdate();

	switch (message)
	{
	case WM_SIZE:
		RECT window_size;
		GetClientRect(hwnd, &window_size);

		UI::width = window_size.right - window_size.left;
		UI::height = window_size.bottom - window_size.top;
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);

		UI::OnUpdate(hdc);

		EndPaint(hwnd, &ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

void UI::Init(HINSTANCE instance, const wchar_t* title, int width, int height) {
	WNDCLASS wndClass{};

	GdiplusStartupInput gdiplusStartupInput;

	GdiplusStartup(&UI::gdiplusToken, &gdiplusStartupInput, NULL);

	UI::width = width;
	UI::height = height;

	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = instance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = TEXT("skidGUI WinClass");

	RegisterClass(&wndClass);

	const HWND window = CreateWindow(
		wndClass.lpszClassName,
		title,
		WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
		0,
		0,
		width,
		height,
		nullptr,
		nullptr,
		wndClass.hInstance,
		nullptr
	);

	UI::window_class = wndClass;
	UI::window_hwnd = window;
}

void UI::Run() {
	ShowWindow(UI::window_hwnd, SW_SHOWNORMAL);
	UpdateWindow(UI::window_hwnd);

	UI::running = true;
	while (UI::running) {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				UI::running = false;
		}

		if (!UI::running)
			break;
	}

	GdiplusShutdown(UI::gdiplusToken);

	if (UI::debug_console)
		FreeConsole();

	DestroyWindow(UI::window_hwnd);
	UnregisterClassW(UI::window_class.lpszClassName, UI::window_class.hInstance);
}