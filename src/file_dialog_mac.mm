#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <string>

@interface FileDialogDelegate : NSObject <NSOpenSavePanelDelegate>
@property (nonatomic, copy) NSArray<NSString*>* allowedExtensions;
@end

@implementation FileDialogDelegate
- (BOOL)panel:(NSOpenPanel*)panel shouldShowFilename:(NSString*)filename {
    NSString* extension = [filename pathExtension].lowercaseString;
    return [self.allowedExtensions containsObject:extension];
}
@end

std::string openFileDialog() {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    if (@available(macOS 12.0, *)) {
        // Modern API for macOS 12.0 and later
        NSArray* contentTypes = @[
            UTTypeAudio,
            UTTypeMP3,
            UTTypeWAV
        ];
        [openDlg setAllowedContentTypes:contentTypes];
    } else {
        // Fallback for older macOS versions
        [openDlg setAllowedFileTypes:@[@"wav", @"mp3", @"ogg"]];
    }
    
    [openDlg setCanChooseFiles:YES];
    [openDlg setCanChooseDirectories:NO];
    
    if ([openDlg runModal] == NSModalResponseOK) {
        NSURL* url = [[openDlg URLs] objectAtIndex:0];
        return std::string([[url path] UTF8String]);
    }
    
    return "";
}