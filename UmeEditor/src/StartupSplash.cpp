#include "StartupSplash.h"

#ifdef _WIN32

#include <windows.h>

#include <atomic>
#include <string>
#include <thread>

namespace Ume::StartupSplash
{
	namespace
	{
		constexpr wchar_t kSplashClassName[] = L"UmeStartupSplashWindow";
		std::atomic<bool> s_Running{ false };
		std::atomic<int> s_AnimFrame{ 0 };
		std::thread s_Thread;
		HWND s_WindowHandle = nullptr;

		void DrawSplash(HDC dc, const RECT& rect)
		{
			HBRUSH background = CreateSolidBrush(RGB(28, 29, 33));
			FillRect(dc, &rect, background);
			DeleteObject(background);

			RECT topBand = rect;
			topBand.bottom = rect.top + 6;
			HBRUSH topBrush = CreateSolidBrush(RGB(45, 110, 170));
			FillRect(dc, &topBand, topBrush);
			DeleteObject(topBrush);

			SetBkMode(dc, TRANSPARENT);
			SetTextColor(dc, RGB(235, 240, 246));

			HFONT titleFont = CreateFontW(
				64, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
				FF_SWISS, L"Segoe UI"
			);
			HFONT oldFont = (HFONT)SelectObject(dc, titleFont);

			RECT titleRect = rect;
			titleRect.top += 100;
			titleRect.bottom = titleRect.top + 90;
			DrawTextW(dc, L"UME", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			SelectObject(dc, oldFont);
			DeleteObject(titleFont);

			HFONT subtitleFont = CreateFontW(
				28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
				FF_SWISS, L"Segoe UI"
			);
			oldFont = (HFONT)SelectObject(dc, subtitleFont);
			SetTextColor(dc, RGB(182, 195, 210));

			const int dotCount = (s_AnimFrame.load() % 4);
			std::wstring loadingText = L"Loading editor";
			loadingText.append(dotCount, L'.');

			RECT subtitleRect = rect;
			subtitleRect.top += 196;
			subtitleRect.bottom = subtitleRect.top + 60;
			DrawTextW(dc, loadingText.c_str(), -1, &subtitleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			SelectObject(dc, oldFont);
			DeleteObject(subtitleFont);

		}

		LRESULT CALLBACK SplashWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			switch (message)
			{
			case WM_CREATE:
				SetTimer(hwnd, 1, 140, nullptr);
				return 0;
			case WM_TIMER:
				s_AnimFrame.fetch_add(1);
				InvalidateRect(hwnd, nullptr, FALSE);
				return 0;
			case WM_NCHITTEST:
				return HTCAPTION;
			case WM_PAINT:
			{
				PAINTSTRUCT paint;
				HDC dc = BeginPaint(hwnd, &paint);
				RECT rect;
				GetClientRect(hwnd, &rect);
				DrawSplash(dc, rect);
				EndPaint(hwnd, &paint);
				return 0;
			}
			case WM_CLOSE:
				DestroyWindow(hwnd);
				return 0;
			case WM_DESTROY:
				KillTimer(hwnd, 1);
				PostQuitMessage(0);
				return 0;
			default:
				return DefWindowProcW(hwnd, message, wParam, lParam);
			}
		}

		void SplashThreadMain()
		{
			HINSTANCE instance = GetModuleHandleW(nullptr);

			HICON hAppIcon = (HICON)LoadImageW(instance, L"GLFW_ICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

			WNDCLASSW wc{};
			wc.lpfnWndProc = SplashWindowProc;
			wc.hInstance = instance;
			wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wc.hIcon = hAppIcon;
			wc.lpszClassName = kSplashClassName;
			wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			RegisterClassW(&wc);

			const int width = 760;
			const int height = 420;
			const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
			const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
			const int left = (screenWidth - width) / 2;
			const int top = (screenHeight - height) / 2;

			s_WindowHandle = CreateWindowExW(
				WS_EX_APPWINDOW | WS_EX_TOPMOST,
				kSplashClassName,
				L"UmeEditor",
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
				left,
				top,
				width,
				height,
				nullptr,
				nullptr,
				instance,
				nullptr
			);

			if (!s_WindowHandle)
			{
				s_Running.store(false);
				return;
			}

			SendMessageW(s_WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hAppIcon);
			HICON hSmallIcon = (HICON)LoadImageW(instance, L"GLFW_ICON", IMAGE_ICON, 16, 16, LR_SHARED);
			SendMessageW(s_WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);

			SetWindowPos(s_WindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			ShowWindow(s_WindowHandle, SW_SHOW);
			UpdateWindow(s_WindowHandle);

			MSG msg;
			while (GetMessageW(&msg, nullptr, 0, 0) > 0)
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}

			s_WindowHandle = nullptr;
			UnregisterClassW(kSplashClassName, instance);
			s_Running.store(false);
		}
	}

	void Start()
	{
		bool expected = false;
		if (!s_Running.compare_exchange_strong(expected, true))
			return;

		s_AnimFrame.store(0);
		s_Thread = std::thread(SplashThreadMain);
	}

	void Stop()
	{
		if (!s_Running.load())
			return;

		HWND hwnd = s_WindowHandle;
		if (hwnd)
			PostMessageW(hwnd, WM_CLOSE, 0, 0);

		if (s_Thread.joinable())
			s_Thread.join();
	}

	bool IsRunning()
	{
		return s_Running.load();
	}
}

#else

namespace Ume::StartupSplash
{
	void Start() {}
	void Stop() {}
	bool IsRunning() { return false; }
}

#endif
