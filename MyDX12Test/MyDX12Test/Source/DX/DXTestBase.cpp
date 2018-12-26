

#include "../Std/stdafx.h"
#include "DXTestBase.h"

using namespace Microsoft::WRL;

DXTestBase::DXTestBase(UINT width, UINT height, std::wstring name) :
	m_width(width),
	m_height(height),
	m_title(name),
	m_useWarpDevice(false)
{
	WCHAR assetsPath[512];
	GetAssetsPath(assetsPath, _countof(assetsPath));
	m_assetsPath = assetsPath;

	// ��ʂ̃A�X�y�N�g������O�v�Z
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXTestBase::~DXTestBase()
{

}

// �A�Z�b�g�̃t���p�X�擾�p
std::wstring DXTestBase::GetAssetFullPath(LPCWSTR assetName)
{
	return m_assetsPath + assetName;
}

// DX12���g�p�\�ȃn�[�h�E�F�A�A�_�v�^�[���擾����֐��B
_Use_decl_annotations_
void DXTestBase::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}

// Helper function for setting the window's title text.
void DXTestBase::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = m_title + L": " + text;
	SetWindowText(WinApp::GetHwnd(), windowText.c_str());
}

// �R�}���h���C���������p�[�X����֐�
_Use_decl_annotations_
void DXTestBase::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			m_useWarpDevice = true;
			m_title = m_title + L" (WARP)";
		}
	}
}
