#include "VRCocoaWindow.h"

#include <OpenSG/OSGCocoaWindow.h>
#import <Cocoa/Cocoa.h>

// This prevents warnings that "NSApplication might not
// respond to setAppleMenu" on OS X 10.4
@interface NSApplication(OpenSG)
- (void)setAppleMenu:(NSMenu *)menu;
@end

using namespace OSG;

VRCocoaWindow::VRCocoaWindow() { init(); }
VRCocoaWindow::~VRCocoaWindow() {}

VRCocoaWindowPtr VRCocoaWindow::create() { return VRCocoaWindowPtr( new VRCocoaWindow() ); }
VRCocoaWindowPtr VRCocoaWindow::ptr() { return static_pointer_cast<VRCocoaWindow>(shared_from_this()); }

void VRCocoaWindow::render(bool fromThread) {
  if (fromThread) return;
  VRWindow::render();
}

void VRCocoaWindow::save(XMLElementPtr node) {}
void VRCocoaWindow::load(XMLElementPtr node) {}

void VRCocoaWindow::onDisplay() {}
void VRCocoaWindow::onMouse(int b, int s, int x, int y) {}
void VRCocoaWindow::onMotion(int x, int y) {}
void VRCocoaWindow::onKeyboard(int k, int s, int x, int y) {}
void VRCocoaWindow::onKeyboard_special(int k, int s, int x, int y) {}

CocoaWindowUnrecPtr cocoaWin;
VRCocoaWindow* vrCocoaWin = 0;

void osx_AllowForeground() {
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
    SetFrontProcess(&psn);
}

@interface MyOpenGLView: NSOpenGLView
{
}
- (BOOL) acceptsFirstResponder;

- (void) handleMouseEvent: (NSEvent*) event;

- (void) mouseDown: (NSEvent*) event;
- (void) mouseDragged: (NSEvent*) event;
- (void) mouseUp: (NSEvent*) event;
- (void) rightMouseDown: (NSEvent*) event;
- (void) rightMouseDragged: (NSEvent*) event;
- (void) rightMouseUp: (NSEvent*) event;
- (void) otherMouseDown: (NSEvent*) event;
- (void) otherMouseDragged: (NSEvent*) event;
- (void) otherMouseUp: (NSEvent*) event;

- (void) keyDown: (NSEvent*) event;

- (void) reshape;
- (void) drawRect: (NSRect) bounds;
@end

@implementation MyOpenGLView

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) handleMouseEvent: (NSEvent*) event
{
    Real32 w,h,a,b,c,d;

    int buttonNumber = [event buttonNumber];
    unsigned int modifierFlags = [event modifierFlags];

    // Traditionally, Apple mice just have one button. It is common practice to simulate
    // the middle and the right button by pressing the option or the control key.
    if (buttonNumber == 0)
    {
        if (modifierFlags & NSAlternateKeyMask)
            buttonNumber = 2;
        if (modifierFlags & NSControlKeyMask)
            buttonNumber = 1;
    }

    NSPoint location = [event locationInWindow];

    switch ([event type])
    {
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
        break;

    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
        break;

    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged:
        break;

    default:
        break;
    }
}

- (void) mouseDown: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) mouseDragged: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) mouseUp: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) rightMouseDown: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) rightMouseDragged: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) rightMouseUp: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) otherMouseDown: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) otherMouseDragged: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) otherMouseUp: (NSEvent*) event
{
    [self handleMouseEvent: event];
}

- (void) keyDown: (NSEvent*) event
{
    if ([[event characters] length] != 1)
        return;
    switch ([[event characters] characterAtIndex: 0])
    {
    case 27:
//        [NSApp terminate:nil];
        [NSApp stop:nil];
        break;
    default:
        break;
    }
}

- (void) reshape
{
    [self update];
    NSWindow *window = [self window];
    NSRect frame = [self bounds];
    float scaleFactor = [window backingScaleFactor];
    int W = static_cast<int>(frame.size.width*scaleFactor);
    int H = static_cast<int>(frame.size.height*scaleFactor);
    cocoaWin->resize( W, H );
}

- (void) drawRect: (NSRect) bounds
{
    vrCocoaWin->render();
}

@end

@interface MyDelegate : NSObject

{
    NSWindow *window;
    MyOpenGLView *glView;
}

- (void) applicationWillFinishLaunching: (NSNotification*) notification;

- (BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication*) application;

- (void) performer: (id) userInfo;

@end

@implementation MyDelegate

- (void) performer: (id) userInfo
{
    fprintf(stderr, "perform\n");
}


- (void) dealloc
{
    [window release];
    [super dealloc];
}

- (void) applicationWillFinishLaunching: (NSNotification*) notification
{
    /* Set up the menubar */
    [NSApp setMainMenu:[[NSMenu alloc] init]];

    NSString *appName = @"testWindowCocoa";
    NSMenu *appleMenu = [[NSMenu alloc] initWithTitle:@""];

    /* Add menu items */

    NSMenu *servicesMenu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:@"Services" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:servicesMenu];
    [appleMenu addItem:menuItem];
    [NSApp setServicesMenu: servicesMenu];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    NSString *title = [@"Hide " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

    menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

    [appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Quit " stringByAppendingString:appName];
    [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

    /* Put menu into the menubar */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:menuItem];

    /* Tell the application object that this is now the application menu */
    [NSApp setAppleMenu:appleMenu];

    /* Finally give up our references to the objects */
    [appleMenu release];
    [menuItem release];

    // Create the window
    window = [NSWindow alloc];
    NSRect rect = { { 0, 0 }, { 300, 600 } };
    [window initWithContentRect: rect styleMask: (NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask) backing: NSBackingStoreBuffered defer: YES];
    [window setTitle: @"testWindowCocoa"];
    [window setReleasedWhenClosed: NO];

    glView = [[MyOpenGLView alloc] autorelease];
    [glView initWithFrame: rect];
    [glView setAutoresizingMask: NSViewMaxXMargin | NSViewWidthSizable | NSViewMaxYMargin | NSViewHeightSizable];
    [[window contentView] addSubview: glView];

    NSOpenGLPixelFormatAttribute attrs[] =
    {
        NSOpenGLPFAWindow,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, NSOpenGLPixelFormatAttribute(16),
        NSOpenGLPixelFormatAttribute(0)
    };
    NSOpenGLPixelFormat *pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attrs];
    [glView setPixelFormat: pixFmt];

    // Create OpenSG window
    cocoaWin = CocoaWindow::create();
    //win->addPort( vp );
    cocoaWin->setContext ( [glView openGLContext] );
    cocoaWin->init();
    cocoaWin->resize( 300, 300 );

    cocoaWin->activate();

    // do some OpenGL init. Will move into State Chunks later.

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

	// Show the window
    [window makeKeyAndOrderFront: nil];
    [window makeFirstResponder:glView];
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication*) application
{
    return YES;
}

@end

void doPerform(id userInfo)
{
    fprintf(stderr, "perform\n");
}

void VRCocoaWindow::init() {
    vrCocoaWin = this;

    osx_AllowForeground();

    // Create application
    [NSApplication sharedApplication];
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    MyDelegate *delegate =  [[MyDelegate new] autorelease];
    [NSApp setDelegate: delegate];

    _win = cocoaWin;
}
