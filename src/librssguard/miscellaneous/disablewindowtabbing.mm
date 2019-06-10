
#import <AppKit/AppKit.h>

// Disables auto window tabbing where supported, otherwise a no-op.
// See http://lists.qt-project.org/pipermail/interest/2016-September/024488.html
void disableWindowTabbing()
{
    //if ([NSWindow respondsToSelector:@selector(allowsAutomaticWindowTabbing)]) {
    //    NSWindow.allowsAutomaticWindowTabbing = NO;
    //}
}
