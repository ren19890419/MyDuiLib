// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#ifndef _AFXDLL
#define _AFXDLL
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers 
#endif
#include <afxwin.h> // MFC core and standard components 
#include <afxext.h> // MFC extensions 
#include <afxdisp.h> // MFC Automation classes 
#include <afxdtctl.h> // MFC support for Internet Explorer 4 Common Controls 
#ifndef _AFX_NO_AFXCMN_SUPPORT 
#include <afxcmn.h> // MFC support for Windows Common Controls 
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxmt.h>
#include <afxsock.h>
/*#include <WinSock2.h>*/
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include <Mmsystem.h>
 #define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// // Windows 头文件:
// #include <windows.h>

// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <wchar.h> 
#include <deque>
#ifndef _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的
#endif
#include <atlbase.h>
#include <atlstr.h>

#include "resource.h"
// TODO: 在此处引用程序需要的其他头文件

// TODO: 在此处引用程序需要的其他头文件
#include "UIlib.h"
using namespace DuiLib;

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 5556
#define THREAD_SLEEP 500
#define TO_DUISTRING(s) DuiLib::toDuiString(s)
#define TO_STDSTRING(s) DuiLib::toStdString(s)

#ifdef _UNICODE
#define MYSTDSTRING std::wstring
#else
#define MYSTDSTRING std::string
#endif

namespace DuiLib
{
	CDuiString toDuiString(const char* arg);
	std::string toStdString(const LPCTSTR str);
}


#include "Convert.h"
using namespace FConvert;