//
//  filePath.m
//  OpenGL01
//
//  Created by aa64mac on 5/15/16.
//  Copyright © 2016 cyj. All rights reserved.
//

#include <iostream>
#import <Foundation/Foundation.h>

// returns the full path to the file `fileName` in the resources directory of the app bundle
std::string ResourcePath(std::string fileName) {
    NSString* fname = [NSString stringWithCString:fileName.c_str() encoding:NSUTF8StringEncoding];
    NSString* path = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:fname];
    return std::string([path cStringUsingEncoding:NSUTF8StringEncoding]);
}