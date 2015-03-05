/*
 * XonarRestart - restarts a Xonar DX device
 * Copyright (C) 2015  Kyouma <http://www.github.com/saber2014>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define WIN32_LEAN_AND_MEAN
#define XONAR_COMMAND_EXIT 0x8078
#define MAX_DEVICE_DESCRIPTION 1024

#include <Windows.h>
#include <shellapi.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <SetupAPI.h>
#include <devguid.h>

#pragma comment(lib, "SetupAPI.lib")

BOOL DetectDevice(wchar_t *pwszDeviceName);
BOOL IsProcessRunning(wchar_t *pwszProcessName, DWORD *dwProcessID);
BOOL StartStopService(wchar_t *pwszServiceName, BOOL bStart);
BOOL LoadUnloadDevice(wchar_t *pwszDeviceName, BOOL bLoad);
void GetDirectory(wchar_t *pwszFullPath, wchar_t *pwszDirectory);

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	wprintf(_T("XonarRestart v0.0.1a by Kyouma, 2015.\n"));
	wprintf(_T("\n"));

	wprintf(_T("THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY!\n"));
	wprintf(_T("USE AT YOUR OWN RISK!\n"));
	wprintf(_T("\n"));

	wprintf(_T("This software is not associated with ASUS.\n"));
	wprintf(_T("\n"));

	//detecting Xonar DX
	wprintf(_T("Detecting Xonar DX... "));

	if (DetectDevice(_T("ASUS Xonar DX Audio Device"))) wprintf(_T("OK\n"));
	else
	{
		wprintf(_T("Error\n"));

		goto terminate;
	}

	//checking if AsusAudioCenter.exe is running
	wprintf(_T("Checking if AsusAudioCenter.exe is running... "));

	DWORD dwProcessID = -1;
	wchar_t wszFullPath[MAX_PATH];

	ZeroMemory(wszFullPath, sizeof(wszFullPath));

	if (IsProcessRunning(_T("AsusAudioCenter.exe"), &dwProcessID))
	{
		wprintf(_T("OK\n"));

		//getting the full path of the process
		HANDLE hAsusAudioCenter = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);

		DWORD dwFullPathSize = MAX_PATH;
		QueryFullProcessImageName(hAsusAudioCenter, 0, wszFullPath, &dwFullPathSize);

		CloseHandle(hAsusAudioCenter);

		//terminating AsusAudioCenter.exe
		wprintf(_T("Terminating AsusAudioCenter.exe... "));

		HWND hWnd = FindWindowEx(NULL, NULL, NULL, _T("Xonar DX Audio Center"));
		
		if (hWnd)
		{
			//trying to send exit command to the main window
			PostMessage(hWnd, WM_COMMAND, XONAR_COMMAND_EXIT, 0x0);

			//TODO: wait untill the process terminates
			Sleep(500);
			
			wprintf(_T("OK\n"));
		
		}
		else  wprintf(_T("Error\n"));

	}
	else wprintf(_T("Error\n"));

	//stopping Windows Audio Services
	wprintf(_T("Stopping Windows Audio Services... "));

	if (StartStopService(_T("Audiosrv"), FALSE))
	{
		Sleep(500);

		wprintf(_T("OK\n"));
	}
	else wprintf(_T("Error\n"));
	
	//unloading Xonar DX driver
	wprintf(_T("Unloading Xonar DX driver... "));

	if (LoadUnloadDevice(_T("ASUS Xonar DX Audio Device"), FALSE))
	{
		Sleep(500);

		wprintf(_T("OK\n"));
	}
	else wprintf(_T("Error\n"));

	//loading Xonar DX driver
	wprintf(_T("Loading Xonar DX driver... "));

	if (LoadUnloadDevice(_T("ASUS Xonar DX Audio Device"), TRUE))
	{
		Sleep(500);

		wprintf(_T("OK\n"));
	}
	else wprintf(_T("Error\n"));

	//starting Windows Audio Services
	wprintf(_T("Starting Windows Audio Services... "));

	if (StartStopService(_T("Audiosrv"), TRUE))
	{
		Sleep(500);

		wprintf(_T("OK\n"));
	}
	else wprintf(_T("Error\n"));

	//starting AsusAudioCenter.exe
	if (dwProcessID > 0)
	{
		wprintf(_T("Starting AsusAudioCenter.exe... "));
		
		wchar_t wszDirectory[MAX_PATH];
		ZeroMemory(wszDirectory, sizeof(wszDirectory));

		GetDirectory(wszFullPath, (wchar_t *)&wszDirectory);

		if (ShellExecute(NULL, _T("open"), wszFullPath, NULL, wszDirectory, SW_SHOWNA) > (HINSTANCE)32)
			wprintf(_T("OK\n"));
		else
			wprintf(_T("Error\n"));
	}

terminate:
	return 0;
}

BOOL DetectDevice(wchar_t *pwszDeviceName)
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_MEDIA, NULL, NULL, DIGCF_PRESENT);

	if (hDevInfo == INVALID_HANDLE_VALUE)
		return FALSE;

	SP_DEVINFO_DATA did = { sizeof(SP_DEVINFO_DATA) };

	for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &did); i++)
	{
		wchar_t wszDeviceDescription[MAX_DEVICE_DESCRIPTION];

		if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &did, SPDRP_DEVICEDESC, NULL, (BYTE *)wszDeviceDescription, MAX_DEVICE_DESCRIPTION, NULL))
			return FALSE;

		if (wcscmp(pwszDeviceName, wszDeviceDescription) == 0)
			return TRUE;
	}

	return FALSE;
}

BOOL IsProcessRunning(wchar_t *pwszProcessName, DWORD *dwProcessID)
{
	BOOL bProcessRunning = FALSE;
	PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return FALSE;
	
	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			if (wcscmp(pe32.szExeFile, pwszProcessName) == 0)
			{
				*dwProcessID = pe32.th32ProcessID;
				bProcessRunning = TRUE;

				break;
			}
		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);

	return bProcessRunning;
}

BOOL StartStopService(wchar_t *pwszServiceName, BOOL bStart)
{
	BOOL ret = FALSE;

	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if (!schSCManager)
		goto terminate;

	SC_HANDLE schService = OpenService(schSCManager, pwszServiceName, SC_MANAGER_ALL_ACCESS);
	
	if (!schService)
		goto terminate;

	SERVICE_STATUS ss;
	ZeroMemory(&ss, sizeof(ss));
	
	ret = bStart ? StartService(schService, 0, NULL) : ControlService(schService, SERVICE_CONTROL_STOP, &ss);
	goto terminate;

terminate:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return ret;
}

BOOL LoadUnloadDevice(wchar_t *pwszDeviceName, BOOL bLoad)
{
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_MEDIA, NULL, NULL, DIGCF_PRESENT);

	if (hDevInfo == INVALID_HANDLE_VALUE)
		return FALSE;

	SP_DEVINFO_DATA did = { sizeof(SP_DEVINFO_DATA) };

	for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &did); i++)
	{
		wchar_t wszDeviceDescription[MAX_DEVICE_DESCRIPTION];

		if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &did, SPDRP_DEVICEDESC, NULL, (BYTE *)wszDeviceDescription, MAX_DEVICE_DESCRIPTION, NULL))
			return FALSE;

		if (wcscmp(pwszDeviceName, wszDeviceDescription) == 0)
		{
			SP_CLASSINSTALL_HEADER cih = { sizeof(SP_CLASSINSTALL_HEADER), DIF_PROPERTYCHANGE };
			SP_PROPCHANGE_PARAMS pcp = { cih, bLoad ? DICS_ENABLE : DICS_DISABLE, DICS_FLAG_GLOBAL, 0 };

			if (!SetupDiSetClassInstallParams(hDevInfo, &did, (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp)))
				return FALSE;

			if (!SetupDiChangeState(hDevInfo, &did))
				return FALSE;

			return TRUE;
		}
	}

	return FALSE;
}

void GetDirectory(wchar_t *pwszFullPath, wchar_t *pwszDirectory)
{
	int nLength = wcslen(pwszFullPath);
	int nNewLength;

	for (int i = nLength - 1; i >= 0; i--)
	{
		if (pwszFullPath[i] == _T('\\'))
		{
			nNewLength = i;
			break;
		}
	}

	memcpy(pwszDirectory, pwszFullPath, nNewLength * sizeof(wchar_t));
}