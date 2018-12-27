#pragma once

#include "../Std/stdafx.h"
#include "../DX/DXTestBase.h"

class DXTestBase;

// �E�B���h�E�Y�A�v���P�[�V�������C��
class WinApp
{
public:
	static int Run(DXTestBase* pSample, HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

protected:

	// ���C���E�B���h�E�v���V�[�W��
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// �E�B���h�E�n���h���ێ�
	static HWND m_hwnd;
};