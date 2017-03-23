#if defined(_DEBUG)

#include "Debug/Stable.h"
#include <QDateTime>

#include <excpt.h>
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

CRITICAL_SECTION dumpCritical;
bool dumpStarted = false;

void dumpFunction(void* ptr)
{
  PEXCEPTION_POINTERS p = (PEXCEPTION_POINTERS)ptr;
  MINIDUMP_EXCEPTION_INFORMATION ei = {GetCurrentThreadId(), p, TRUE};
  SECURITY_ATTRIBUTES sec = {sizeof(SECURITY_ATTRIBUTES)};
  QDateTime currentTime = QDateTime::currentDateTimeUtc();

  QString fileName = currentTime.toString("ddMMyyhhmmsst") +
//                                          QString("%1%2%3%4").arg(PacsApplication::getMajor()).arg(PacsApplication::getMinor())
//                                          .arg(PacsApplication::getPatch()).arg(PacsApplication::getRevision()) + 
                                          ".dmp";
  HANDLE hfile = CreateFile ( (const wchar_t*)fileName.utf16(), GENERIC_WRITE, 0, &sec, CREATE_ALWAYS, 0, NULL );


  qCritical("unhandled exception caught.");
  EXCEPTION_RECORD* e = p->ExceptionRecord;
  qCritical("CODE:    0x%08x", e->ExceptionCode);
  qCritical("FLAGS:   0x%08x", e->ExceptionFlags);
  qCritical("ADDRESS: 0x%08x", e->ExceptionAddress);

  if ( hfile == INVALID_HANDLE_VALUE )
  {
    qCritical ( "can't create dump file." );
  }
  else
  {
    MINIDUMP_TYPE type = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory); //MiniDumpWithFullMemory; //(MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithDataSegs);
    BOOL result;

    qCritical ( "writing dump..." );
    result = MiniDumpWriteDump ( GetCurrentProcess(), GetCurrentProcessId(), hfile, type, &ei, NULL, NULL);
    if ( result )
    {
      qCritical ( "done" );
      std::wstring message =
        std::wstring(L"Unhandled exeption has occured. Dump saved to file: ") + fileName.toStdWString();
      MessageBox(NULL, message.c_str(), L"Critical", MB_OK );
    }
    else
    {
      qCritical ( "failed(code=%d)", HRESULT_CODE((HRESULT)GetLastError()) );
      MessageBox(NULL, L"Unhandled exeption has occured. Dump has not been saved due to error (see log for details).", L"Critical", MB_OK );
    }
    CloseHandle(hfile); 
    
  }
}

LONG WINAPI filter (PEXCEPTION_POINTERS p)
{
  EnterCriticalSection(&dumpCritical);
  if ( !dumpStarted )
  {
    DWORD id;
    dumpStarted = true;
    HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)dumpFunction, p, 0, &id);

    if ( !hthread )
    {
      qCritical("unable start dump thread, writing dump in current thread...");
      dumpFunction(p);
    }
    else
    {
      WaitForMultipleObjects(1, &hthread, 0, -1);
    }
  }
  LeaveCriticalSection(&dumpCritical);

  return EXCEPTION_EXECUTE_HANDLER;
}

#endif

void installCrashHandler()
{
#if defined(_DEBUG)
  InitializeCriticalSection(&dumpCritical);
  SetUnhandledExceptionFilter(filter);
#endif
}
