#include "WinApp.h"

HWND WinApp::m_hwnd = nullptr;

int WinApp::Run(DXTestBase* pSample, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
	// �R�}���h���C�������擾
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	// �R�}���h���C���������p�[�X
	pSample->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	// �E�B���h�E�N���X�\���̂̐ݒ�
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;			// �X�^�C���̎w��FCS_VREDRAW(���������ɃE�B���h�E���ύX���ꂽ��ĕ`��)��CS_HREDRAW(���������ɃE�B���h�E���ύX���ꂽ��ĕ`��)�̍��킹��
	windowClass.lpfnWndProc = WindowProc;					// �g�p����E�B���h�E�v���V�[�W���̃|�C���^���w��
	windowClass.cbClsExtra = 0;								// ���̃E�B���h�E�N���X�ׂ̈̕⏕�̈�̃T�C�Y�B���g�p�Ȃ�0
	windowClass.cbWndExtra = 0;								// ���̃E�B���h�E�ׂ̈̕⏕�̈�̃T�C�Y�B���g�p�Ȃ�0
	windowClass.hInstance = hInstance;						// �A�v���P�[�V�����n���h����ݒ�
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);	// �A�C�R���̃n���h�����w��
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);		// �J�[�\���̃n���h�����w��
	windowClass.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);	// �w�i�̓h��Ԃ��u���V�n���h�����w��
	windowClass.lpszMenuName = NULL;						// ���j���[���\�[�X�̖��O���w��
	windowClass.lpszClassName = L"DXSampleClass";			// �E�B���h�E�N���X�����w��

	// �E�B���h�E�N���X�̓o�^�F��������̃f�[�^�x�[�X�Ɍ^��o�^���邾���ŁA�܂��E�B���h�E�̎��͖̂����B
	if(!RegisterClassEx(&windowClass))
	{
		// �E�B���h�E�̓o�^�Ɏ��s
		return 0;
	}

	// �E�B���h�E�T�C�Y�ݒ�
	RECT windowRect = { 0, 0, static_cast<LONG>(pSample->GetWidth()), static_cast<LONG>(pSample->GetHeight()) };
	// �E�B���h�E�̘g�������l�������T�C�Y���v�Z
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);	

	// �E�B���h�E�̐���
	m_hwnd = CreateWindow(
		windowClass.lpszClassName,				// �o�^����Ă���N���X��
		pSample->GetTitle(),					// �E�B���h�E��
		WS_OVERLAPPEDWINDOW,					// �E�B���h�E�X�^�C��
		CW_USEDEFAULT,							// �E�B���h�E�������̈ʒu
		CW_USEDEFAULT,							// �E�B���h�E�c�����̈ʒu
		windowRect.right - windowRect.left,		// �E�B���h�E�̕�
		windowRect.bottom - windowRect.top,		// �E�B���h�E�̍���
		nullptr,								// �e�A�I�[�i�[�E�B���h�E�̃n���h��
		nullptr,								// ���j���[�A�q�E�B���h�E�̃n���h��
		hInstance,								// �A�v���P�[�V�����C���X�^���X�̃n���h��
		pSample);								// �E�B���h�E�쐬�f�[�^

	if (m_hwnd == NULL) 
	{
		// �E�B���h�E�̐����Ɏ��s
		return 0;
	}

	// DX������
	pSample->OnInit();

	// �E�B���h�E�̕\��
	ShowWindow(m_hwnd, nCmdShow);

	// ���b�Z�[�W���[�v
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// ���b�Z�[�W�L���[�̎擾
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

		// �e�X�g���b�Z�[�W�{�b�N�X
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

	// �f�t�H���g�E�B���h�E�v���V�[�W���F��L�܂ł�Switch���ŏ������Ȃ��������b�Z�[�W�͂����ŏ���
	return DefWindowProc(hWnd, message, wParam, lParam);
}
