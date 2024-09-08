#import <Cocoa/Cocoa.h>
#include <string>

std::string openFileDialog() {
    @autoreleasepool {
        NSOpenPanel* openDlg = [NSOpenPanel openPanel];
        [openDlg setCanChooseFiles:YES];
        [openDlg setCanChooseDirectories:NO];
        [openDlg setAllowsMultipleSelection:NO];
        [openDlg setAllowedFileTypes:@[@"wav", @"mp3", @"ogg"]];

        if ([openDlg runModal] == NSModalResponseOK) {
            NSURL* url = [openDlg URL];
            return std::string([[url path] UTF8String]);
        }
    }
    return "";
}