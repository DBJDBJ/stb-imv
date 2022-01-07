#pragma once
/*

  For no-console windows desktop apps we use as the last resort log file
  epsecially in release builds where there is no OutputDebugString

  CAVEAT EMPTOR: this is safe and slow approach. If speed is primary concern, do not use 
 
 (c) 2021 by dbj@dbj.org 
 https://dbj.org/license_dbj
 
include exactly once like this
 
#define DBJ_WIN32_LOGFILE_IMPLEMENTATION
#include <dbj_win32_log_file.h>

how to use:

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   LPWSTR       cmdline = GetCommandLineW();
   int argc = 0 ;
   LPWSTR       *argv = CommandLineToArgvW(cmdline, &argc);

   assert( argc > 0 );

   // log file is app_full_path + '.log'
   dbj_log_file = dbj_create_log_file(argv[0], 1);
   dbj_write_log_file("STARTING");

   #ifndef __clang__
   // if clang this will be auto called on exit
   // otherwise must call manually before main ends
   dbj_close_log_file();
   #endif

 }

*/
// https://docs.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers

#ifndef _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10 0x0A00
#endif // _WIN32_WINNT_WIN10

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif 

#ifndef WINVER  
#define  WINVER  _WIN32_WINNT_WIN10
#endif

#define NOCOMM
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <crtdbg.h>
#include <stdio.h> // vsprintf_s

#undef assert
#define assert _ASSERTE

// single function API, risky ... but ...
void dbj_log_print(const char* file_, const char* line_, char* str, ...);

#ifndef __clang__
void dbj_close_log_file(void);
void dbj_construct_log_file(void);
#else  // __clang__
#endif // __clang__


#ifdef DBJ_WIN32_LOGFILE_IMPLEMENTATION

// DBJ: added log file handle
HANDLE dbj_log_file = 0;

#ifdef __clang__
__attribute__((destructor))
#endif // __clang__
void dbj_close_log_file(void) {
    if (dbj_log_file)
        CloseHandle(dbj_log_file);
}

void dbj_terror( void )
{
    wchar_t buf[BUFSIZ] = {0 } ;
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

    MessageBoxW(NULL, buf, L"dbj win32 log file terminating error", MB_OK);

    exit(0);
}

HANDLE dbj_create_log_file(void)
{
    LPWSTR       cmdline = GetCommandLineW();
    int argc;
    LPWSTR* argv = CommandLineToArgvW(cmdline, &argc);

    LPWSTR log_file_name = argv[0];

    WCHAR wbuff[2048] = { 0 };
    {
    int rez = wsprintfW(wbuff, L"%s.log", log_file_name);
    assert(rez > 1);
    log_file_name = &wbuff[0];
    }

    HANDLE hFile = CreateFileW(
        log_file_name,          // name of the write
        GENERIC_WRITE,          // open for writing
        0,                      // do not share
        NULL,                   // default security
        CREATE_ALWAYS,          // create new file always
        FILE_ATTRIBUTE_NORMAL,  // normal file
        NULL);                  // no attr. template

    if(hFile == INVALID_HANDLE_VALUE)
        dbj_terror();
    return hFile;
}

#ifdef __clang__
__attribute__((constructor))
#endif // __clang__
void dbj_construct_log_file(void) {
    dbj_log_file = dbj_create_log_file();
}

/*
this actually might be NOT slower than using some global CRITICAL_SECTION
and entering/deleting it only once ... why not measuring it?
*/
void  default_protector_function(CRITICAL_SECTION CS_, int lock)
{
    // Think: this is one per process
    // therefore no
    // static CRITICAL_SECTION   CS_;

    if (lock)
    {
        InitializeCriticalSection(&CS_);
        EnterCriticalSection(&CS_);
    }
    else {
        LeaveCriticalSection(&CS_);
        DeleteCriticalSection(&CS_);
    }
}

void  dbj_write_log_file(LPCSTR text_)
{
    assert(text_);
    assert(dbj_log_file);

    DWORD dwBytesToWrite = (DWORD)strlen(text_);
    DWORD dwBytesWritten = 0;

    BOOL bErrorFlag = WriteFile(
        dbj_log_file,           // open file handle
        text_,      // start of data to write
        dwBytesToWrite,  // number of bytes to write
        &dwBytesWritten, // number of bytes that were written
        NULL);            // no overlapped structure

    if (FALSE == bErrorFlag)
    {
        dbj_terror();
    }
}

// DBJ: just a bit less trivial
void dbj_log_print(const char* file_, const char* line_, char* str, ...) 
{
    assert(file_ && line_);

    static CRITICAL_SECTION   CS_; // local per this function
    default_protector_function(CS_,1);

    char buffer[1024] = { 0 };
    va_list va = 0;
    va_start(va, str);
    int vsprintf_rezult = vsprintf_s(buffer, 1024, str, va);
    assert(vsprintf_rezult > 0);
    va_end(va);

    dbj_write_log_file("\n");
    dbj_write_log_file(file_);
    dbj_write_log_file("|");
    dbj_write_log_file(line_);
    dbj_write_log_file("|");
    dbj_write_log_file(buffer);

#ifdef DBJ_ERROR_USES_MBOX
    MessageBox(NULL, buffer, "imv(stb) (dbj 2021) error", MB_OK);
#endif
    default_protector_function(CS_, 0);
}

#endif // DBJ_WIN32_LOGFILE_IMPLEMENTATION