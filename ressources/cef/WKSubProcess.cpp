#include <include/cef_app.h>

int main(int argc, char *argv[]) {
    CefMainArgs args;
    return CefExecuteProcess(args, 0, 0);
}
