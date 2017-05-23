#include "windows.h"
#include "stdio.h"
#include "string.h"
#include "setupapi.h"
#pragma comment(lib, "setupapi.lib")

char target_device_prefix[22];

bool check(char *trg_prefix)
{
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	char hardware_path[1024];

	// List all connected USB devices
	hDevInfo = SetupDiGetClassDevs(NULL, "USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);

	for (unsigned int index = 0;; index++) {
		DeviceInfoData.cbSize = sizeof(DeviceInfoData);
		if (!SetupDiEnumDeviceInfo(hDevInfo, index, &DeviceInfoData))
			return false;     // no target path prefix match

		SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID, NULL, (BYTE*)hardware_path, sizeof(hardware_path), NULL);

		if (memcmp(hardware_path, trg_prefix, strlen(trg_prefix)) == 0)
			return true;     // match
	}

}

void run_check_and_lock(void)
{
	if (!check(target_device_prefix))
	{
		if (!LockWorkStation())
		{
			MessageBox(NULL, "LockWorkStation failed", "Error", 0);
			exit(1);
		}
	}
}

int str_to_val(char *in)
{
	char *ptr = in;
	int ret = 0;
	while (*ptr != 0)
	{
		if (*ptr >= '0' && *ptr <= '9')
			ret = (ret << 4) + (*ptr - '0');
		else if (*ptr >= 'a' && *ptr <= 'f')
			ret = (ret << 4) + (*ptr - 'a' + 10);
		else if (*ptr >= 'A' && *ptr <= 'F')
			ret = (ret << 4) + (*ptr - 'A' + 10);
		else
			return -1;

		ptr++;
	}
	return ret;
}

LRESULT CALLBACK CallBackProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_DEVICECHANGE:
		run_check_and_lock();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
int main(int argc, char *argv[])
{
	bool is_exist = false;

	long vid = -1, pid = -1;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-vid") == 0 && i + 1 < argc)
			vid = str_to_val(argv[++i]);
		else if (strcmp(argv[i], "-pid") == 0 && i + 1 < argc)
			pid = str_to_val(argv[++i]);
	}

	if (vid == -1 || pid == -1 || vid > 0xffff || pid > 0xffff)
	{
		MessageBox(NULL, "incorrect vid/pid", "error", 0);
		return -1;
	}

	sprintf_s(target_device_prefix, 22, "USB\\VID_%04X&PID_%04X", vid, pid);

	// Window Class Settings
	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(0);
	WNDCLASSEX wndClassEx;
	wndClassEx.cbSize = sizeof(WNDCLASSEX); wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
	wndClassEx.lpfnWndProc = CallBackProc;
	wndClassEx.cbClsExtra = 0; wndClassEx.cbWndExtra = 0; wndClassEx.hInstance = hInst;
	wndClassEx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPLICATION));
	wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClassEx.lpszMenuName = NULL; wndClassEx.lpszClassName = TEXT("win32app");
	wndClassEx.hIconSm = LoadIcon(wndClassEx.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	if (!RegisterClassEx(&wndClassEx)) 
	{ 
		MessageBox(NULL, "RegisterClassEx failed", "Error", 0);
		return 1;
	}

	HWND hWnd = CreateWindow(TEXT("win32app"),
		TEXT("device_checker"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		100, 100,
		NULL, NULL,
		hInst,
		NULL);
	if (!hWnd) { 
		MessageBox(NULL, "CreateWindow failed", "Error", 0); 
		return 1; 
	}

	run_check_and_lock();

	// Update Window
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// Message Loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
