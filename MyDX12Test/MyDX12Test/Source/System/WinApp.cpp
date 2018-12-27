#include "WinApp.h"

HWND WinApp::m_hwnd = nullptr;

int WinApp::Run(DXTestBase* pSample, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
	// コマンドライン引数取得
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	// コマンドライン引数をパース
	pSample->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	// ウィンドウクラス構造体の設定
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;			// スタイルの指定：CS_VREDRAW(垂直方向にウィンドウが変更されたら再描画)とCS_HREDRAW(水平方向にウィンドウが変更されたら再描画)の合わせる
	windowClass.lpfnWndProc = WindowProc;					// 使用するウィンドウプロシージャのポインタを指定
	windowClass.cbClsExtra = 0;								// 次のウィンドウクラスの為の補助領域のサイズ。未使用なら0
	windowClass.cbWndExtra = 0;								// 次のウィンドウの為の補助領域のサイズ。未使用なら0
	windowClass.hInstance = hInstance;						// アプリケーションハンドルを設定
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);	// アイコンのハンドルを指定
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);		// カーソルのハンドルを指定
	windowClass.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);	// 背景の塗りつぶすブラシハンドルを指定
	windowClass.lpszMenuName = NULL;						// メニューリソースの名前を指定
	windowClass.lpszClassName = L"DXSampleClass";			// ウィンドウクラス名を指定

	// ウィンドウクラスの登録：メモリ上のデータベースに型を登録するだけで、まだウィンドウの実体は無い。
	if(!RegisterClassEx(&windowClass))
	{
		// ウィンドウの登録に失敗
		return 0;
	}

	// ウィンドウサイズ設定
	RECT windowRect = { 0, 0, static_cast<LONG>(pSample->GetWidth()), static_cast<LONG>(pSample->GetHeight()) };
	// ウィンドウの枠部分を考慮したサイズを計算
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);	

	// ウィンドウの生成
	m_hwnd = CreateWindow(
		windowClass.lpszClassName,				// 登録されているクラス名
		pSample->GetTitle(),					// ウィンドウ名
		WS_OVERLAPPEDWINDOW,					// ウィンドウスタイル
		CW_USEDEFAULT,							// ウィンドウ横方向の位置
		CW_USEDEFAULT,							// ウィンドウ縦方向の位置
		windowRect.right - windowRect.left,		// ウィンドウの幅
		windowRect.bottom - windowRect.top,		// ウィンドウの高さ
		nullptr,								// 親、オーナーウィンドウのハンドル
		nullptr,								// メニュー、子ウィンドウのハンドル
		hInstance,								// アプリケーションインスタンスのハンドル
		pSample);								// ウィンドウ作成データ

	if (m_hwnd == NULL) 
	{
		// ウィンドウの生成に失敗
		return 0;
	}

	// DX初期化
	pSample->OnInit();

	// ウィンドウの表示
	ShowWindow(m_hwnd, nCmdShow);

	// メッセージループ
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// メッセージキューの取得
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	pSample->OnDestroy();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

// Main message handler for the sample.
LRESULT CALLBACK WinApp::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DXTestBase* pSample = reinterpret_cast<DXTestBase*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
	{
		// Save the DXSample* passed in to CreateWindow.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	return 0;

	case WM_KEYDOWN:
		if (pSample)
		{
			pSample->OnKeyDown(static_cast<UINT8>(wParam));
		}

		// テストメッセージボックス
		return 0;

	case WM_KEYUP:
		if (pSample)
		{
			pSample->OnKeyUp(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_PAINT:
		if (pSample)
		{
			pSample->OnUpdate();
			pSample->OnRender();
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// デフォルトウィンドウプロシージャ：上記までのSwitch文で処理しなかったメッセージはここで処理
	return DefWindowProc(hWnd, message, wParam, lParam);
}
