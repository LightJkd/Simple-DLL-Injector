#include <windows.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <wininet.h>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wininet.lib")

typedef NTSTATUS(NTAPI* pNtCreateThreadEx)(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    LPVOID ObjectAttributes,
    HANDLE ProcessHandle,
    LPVOID StartRoutine,
    LPVOID Argument,
    ULONG CreateFlags,
    ULONG_PTR ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    LPVOID AttributeList
);

bool DownloadFile(const std::string& url, const std::string& filePath) {
    HINTERNET hInternet = InternetOpenA("DLL Injector", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) {
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead) {
        outFile.write(buffer, bytesRead);
    }

    outFile.close();
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);
    return true;
}

DWORD GetProcess(const char* processName) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (!strcmp(pe32.szExeFile, processName)) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(DWORD processId, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) return false;

    void* pRemoteDllPath = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteDllPath) {
        CloseHandle(hProcess);
        return false;
    }

    WriteProcessMemory(hProcess, pRemoteDllPath, dllPath, strlen(dllPath) + 1, NULL);

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    pNtCreateThreadEx NtCreateThreadEx = (pNtCreateThreadEx)GetProcAddress(hNtdll, "NtCreateThreadEx");
    if (!NtCreateThreadEx) {
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread;
    NTSTATUS status = NtCreateThreadEx(
        &hThread,
        THREAD_ALL_ACCESS,
        NULL,
        hProcess,
        (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        pRemoteDllPath,
        FALSE,
        0,
        0,
        0,
        NULL
    );

    if (status != 0) {
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

std::string GetTempFolderPath() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    return std::string(tempPath);
}

std::string CreateRandomFolder(const std::string& baseDir) {
    srand(static_cast<unsigned int>(time(NULL)));
    char randomFolderName[MAX_PATH];
    for (int i = 0; i < 8; ++i) {
        randomFolderName[i] = 'A' + rand() % 26;
    }
    randomFolderName[8] = '\0';
    std::string fullPath = baseDir + "\\" + randomFolderName;
    CreateDirectoryA(fullPath.c_str(), NULL);
    return fullPath;
}

void DeleteFolder(const std::string& folderPath) {
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((folderPath + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        const std::string filePath = folderPath + "\\" + findFileData.cFileName;
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
                DeleteFolder(filePath);
            }
        } else {
            DeleteFileA(filePath.c_str());
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
    RemoveDirectoryA(folderPath.c_str());
}

int main() {
    std::string dllPath;
    int choice;

    std::cout << "1. Ввести путь до .dll\n";
    std::cout << "2. Выгрузить .dll с сайта\n";
    std::cin >> choice;
    std::cin.ignore();

    if (choice == 1) {
        std::cout << "Введите путь до .dll: ";
        std::getline(std::cin, dllPath);
    } else if (choice == 2) {
        std::string url;
        std::cout << "Введите URL .dll файла: ";
        std::getline(std::cin, url);

        std::string tempPath = GetTempFolderPath();
        std::string randomFolder = CreateRandomFolder(tempPath);
        dllPath = randomFolder + "\\shcore.dll";

        if (!DownloadFile(url, dllPath)) {
            std::cout << "Ошибка при загрузке DLL файла.\n";
            DeleteFolder(randomFolder);
            return 1;
        }
    } else {
        std::cout << "Неверный выбор.\n";
        return 1;
    }

    DWORD processId = GetProcess("javaw.exe");
    if (processId == 0) {
        std::cout << "Процесс javaw.exe не найден.\n";
        return 1;
    }

    if (InjectDLL(processId, dllPath.c_str())) {
        std::cout << "Инжект успешно удался.\n";
    } else {
        std::cout << "Инжект не удался.\n";
    }

    if (choice == 2) {
        DeleteFolder(dllPath.substr(0, dllPath.find_last_of("\\/")));
    }

    return 0;
}
