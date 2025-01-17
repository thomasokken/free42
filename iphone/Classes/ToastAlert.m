//
//  ToastAlert.h
//  Free42
//
//  Created by htafoya on 16-01-2013.
//


#import "ToastAlert.h"
#import <QuartzCore/QuartzCore.h>

@implementation ToastAlert

#define POPUP_DELAY  1.5

- (id)initWithText: (NSString*) msg
{

    self = [super init];
    if (self) {

        self.backgroundColor = [UIColor colorWithWhite:0 alpha:0.7];
        self.textColor = [UIColor colorWithWhite:1 alpha: 0.95];
        self.font = [UIFont fontWithName: @"Helvetica-Bold" size: 13];
        self.text = msg;
        self.numberOfLines = 0;
        self.textAlignment = UITextAlignmentCenter;
        self.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;


    }
    return self;
}

- (void)didMoveToSuperview {

    UIView* parent = self.superview;

    if(parent) {

        CGSize maximumLabelSize = CGSizeMake(300, 200);
        CGSize expectedLabelSize = [self.text sizeWithFont: self.font  constrainedToSize:maximumLabelSize lineBreakMode: NSLineBreakByTruncatingTail];

        expectedLabelSize = CGSizeMake(expectedLabelSize.width + 20, expectedLabelSize.height + 10);

        self.frame = CGRectMake(parent.center.x - expectedLabelSize.width/2,
                                expectedLabelSize.height + 10,
                                expectedLabelSize.width,
                                expectedLabelSize.height);

        CALayer *layer = self.layer;
        layer.cornerRadius = 4.0f;

        [self performSelector:@selector(dismiss:) withObject:nil afterDelay:POPUP_DELAY];
    }
}

- (void)dismiss:(id)sender {
    // Fade out the message and destroy self
    [UIView animateWithDuration:0.6  delay:0 options: UIViewAnimationOptionAllowUserInteraction
                     animations:^  { self.alpha = 0; }
                     completion:^ (BOOL finished) { [self removeFromSuperview]; }];
}

@end
