

#include "../Std/stdafx.h"
#include "WinApp.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	return WinApp::Run(/*&sample,*/ hInstance, nCmdShow);
}
