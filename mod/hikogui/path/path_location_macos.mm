// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "strings.hpp"
#include "../utility/module.hpp"
#include "url_parser.hpp"
#include "../macros.hpp"
#include <regex>

#import <Foundation/Foundation.h>
#include <unistd.h>
#include <mach-o/dyld.h>



namespace hi::inline v1 {

URL URL::urlFromCurrentWorkingDirectory() noexcept
{
    char currentDirectory[MAXPATHLEN];
    if (getcwd(currentDirectory, MAXPATHLEN) == NULL) {
        switch (errno) {
        case ENOENT:
            // Directory no longer exists.
            return URL("none:");
        default:
            hi_no_default();
        }
    }
    return URL::urlFromPath(currentDirectory);
}

URL URL::urlFromExecutableFile() noexcept
{
    uint32_t executablePathLength = MAXPATHLEN;

    char *executablePath = new char[executablePathLength];

    if (_NSGetExecutablePath(executablePath, &executablePathLength) == -1) {
        // Not enough room, try again.
        delete [] executablePath;
        executablePath = new char[executablePathLength];

        if (_NSGetExecutablePath(executablePath, &executablePathLength) == -1) {
            hi_no_default();
        }
    }

    auto url = URL::urlFromPath(executablePath);

    delete [] executablePath;
    return url;
}

URL URL::urlFromResourceDirectory() noexcept
{
    @autoreleasepool {
        NSURL *url = [NSBundle.mainBundle resourceURL];
        return URL::urlFromPath(url.absoluteString.UTF8String);
    }
}

URL URL::urlFromApplicationDataDirectory() noexcept
{
    @autoreleasepool {
        NSArray *urls = [NSFileManager.defaultManager
            URLsForDirectory:NSApplicationSupportDirectory
            inDomains:NSUserDomainMask
        ];
        NSURL *url = urls.firstObject;
        return URL::urlFromPath(url.absoluteString.UTF8String);
    }
}

URL URL::urlFromSystemfontDirectory() noexcept
{
    return URL("file:/System/Library/fonts");
}

}
