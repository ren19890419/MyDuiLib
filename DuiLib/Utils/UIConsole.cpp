#include "StdAfx.h"
#include <iostream>

#include <io.h>
#include <fcntl.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

namespace DuiLib
{
	Console::Console() :m_bConsoleOpen(true)//, m_pLock(nullptr)
	{
		m_bConsoleInit = false;
		m_stdOutputHandle = INVALID_HANDLE_VALUE;
		m_stdErrHandle = INVALID_HANDLE_VALUE;
		m_pLock = nullptr;
		m_pLock = new CDuiLock;
	}
	Console::~Console()
	{
		if (m_pLock) delete m_pLock;
		m_pLock = nullptr;
	}
	void Console::_RedirectIOToConsole()
	{
		if (m_bConsoleInit)
			return;
		CDuiAutoLocker lock(m_pLock);
		// 分配一个控制台，以便于输出一些有用的信息
		AllocConsole();
		// 取得 STDOUT 的文件系统
		m_stdOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		m_stdErrHandle = GetStdHandle(STD_ERROR_HANDLE);
		// Redirect STDOUT to the new console by associating STDOUT's file 
		// descriptor with an existing operating-system file handle.
		int hConsoleHandle = _open_osfhandle((intptr_t)m_stdOutputHandle, _O_TEXT);
		int hConsoleErrHandle = _open_osfhandle((intptr_t)m_stdErrHandle, _O_TEXT);
		FILE *pFile = _fdopen(hConsoleHandle, "w");
		*stdout = *pFile;
		FILE *pFileErr = _fdopen(hConsoleErrHandle, "w");
		*stderr = *pFileErr;
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		// 这个调用确保 iostream 和 C run-time library 的操作在源代码中有序。 
		std::ios::sync_with_stdio();
		m_bConsoleInit = true;
	}
	void Console::_Print(const TCHAR* szBuffer, e_Flag flag)
	{
		CDuiAutoLocker lock(m_pLock);
		switch (flag)
		{
		case e_Flag::e_Error:
			{
				::SetConsoleTextAttribute(m_stdErrHandle, FOREGROUND_RED);
#ifdef UNICODE
				std::wcerr << szBuffer;
#else
				std::cout << szBuffer;
#endif
				::SetConsoleTextAttribute(m_stdErrHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			}
			break;
		case e_Flag::e_Warn:
			{
				::SetConsoleTextAttribute(m_stdOutputHandle, FOREGROUND_RED | FOREGROUND_GREEN);
#ifdef UNICODE
				std::wcout << szBuffer;
#else
				std::cout << szBuffer;
#endif
				::SetConsoleTextAttribute(m_stdOutputHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			}
			break;
		case e_Flag::e_Info:
		default:
			{
#ifdef UNICODE
				std::wcout << szBuffer;
#else
				std::cout << szBuffer;
#endif
			}
			break;
		}

#ifdef _DEBUG
		OutputDebugString(szBuffer);
#endif
	}

	void Console::Write(const TCHAR* format, ...)
	{
		if (!Console::Instance()->m_bConsoleOpen) return;
		Console::Instance()->_RedirectIOToConsole();

		TCHAR szBuffer[10240] = { 0 };
		va_list args;
		va_start(args, format);
		wvnsprintf(szBuffer, lengthof(szBuffer) - 2, format, args);
		va_end(args);

		Console::Instance()->_Print(szBuffer, e_Flag::e_Info);
	}
	void Console::WriteLine(const TCHAR* format, ...)
	{
		if (!Console::Instance()->m_bConsoleOpen) return;
		Console::Instance()->_RedirectIOToConsole();

		TCHAR szBuffer[10240] = { 0 };
		va_list args;
		va_start(args, format);
		wvnsprintf(szBuffer, lengthof(szBuffer) - 2, format, args);
		va_end(args);
		_tcscat(szBuffer, _T("\n"));

		Console::Instance()->_Print(szBuffer, e_Flag::e_Info);
	}
	void Console::Warning(const TCHAR* format, ...)
	{
		if (!Console::Instance()->m_bConsoleOpen) return;
		Console::Instance()->_RedirectIOToConsole();

		TCHAR szBuffer[10240] = { 0 };
		va_list args;
		va_start(args, format);
		wvnsprintf(szBuffer, lengthof(szBuffer) - 2, format, args);
		va_end(args);

		Console::Instance()->_Print(szBuffer, e_Flag::e_Warn);
	}
	void Console::Error(const TCHAR* format, ...)
	{
		if (!Console::Instance()->m_bConsoleOpen) return;
		Console::Instance()->_RedirectIOToConsole();

		TCHAR szBuffer[10240] = { 0 };
		va_list args;
		va_start(args, format);
		wvnsprintf(szBuffer, lengthof(szBuffer) - 2, format, args);
		va_end(args);

		Console::Instance()->_Print(szBuffer, e_Flag::e_Error);
	}

}