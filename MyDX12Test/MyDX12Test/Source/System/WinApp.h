#pragma once

#include "../DX/DXTestBase.h"

// ウィンドウズアプリケーションメイン
class WinApp
{
public:
	static int Run(/*DXSample* pSample, */HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

protected:

	// メインウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// ウィンドウハンドル保持
	static HWND m_hwnd;
};