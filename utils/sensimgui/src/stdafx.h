// Change these values to use different versions
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745%28v=vs.85%29.aspx

// Windows XP and later
#define WINVER			0x0501
#define _WIN32_WINNT	0x0501
#define _WIN32_IE		0x0500
#define _RICHEDIT_VER	0x0100

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

//** Eliminating deprecation warnings
// http://msdn.microsoft.com/en-us/library/ms175759%28v=vs.80%29.aspx
// #define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

//** для CPropertySheetImpl
#define _WTL_NEW_PAGE_NOTIFY_HANDLERS

//** With Visual C++, STRICT type checking is defined by default.
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa383731%28v=vs.85%29.aspx
// http://stackoverflow.com/questions/11204157/vs2010-debugger-in-c-meaning-of-unused-or-unused-0
// http://forums.codeguru.com/showthread.php?517986-Visual-C-Why-Watch-window-shows-unused-for-a-HWND-variable
//#define STRICT

#include <atlbase.h>	// ATL
#if (_ATL_VER >= 0x0700)
#define _WTL_NO_CSTRING	// not using WTL::CString
#include <atlstr.h>		// ATL, using ATL::CString/CStringA/CStringW insread of WTL::CString
#endif
#include <atlapp.h>		// WTL

#if (_ATL_VER < 0x0700)
#define _AtlBaseModule _Module
#endif
extern CAppModule _Module;

#include <atlcom.h>		// ATL
#include <atlhost.h>	// ATL
#include <atlwin.h>		// ATL
#include <atlctl.h>		// ATL

#include <atlmisc.h>	// WTL
#include <atlddx.h>		// WTL

#if _MSC_VER >= 1400
// Manifest
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
