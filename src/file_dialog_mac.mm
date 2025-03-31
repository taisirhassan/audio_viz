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
        
        // Set up allowed file types using modern API if available
        if (@available(macOS 12.0, *)) {
            NSMutableArray* allowedTypes = [NSMutableArray array];
            [allowedTypes addObject:UTTypeAudio];
            [allowedTypes addObject:[UTType typeWithFilenameExtension:@"wav"]];
            [allowedTypes addObject:[UTType typeWithFilenameExtension:@"mp3"]];
            [allowedTypes addObject:[UTType typeWithFilenameExtension:@"ogg"]];
            openDlg.allowedContentTypes = allowedTypes;
        } else {
            // Fallback for older macOS versions
            [openDlg setAllowedFileTypes:@[@"wav", @"mp3", @"ogg", @"aif", @"aiff", @"m4a"]];
        }
        
        // Set up the file type popup
        [openDlg setTitle:@"Choose Audio File"];
        [openDlg setMessage:@"Please select an audio file to visualize"];
        [openDlg setPrompt:@"Choose"];
        
        if ([openDlg runModal] == NSModalResponseOK) {
            NSURL* fileUrl = [openDlg URL];
            return std::string([[fileUrl path] UTF8String]);
        }
        return "";
    }
}