

#include "../Std/stdafx.h"
#include "../DX/D3D12Basic.h"
#include "../DX/D3D12Triangle.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	//D3D12Basic sample(1280, 720, L"D3D12 Hello Window");
	D3D12Triangle sample(1280, 720, L"D3D12 Triangle");
	return WinApp::Run(&sample, hInstance, nCmdShow);
}
