#include "stdafx.h"
#include "Resource.h"
#include "MWinMgr.h"
#include "MainWin.h"
#include "NHttp.h"

#pragma comment (lib, "NbkCore.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "zdll.lib")
#pragma comment (lib, "Gdiplus.lib")

using namespace Gdiplus;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	int ret = FALSE;
	WSADATA wsad;

	ret = WSAStartup(MAKEWORD(2, 2), &wsad);
	if (ret != 0) {
		OutputDebugString(_T("WSAStartup failed!\n"));
		return FALSE;
	}

	GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	MWinMgr::Init(hInstance);
	MWinMgr* mgr = MWinMgr::GetInstance();

	MainWin* mainWin = new MainWin();
	if (mainWin->Create(NULL)) {
		mainWin->Show(nCmdShow);
		mainWin->Update();

		mgr->LoadAccelerators(IDC_NBKSHELL);
		ret = mgr->MainLoop();
	}

	delete mainWin;
	mgr->Quit();

	GdiplusShutdown(gdiplusToken);
	WSACleanup();

	return ret;
}
