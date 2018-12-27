#pragma once

#include "../Std/stdafx.h"
#include "../DX/DXTestBase.h"

class DXTestBase;

// ウィンドウズアプリケーションメイン
class WinApp
{
public:
	static int Run(DXTestBase* pSample, HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

protected:

	// メインウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// ウィンドウハンドル保持
	static HWND m_hwnd;
};