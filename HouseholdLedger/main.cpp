#include <windows.h>
#include <wrl.h>

#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "AccountManager.h"
#include "ApiServer.h"
#include "BudgetManager.h"
#include "CategoryManager.h"
#include "ScheduleManager.h"

#include "WebView2.h"

using Microsoft::WRL::ComPtr;

namespace {
struct ResourceEntry {
    int id;
    const wchar_t* filename;
};

const ResourceEntry kResourceFiles[] = {
    { 101, L"index.html" },
    { 102, L"app.js" },
    { 103, L"style.css" },
    { 104, L"budget.html" },
    { 105, L"budget.js" },
    { 106, L"rpg_preview.html" },
    { 107, L"rpg_preview.md" },
    { 108, L"schedules.html" },
    { 109, L"schedules.js" }
};

bool writeResourceToFile(HINSTANCE instance, int resourceId, const std::filesystem::path& outputPath) {
    HRSRC resInfo = FindResourceW(instance, MAKEINTRESOURCEW(resourceId), L"FILE");
    if (!resInfo) {
        return false;
    }
    HGLOBAL resData = LoadResource(instance, resInfo);
    if (!resData) {
        return false;
    }
    DWORD size = SizeofResource(instance, resInfo);
    void* data = LockResource(resData);
    if (!data || size == 0) {
        return false;
    }

    std::filesystem::create_directories(outputPath.parent_path());
    HANDLE file = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(file, data, size, &written, nullptr);
    CloseHandle(file);
    return ok && written == size;
}

std::filesystem::path extractResourcesToTemp(HINSTANCE instance) {
    wchar_t tempPathBuffer[MAX_PATH];
    DWORD pathLen = GetTempPathW(MAX_PATH, tempPathBuffer);
    std::filesystem::path tempRoot(tempPathBuffer, tempPathBuffer + pathLen);

    wchar_t tempFileName[MAX_PATH];
    GetTempFileNameW(tempRoot.c_str(), L"HLD", 0, tempFileName);
    std::filesystem::path tempDir(tempFileName);
    DeleteFileW(tempFileName);
    std::filesystem::create_directories(tempDir);

    for (const auto& entry : kResourceFiles) {
        writeResourceToFile(instance, entry.id, tempDir / entry.filename);
    }

    return tempDir;
}

struct AppState {
    httplib::Server server;
    std::thread serverThread;
    std::filesystem::path mountPath;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_SIZE) {
        auto* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (state && state->controller) {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            state->controller->put_Bounds(bounds);
        }
        return 0;
    }
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCmd) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    AccountManager manager;
    CategoryManager categories;
    BudgetManager budgets;
    ScheduleManager schedules;
    categories.loadFromFile();
    schedules.generateDueTransactions(manager);

    AppState state;
    state.mountPath = extractResourcesToTemp(instance);
    register_routes(state.server, manager, categories, budgets, schedules, state.mountPath.u8string());
    state.serverThread = std::thread([&state]() {
        state.server.listen("127.0.0.1", 8888);
    });

    const wchar_t kClassName[] = L"HouseholdLedgerWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        kClassName,
        L"Household Ledger",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd) {
        state.server.stop();
        if (state.serverThread.joinable()) {
            state.serverThread.join();
        }
        CoUninitialize();
        return 0;
    }

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));
    ShowWindow(hwnd, showCmd);

    std::wstring url = L"http://localhost:8888/";
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd, &state, url](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (!env) {
                    return result;
                }
                env->CreateCoreWebView2Controller(
                    hwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd, &state, url](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (controller) {
                                state.controller = controller;
                                controller->get_CoreWebView2(&state.webview);
                                RECT bounds;
                                GetClientRect(hwnd, &bounds);
                                controller->put_Bounds(bounds);
                                if (state.webview) {
                                    state.webview->Navigate(url.c_str());
                                }
                            }
                            return result;
                        })
                        .Get());
                return result;
            })
            .Get());

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    state.server.stop();
    if (state.serverThread.joinable()) {
        state.serverThread.join();
    }
    CoUninitialize();
    return 0;
}
