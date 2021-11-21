#pragma once
/*

  For no-console windows desktop apps we use as the last resort log file
  epsecially in release builds where there is no OutputDebugString

  CAVEAT EMPTOR: this is safe and slow approach. if speed is primary do not use 
 
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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <crtdbg.h>

#undef assert
#define assert _ASSERTE

HANDLE dbj_create_log_file(LPWSTR log_file_name, int append_log_suffix);

void  dbj_write_log_file(LPCSTR text_);


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

HANDLE dbj_create_log_file(LPWSTR log_file_name, int append_log_suffix)
{
    WCHAR wbuff[2048] = { 0 };
    if (append_log_suffix) {
        int rez = wsprintfW(wbuff, L"%s.log", log_file_name);

        assert(rez > 1);

        log_file_name = &wbuff[0];
    }

    HANDLE hFile = CreateFileW(log_file_name,                // name of the write
        GENERIC_WRITE,          // open for writing
        0,                      // do not share
        NULL,                   // default security
        CREATE_NEW,             // create new file only
        FILE_ATTRIBUTE_NORMAL,  // normal file
        NULL);                  // no attr. template

    if (hFile == INVALID_HANDLE_VALUE)
    {
        error("CreateFile failed, aborting ...");
        exit(0);
    }
    return hFile;
}

/*
this actually might be NOT slower than using some global CRITICAL_SECTION
and entering/deleting it only once ... why not measuring it?
*/
void  default_protector_function(int lock)
{
    // Think: this is one per process
    static CRITICAL_SECTION   CS_;

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

    default_protector_function(1);

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
        error("WriteFile failed, aborting ...");
        exit(0);
    }

    default_protector_function(0);
}

#endif // DBJ_WIN32_LOGFILE_IMPLEMENTATION