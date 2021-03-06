// hook_messagebox.cpp: 定義主控台應用程式的進入點。
//

#include "stdafx.h"
#include <Windows.h>
#define print(X) OutputDebugString(X)
#define jmp 0xe9
#define CODE_LENGTH 5


BYTE oldCode[CODE_LENGTH];
BYTE newCode[CODE_LENGTH];

HANDLE hProcess;
HINSTANCE hInst;


typedef int(WINAPI *ptrMessageBoxW)(
	HWND    hWnd,
	LPCWSTR lpText,
	LPCWSTR lpCaption,
	UINT    uType
	);
ptrMessageBoxW originMsgBox;


void hookOff();
void hookOn();
void GetAdr();

int WINAPI hookedMessageBoxW(
	HWND    hWnd,
	LPCWSTR lpText,
	LPCWSTR lpCaption,
	UINT    uType
){
	print(lpText);
	hookOff();
	int ret = MessageBoxW(hWnd, _T("Hooked"), lpCaption, uType);
	hookOff();
	return ret;
};
void debugPrivilege() {
	HANDLE hToken;
	bool bRet = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
	if (bRet) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
	}
}
void hookOn() {
	if (hProcess == NULL)return;
	print(_T("hookon\n"));
	DWORD dwTmp;
	DWORD dwOldProtect;
	SIZE_T writedByte;

	VirtualProtectEx(hProcess, originMsgBox, CODE_LENGTH, PAGE_READWRITE, &dwOldProtect);
	WriteProcessMemory(hProcess, originMsgBox, newCode, CODE_LENGTH, &writedByte);
	if (writedByte == 0)print(_T("[!] Fail to write process!\n"));

	VirtualProtectEx(hProcess, originMsgBox, CODE_LENGTH, dwOldProtect, &dwTmp);
}

void hookOff() {
	if (hProcess == NULL)return;
	DWORD dwTmp;
	DWORD dwOldProtect;
	SIZE_T writedByte;
	
	VirtualProtectEx(hProcess, originMsgBox, CODE_LENGTH, PAGE_READWRITE, &dwOldProtect);
	WriteProcessMemory(hProcess, originMsgBox, oldCode, CODE_LENGTH,&writedByte);
	
	if(writedByte==0)print(_T("[!] Fail to write process!\n"));

	VirtualProtectEx(hProcess, originMsgBox, CODE_LENGTH, dwOldProtect, &dwTmp);
}

void GetAdr() {
	HMODULE hModule = LoadLibrary(_T("user32.dll"));
	if (hModule == NULL) {
		print(_T("[!] Fail to load lib!\n"));
		return;
	}
	originMsgBox = (ptrMessageBoxW)GetProcAddress(hModule, "MessageBoxW");
	if (originMsgBox == NULL) {
		print(_T("[!] Fail to get Adr!\n"));
		return;
	}
	memcpy(oldCode, originMsgBox, 5);
	/*_asm {
		mov esi, originMsgBox
		lea edi, oldCode
		cld
		movsd
		movsb

	}*/
	newCode[0] = jmp;
	_asm
	{
		lea eax, hookedMessageBoxW
		mov ebx, originMsgBox
		sub eax, ebx
		sub eax, CODE_LENGTH
		mov dword ptr[newCode + 1], eax
	}/*
	 jmp dest
	 dest = myAddress - originAddress - 5
	 */

	hookOn();

}
int main()
{
	debugPrivilege();
	hProcess = GetCurrentProcess();
	if (hProcess == NULL)print(_T("[!] Fail to get Hproc \n"));

	GetAdr();
	MessageBox(NULL, _T("Test"), _T("Wonder if it works"), MB_OK);
    return 0;
}







