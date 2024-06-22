#include <include/cef_app.h>
#include <include/wrapper/cef_library_loader.h>

int main(int argc, char* argv[]) {
	CefScopedLibraryLoader library_loader;
	if (!library_loader.LoadInHelper()) { return 1; }

	CefMainArgs args(argc, argv);
	return CefExecuteProcess(args, nullptr, nullptr);
}
