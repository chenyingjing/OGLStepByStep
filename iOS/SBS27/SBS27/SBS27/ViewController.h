//
//  ViewController.h
//  OpenGLES0204
//
//  Created by aa64mac on 10/01/2017.
//  Copyright © 2017 cyj. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "OpenGLView.h"

@interface ViewController : UIViewController

@property (weak, nonatomic) IBOutlet OpenGLView *openGLView;
@property (weak, nonatomic) IBOutlet UIButton *leftButton;
@property (weak, nonatomic) IBOutlet UIButton *downButton;
@property (weak, nonatomic) IBOutlet UIButton *upButton;
@property (weak, nonatomic) IBOutlet UIButton *rightButton;
@property (weak, nonatomic) IBOutlet UIButton *backwardButton;
@property (weak, nonatomic) IBOutlet UIButton *forwardButton;
@property (weak, nonatomic) IBOutlet UIButton *lightPositionButton;
@property (weak, nonatomic) IBOutlet UIButton *lightRedButton;
@property (weak, nonatomic) IBOutlet UIButton *lightBlueButton;
@property (weak, nonatomic) IBOutlet UIButton *lightWhiteButton;
@property (weak, nonatomic) IBOutlet UISlider *deltaSlider;

@end

