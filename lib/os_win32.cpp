#include <iso646.h>
#include <string>
#include <windows.h>

#include "rtl_exec.h"

struct Process {
    HANDLE process;
};

namespace rtl {

void os$chdir(const std::string &path)
{
    BOOL r = SetCurrentDirectory(path.c_str());
    if (not r) {
        throw RtlException("PathNotFound", path);
    }
}

std::string os$getcwd()
{
    char buf[MAX_PATH];
    GetCurrentDirectory(sizeof(buf), buf);
    return buf;
}

void os$kill(void *process)
{
    Process *p = reinterpret_cast<Process *>(process);
    TerminateProcess(p->process, 1);
    CloseHandle(p->process);
    p->process = INVALID_HANDLE_VALUE;
    delete p;
}

void *os$spawn(const std::string &command)
{
    Process *p = new Process;
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    BOOL r = CreateProcess(
        NULL,
        const_cast<LPSTR>(command.c_str()),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi);
    if (not r) {
        throw RtlException("PathNotFound", command.c_str());
    }
    p->process = pi.hProcess;
    CloseHandle(pi.hThread);
    return p;
}

Number os$wait(void *process)
{
    DWORD r;
    {
        Process *p = reinterpret_cast<Process *>(process);
        WaitForSingleObject(p->process, INFINITE);
        GetExitCodeProcess(p->process, &r);
        CloseHandle(p->process);
        p->process = INVALID_HANDLE_VALUE;
        delete p;
    }
    return number_from_uint32(r);
}

}