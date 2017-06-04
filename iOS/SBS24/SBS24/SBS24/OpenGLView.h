//
//  OpenGLView.h
//  Tutorial01
//
//  Created by aa64mac on 01/11/2016.
//  Copyright © 2016 cyj. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

@interface OpenGLView : UIView {
    CAEAGLLayer* _eaglLayer;
    EAGLContext* _context;
    GLuint _colorRenderBuffer;
    GLuint _frameBuffer;
    GLuint _depthRenderBuffer;
    GLuint _programHandle;
    GLuint _positionSlot;
    
}

@property (weak, nonatomic) UIButton *leftButton;
@property (weak, nonatomic) UIButton *backwardButton;
@property (weak, nonatomic) UIButton *forwardButton;
@property (weak, nonatomic) UIButton *rightButton;
@property (weak, nonatomic) UIButton *downButton;
@property (weak, nonatomic) UIButton *upButton;
@property (weak, nonatomic) UIButton *lightPositionButton;
@property (weak, nonatomic) UIButton *lightRedButton;
@property (weak, nonatomic) UIButton *lightBlueButton;
@property (weak, nonatomic) UIButton *lightWhiteButton;

@property float moveX;
@property float moveY;
@property float scale;

@end
