//
//  OGLErrorCheck.cpp
//  SBS24
//
//  Created by chenyingjing on 07/06/2017.
//  Copyright © 2017 cyj. All rights reserved.
//

#include "OGLErrorCheck.h"

void checkError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        printf("OpenGL Error: %d\n", error);
}
