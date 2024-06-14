#define CEF_ENABLE_SANDBOX 0

#include <include/cef_app.h>
#include <include/wrapper/cef_library_loader.h>
#include <vector>

int main(int argc, char *argv[]) {
    // Load the CEF framework library
    CefScopedLibraryLoader library_loader;
    if (!library_loader.LoadInHelper()) {
      return 1;
    }

    CefMainArgs args(argc, argv);
    return CefExecuteProcess(args, nullptr, nullptr);
}
