#define CEF_ENABLE_SANDBOX 0

#include <include/cef_app.h>

int main(int argc, char *argv[]) {
    CefMainArgs args(argc, argv);
    return CefExecuteProcess(args, 0, 0);
}
