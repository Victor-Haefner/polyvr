#define CEF_ENABLE_SANDBOX 0

#include <windows.h>
#include <include/cef_app.h>

int APIENTRY main(HINSTANCE hInstance) {
    CefMainArgs args(hInstance);
    return CefExecuteProcess(args, 0, 0);
}