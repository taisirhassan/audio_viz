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
    @autoreleasepool {
        NSOpenPanel* openDlg = [NSOpenPanel openPanel];
        [openDlg setCanChooseFiles:YES];
        [openDlg setCanChooseDirectories:NO];
        [openDlg setAllowsMultipleSelection:NO];
        
        if (@available(macOS 12.0, *)) {
            [openDlg setAllowedContentTypes:@[
                UTTypeAudio,
                [UTType typeWithFilenameExtension:@"wav"],
                [UTType typeWithFilenameExtension:@"mp3"],
                [UTType typeWithFilenameExtension:@"ogg"]
            ]];
        } else {
            FileDialogDelegate* delegate = [[FileDialogDelegate alloc] init];
            delegate.allowedExtensions = @[@"wav", @"mp3", @"ogg"];
            [openDlg setDelegate:delegate];
        }

        if ([openDlg runModal] == NSModalResponseOK) {
            NSURL* url = [openDlg URL];
            return std::string([[url path] UTF8String]);
        }
    }
    return "";
}