/* Starts the installed Dia package and loads diatweaks.dll into that Dia.exe.
   It will not load the DLL into any other process. The MSIX is not changed. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_from_ps;
static DWORD g_last_error;
static wchar_t g_message[1024];

static void note(const wchar_t *text) {
    wcsncpy(g_message, text, 1023);
    g_message[1023] = 0;
    fwprintf(stderr, L"%ls\n", g_message);
    fflush(stderr);
}

static void save_note(void) {
    wchar_t root[MAX_PATH], path[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", root, MAX_PATH);
    if (!n || n >= MAX_PATH) return;
    _snwprintf(path, MAX_PATH, L"%s\\DiaPatches", root);
    CreateDirectoryW(path, NULL);
    _snwprintf(path, MAX_PATH, L"%s\\DiaPatches\\last-run.txt", root);
    FILE *file = _wfopen(path, L"wb");
    if (!file) return;
    char utf[1200];
    WideCharToMultiByte(CP_UTF8, 0, g_message, -1, utf, sizeof utf, NULL, NULL);
    fputs(utf, file);
    fputc('\n', file);
    fclose(file);
}

static int contains_i(const wchar_t *hay, const wchar_t *needle) {
    size_t n = wcslen(needle);
    for (; hay && *hay; hay++)
        if (_wcsnicmp(hay, needle, n) == 0) return 1;
    return 0;
}

static int is_dia_image(const wchar_t *path) {
    const wchar_t *base = wcsrchr(path, L'\\');
    base = base ? base + 1 : path;
    if (_wcsicmp(base, L"Dia.exe") != 0) return 0;
    return contains_i(path, L"\\WindowsApps\\TheBrowserCompany.Dia");
}

static int command_is_child(HANDLE process, int *known) {
    typedef long NTSTATUS;
    typedef NTSTATUS(NTAPI *QueryFn)(HANDLE, int, void *, unsigned long, unsigned long *);
    *known = 0;
    FARPROC raw_query = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");
    QueryFn query = NULL;
    if (!raw_query) return 0;
    memcpy(&query, &raw_query, sizeof query);
    unsigned long need = 0;
    query(process, 60, NULL, 0, &need);
    if (need < 64 || need > 65536) need = 8192;
    unsigned char *buf = (unsigned char *)calloc((size_t)need + sizeof(wchar_t), 1);
    if (!buf) return 0;
    unsigned long got = 0;
    NTSTATUS status = query(process, 60, buf, need, &got);
    int child = 0;
    if (status >= 0) {
        *known = 1;
        typedef struct {
            unsigned short length;
            unsigned short max;
            wchar_t *buffer;
        } UStr;
        UStr *us = (UStr *)buf;
        wchar_t stack[1500];
        const wchar_t *text = NULL;
        if (us->buffer && (unsigned char *)us->buffer >= buf && (unsigned char *)us->buffer < buf + need) {
            text = us->buffer;
        } else if (us->buffer && us->length && us->length < sizeof stack) {
            SIZE_T read = 0;
            if (ReadProcessMemory(process, us->buffer, stack, us->length, &read)) {
                stack[read / sizeof(wchar_t)] = 0;
                text = stack;
            }
        }
        if (!text) {
            size_t chars = got / sizeof(wchar_t);
            for (size_t i = 0; i + 7 < chars; i++) {
                if (_wcsnicmp((wchar_t *)buf + i, L"--type=", 7) == 0) child = 1;
            }
        } else if (wcsstr(text, L"--type=")) {
            child = 1;
        }
    }
    free(buf);
    return child;
}

typedef struct {
    DWORD pid;
    int found;
} WindowProbe;

static BOOL CALLBACK window_probe(HWND hwnd, LPARAM param) {
    WindowProbe *probe = (WindowProbe *)param;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != probe->pid || !IsWindowVisible(hwnd)) return TRUE;
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) return TRUE;
    if (rect.right - rect.left > 200 && rect.bottom - rect.top > 200) probe->found = 1;
    return TRUE;
}

static int has_window(DWORD pid) {
    WindowProbe probe = {pid, 0};
    EnumWindows(window_probe, (LPARAM)&probe);
    return probe.found;
}

static int dll_loaded(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W mod;
    mod.dwSize = sizeof mod;
    int yes = 0;
    if (Module32FirstW(snap, &mod)) {
        do {
            if (_wcsicmp(mod.szModule, L"diatweaks.dll") == 0) yes = 1;
        } while (!yes && Module32NextW(snap, &mod));
    }
    CloseHandle(snap);
    return yes;
}

static int inject_pid(DWORD pid, const wchar_t *dll) {
    if (dll_loaded(pid)) return 1;
    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                                     PROCESS_VM_READ,
                                 FALSE, pid);
    if (!process) {
        g_last_error = GetLastError();
        return 0;
    }
    size_t bytes = (wcslen(dll) + 1) * sizeof(wchar_t);
    void *remote = VirtualAllocEx(process, NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    int wrote = remote && WriteProcessMemory(process, remote, dll, bytes, NULL);
    FARPROC load = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    LPTHREAD_START_ROUTINE start = NULL;
    if (load) memcpy(&start, &load, sizeof start);
    HANDLE thread = NULL;
    if (wrote && start) thread = CreateRemoteThread(process, NULL, 0, start, remote, 0, NULL);
    if (!thread) {
        g_last_error = GetLastError();
        if (remote) VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return 0;
    }
    WaitForSingleObject(thread, 15000);
    CloseHandle(thread);
    if (remote) VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);
    for (int i = 0; i < 40; i++) {
        if (dll_loaded(pid)) return 1;
        Sleep(50);
    }
    return 0;
}

static int list_dia(DWORD *pids, int cap) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof entry;
    int count = 0;
    if (Process32FirstW(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, L"Dia.exe") != 0) continue;
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
            if (!process) continue;
            wchar_t path[2048];
            DWORD cch = 2048;
            int match = QueryFullProcessImageNameW(process, 0, path, &cch) && is_dia_image(path);
            CloseHandle(process);
            if (match && count < cap) pids[count++] = entry.th32ProcessID;
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
    return count;
}

/* 1 = UI process, 0 = renderer/gpu/utility, -1 = command line unreadable. */
static int ui_kind(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!process) return has_window(pid) ? 1 : -1;
    int known = 0;
    int child = command_is_child(process, &known);
    CloseHandle(process);
    if (known) return child ? 0 : 1;
    return has_window(pid) ? 1 : -1;
}

static int inject_ui(const wchar_t *dll) {
    DWORD pids[32];
    int count = list_dia(pids, 32);
    int loaded = 0;
    for (int i = 0; i < count; i++) {
        int kind = ui_kind(pids[i]);
        if (kind == 0) continue;
        if (kind < 0) continue;
        if (inject_pid(pids[i], dll)) loaded = 1;
    }
    return loaded;
}

static int dia_alias(wchar_t *out, int cap) {
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", out, cap);
    if (!n || n >= (DWORD)cap) return 0;
    if (wcslen(out) + 32 >= (size_t)cap) return 0;
    wcscat(out, L"\\Microsoft\\WindowsApps\\Dia.exe");
    return GetFileAttributesW(out) != INVALID_FILE_ATTRIBUTES;
}

static int our_dll(wchar_t *out, int cap) {
    DWORD n = GetModuleFileNameW(NULL, out, cap);
    if (!n || n >= (DWORD)cap) return 0;
    wchar_t *slash = wcsrchr(out, L'\\');
    if (!slash || (size_t)(slash - out) + 16 >= (size_t)cap) return 0;
    wcscpy(slash + 1, L"diatweaks.dll");
    return GetFileAttributesW(out) != INVALID_FILE_ATTRIBUTES;
}

static int status_text(void) {
    DWORD pids[32];
    int count = list_dia(pids, 32);
    int ui = 0, loaded = 0;
    for (int i = 0; i < count; i++) {
        if (ui_kind(pids[i]) != 1) continue;
        ui = 1;
        if (dll_loaded(pids[i])) loaded = 1;
    }
    wchar_t alias[MAX_PATH];
    int installed = dia_alias(alias, MAX_PATH);
    _snwprintf(g_message, 1024, L"dia-installed: %ls\r\ndia-running: %ls\r\nloaded: %ls", installed ? L"yes" : L"no",
               ui ? L"yes" : L"no", loaded ? L"yes" : L"no");
    wprintf(L"%ls\n", g_message);
    return 0;
}

static int cmd_inject(const wchar_t *dll, int wait_ms) {
    DWORD start = GetTickCount();
    for (;;) {
        if (inject_ui(dll)) {
            note(L"DiaTweaks is loaded into Dia.");
            return 0;
        }
        if (wait_ms <= 0 || GetTickCount() - start > (DWORD)wait_ms) break;
        Sleep(150);
    }
    DWORD pids[8];
    if (list_dia(pids, 8) == 0) note(L"Dia is not running.");
    else {
        wchar_t text[256];
        _snwprintf(text, 256, L"Dia is running, but DiaTweaks could not load into the window process (error %lu).",
                   g_last_error);
        note(text);
    }
    return 1;
}

static int cmd_launch(const wchar_t *dll) {
    DWORD pids[8];
    if (list_dia(pids, 8) > 0) return cmd_inject(dll, 0);
    wchar_t alias[MAX_PATH];
    if (!dia_alias(alias, MAX_PATH)) {
        note(L"Dia is not installed for this user (no Microsoft\\WindowsApps\\Dia.exe alias).");
        return 1;
    }
    wchar_t cmdline[1200];
    _snwprintf(cmdline, 1200, L"\"%ls\"", alias);
    STARTUPINFOW si;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(alias, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        wchar_t text[256];
        _snwprintf(text, 256, L"Could not start Dia from its app execution alias (error %lu).", GetLastError());
        note(text);
        return 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (cmd_inject(dll, 30000) != 0) return 1;
    note(L"Started Dia with DiaTweaks.");
    return 0;
}

int main(void) {
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    const wchar_t *cmd = L"launch";
    for (int i = 1; argv && i < argc; i++) {
        if (wcscmp(argv[i], L"--from-ps") == 0) g_from_ps = 1;
        else cmd = argv[i];
    }
    wchar_t dll[MAX_PATH];
    int rc = 0;
    int have_dll = our_dll(dll, MAX_PATH);
    if (wcscmp(cmd, L"status") == 0) {
        rc = status_text();
    } else if (!have_dll) {
        note(L"diatweaks.dll is not next to diawrap.exe.");
        rc = 1;
    } else if (wcscmp(cmd, L"inject") == 0) {
        rc = cmd_inject(dll, 0);
    } else if (wcscmp(cmd, L"launch") == 0) {
        rc = cmd_launch(dll);
    } else {
        note(L"usage: diawrap launch | inject | status");
        rc = 2;
    }
    save_note();
    if (rc != 0 && !g_from_ps && wcscmp(cmd, L"status") != 0)
        MessageBoxW(NULL, g_message, L"DiaTweaks", MB_OK | MB_ICONWARNING);
    if (argv) LocalFree(argv);
    return rc;
}
