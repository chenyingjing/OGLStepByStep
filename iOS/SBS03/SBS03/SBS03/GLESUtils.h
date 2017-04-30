//
//  GLESUtils.h
//  Tutorial02
//
//  Created by aa64mac on 05/11/2016.
//  Copyright © 2016 cyj. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <OpenGLES/ES2/gl.h>

@interface GLESUtils : NSObject

// Create a shader object, load the shader source string, and compile the shader.
//
+(GLuint)loadShader:(GLenum)type withString:(NSString *)shaderString;

+(GLuint)loadShader:(GLenum)type withFilepath:(NSString *)shaderFilepath;

@end
