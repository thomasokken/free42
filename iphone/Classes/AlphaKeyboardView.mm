/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#import <sys/time.h>

#import "AlphaKeyboardView.h"
#import "RootViewController.h"
#import "CalcView.h"
#import "core_main.h"


#define SPEC_LAST -1
#define SPEC_NONE 0
#define SPEC_SHIFT 1
#define SPEC_BACKSPACE 2
#define SPEC_ALT 3
#define SPEC_ESC 4
#define SPEC_SPACE 5
#define SPEC_RS 6
#define SPEC_ENTER 7

struct key {
    int x, y, w, h;
    int special;
    const char *normal, *shifted, *num, *sym;
};

const key kbMap[] = {
    {   9,   7,  92, 66, SPEC_NONE, "Q", "q", "1", "[" },
    { 113,   7,  94, 66, SPEC_NONE, "W", "w", "2", "]" },
    { 219,   7,  94, 66, SPEC_NONE, "Eᴇ", "e", "3", "{" },
    { 325,   7,  94, 66, SPEC_NONE, "R", "r", "4", "}" },
    { 431,   7,  94, 66, SPEC_NONE, "T", "t", "5", "#" },
    { 537,   7,  92, 66, SPEC_NONE, "Yʏ", "y", "6", "%" },
    { 641,   7,  94, 66, SPEC_NONE, "UÜ", "u", "7", "↑↓←→↵" },
    { 747,   7,  94, 66, SPEC_NONE, "I", "i", "8", "*×" },
    { 853,   7,  94, 66, SPEC_NONE, "OÖ", "o", "9", "+" },
    { 959,   7,  94, 66, SPEC_NONE, "P", "pπ", "0°", "=≠" },
    {  60,  87,  94, 66, SPEC_NONE, "AÄÅÆ", "a", "-•", "_" },
    { 166,  87,  94, 66, SPEC_NONE, "SΣ", "s", "/\\÷", "\\" },
    { 272,  87,  94, 66, SPEC_NONE, "D", "d", ":∶", "|├" },
    { 378,  87,  94, 66, SPEC_NONE, "F", "f", ";", "~" },
    { 484,  87,  94, 66, SPEC_NONE, "G", "g", "(", "<≤" },
    { 590,  87,  94, 66, SPEC_NONE, "H", "h", ")", ">≥" },
    { 696,  87,  94, 66, SPEC_NONE, "J", "j", "√∫", "∡▸▒" },
    { 802,  87,  94, 66, SPEC_NONE, "K", "k", "&", "&" },
    { 908,  87,  94, 66, SPEC_NONE, "L", "l", "@", "@" },
    {   9, 167, 126, 66, SPEC_SHIFT },
    { 166, 167,  94, 66, SPEC_NONE, "Z", "z", ".…", ".…" },
    { 272, 167,  94, 66, SPEC_NONE, "X", "x", ",", "," },
    { 378, 167,  94, 66, SPEC_NONE, "C", "c", "?¿", "?¿" },
    { 484, 167,  94, 66, SPEC_NONE, "V", "v", "!", "!" },
    { 590, 167,  94, 66, SPEC_NONE, "B", "b", "'`", "'`" },
    { 696, 167,  94, 66, SPEC_NONE, "NÑ", "n", "\"", "\"" },
    { 802, 167,  94, 66, SPEC_NONE, "M", "mμ", "$£", "£" },
    { 927, 167, 126, 66, SPEC_BACKSPACE },
    {   9, 247, 126, 66, SPEC_ALT },
    { 147, 247, 126, 66, SPEC_ESC },
    { 285, 247, 446, 66, SPEC_SPACE },
    { 743, 247, 149, 66, SPEC_RS },
    { 904, 247, 149, 66, SPEC_ENTER },
    {   0,   0,   0,  0, SPEC_LAST }
};

#define KB_WIDTH 1062
#define KB_HEIGHT 320

int findKey(CGPoint p, CGFloat xs, CGFloat ys) {
    int x = p.x / xs;
    int y = p.y / ys;
    CGFloat bestDistance = CGFLOAT_MAX;
    int bestKey = -1;
    int n = -1;
    for (const key *k = kbMap; k->special != SPEC_LAST; k++) {
        n++;
        if (x >= k->x && y >= k->y && x < k->x + k->w && y < k->y + k->h)
            return n;
        CGFloat xd = xs * (x < k->x ? k->x - x : x >= k->x + k->w ? x + 1 - k->x - k->w : 0);
        CGFloat yd = ys * (y < k->y ? k->y - y : y >= k->y + k->h ? y + 1 - k->y - k->h : 0);
        CGFloat d = xd == 0 ? yd : yd == 0 ? xd : (int) (sqrt(xd * xd + yd * yd) + 0.5);
        if (d < bestDistance) {
            bestKey = n;
            bestDistance = d;
        }
    }
    return bestDistance < 7 ? bestKey : -1;
}

int millitime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


@implementation AlphaKeyboardView {
    CGRect kbRect;
    CGRect safeRect;
    bool portrait;
    int currentKey;
    bool keyExpanded;
    int shiftReleased;
    bool num;
    bool shift;
    bool lock;
    bool expKeyRectValid;
    CGRect expKeyRect;
    CGRect expBubbleRect;
    CGFloat expOffset;
    CGFloat expSpacing;
    int expCurrentIndex;
    int expMaxIndex;
    bool expStillOnKey;
}

- (id) initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (void) awakeFromNib {
    [super awakeFromNib];
    self.multipleTouchEnabled = false;
    self.backgroundColor = UIColor.clearColor;
}

- (void) raised {
    currentKey = -1;
    num = false;
    shift = false;
    lock = false;
    [self setNeedsDisplay];
}

- (void) layoutSubviews {
    kbRect = self.bounds;
    portrait = self.bounds.size.width < self.bounds.size.height;
    CGFloat ah = kbRect.size.height / (portrait ? 3 : 2);
    kbRect.origin.y += kbRect.size.height - ah;
    kbRect.size.height = ah;

    if (portrait) {
        int bm = [RootViewController getBottomMargin];
        safeRect = kbRect;
        kbRect.origin.y -= bm;
        safeRect.origin.y -= bm;
        safeRect.size.height += bm;
    } else {
        safeRect = CGRectMake(0, 0, 0, 0);
    }
    [self setNeedsDisplay];
}

void set_color_comps(CGFloat *comps, int rgb) {
    comps[0] = ((rgb >> 16) & 255) / 255.0;
    comps[1] = ((rgb >> 8) & 255) / 255.0;
    comps[2] = (rgb & 255) / 255.0;
}

- (void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    [super traitCollectionDidChange:previousTraitCollection];
    [self setNeedsDisplay];
}

const CGFloat bRadius = 5;

- (void) drawRect:(CGRect)rect {
    bool dark = UIScreen.mainScreen.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark;
    CGColorRef back, cap, grayCap, blueCap, shadow, text;
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    CGFloat comps[4];
    comps[3] = 1.0;
    if (dark) {
        set_color_comps(comps, 0x202020); back = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x585858); cap = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0xffffff); text = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x141414); shadow = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x363636); grayCap = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x0a60ff); blueCap = CGColorCreate(color_space, comps);
    } else {
        set_color_comps(comps, 0xc5c9d1); back = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0xffffff); cap = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x000000); text = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x74767b); shadow = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x999fae); grayCap = CGColorCreate(color_space, comps);
        set_color_comps(comps, 0x0a60ff); blueCap = CGColorCreate(color_space, comps);
    }
    CGColorSpaceRelease(color_space);
    
    CGContextRef myContext = UIGraphicsGetCurrentContext();
    CGFloat xs = kbRect.size.width / KB_WIDTH;
    CGFloat ys = kbRect.size.height / KB_HEIGHT;

    CGContextSetFillColorWithColor(myContext, back);
    CGContextFillRect(myContext, kbRect);
    CGContextFillRect(myContext, safeRect);

    int kn = -1;
    for (const key *k = kbMap; k->special != SPEC_LAST; k++) {
        kn++;
        bool active = kn == currentKey;
        const int sw = 1;
        CGRect r = CGRectMake(k->x * xs + kbRect.origin.x, k->y * ys + kbRect.origin.y + sw, k->w * xs, k->h * ys - sw);
        UIBezierPath *path = [UIBezierPath bezierPathWithRoundedRect:r cornerRadius:bRadius];
        CGContextSetFillColorWithColor(myContext, shadow);
        [path fill];
        r.origin.y -= sw;

        if (k->special == SPEC_NONE) {
            const char *chars = num ? shift ? k->sym : k->num : shift ? k->shifted : k->normal;
            CGContextSetFillColorWithColor(myContext, cap);
            if (!active) {
                [self drawCharKey:r forChar:chars color:text];
            } else if (!keyExpanded) {
                [self drawSmallBubble:r forChar:chars shadowColor:shadow textColor:text];
            } else {
                NSString *s = [NSString stringWithUTF8String:chars];
                [self drawLargeBubble:r forChars:s shadowColor:shadow textColor:text blueColor:blueCap];
            }
        } else {
            path = [UIBezierPath bezierPathWithRoundedRect:r cornerRadius:bRadius];
            if (k->special == SPEC_RS || k->special == SPEC_ENTER)
                CGContextSetFillColorWithColor(myContext, active ? cap : blueCap);
            else if (k->special == SPEC_SHIFT && shift && !num)
                CGContextSetFillColorWithColor(myContext, cap);
            else if (k->special == SPEC_SHIFT || k->special == SPEC_ALT)
                CGContextSetFillColorWithColor(myContext, grayCap);
            else
                CGContextSetFillColorWithColor(myContext, active ? cap : grayCap);
            [path fill];
            CGFloat fontSize;
            if ((!num && k->special == SPEC_SHIFT) || k->special == SPEC_BACKSPACE)
                fontSize = r.size.height / 1.7;
            else
                fontSize = r.size.height / 2.5;
            NSMutableDictionary *atts = [NSMutableDictionary dictionary];
            UIFont *font = [UIFont systemFontOfSize:fontSize];
            [atts setObject:font forKey:NSFontAttributeName];
            NSString *label = nil;
            switch (k->special) {
                case SPEC_SHIFT: label = num ? shift ? @"123" : @"#+=" : lock ? @"\u21ea" : @"\u21e7"; break;
                case SPEC_BACKSPACE: label = @"\u232b"; break;
                case SPEC_ALT: label = num ? @"ABC" : @"123"; break;
                case SPEC_ESC: label = @"Esc"; break;
                case SPEC_SPACE: label = @"Space"; break;
                case SPEC_RS: label = @"R\u2009/\u2009S"; break;
                case SPEC_ENTER: label = @"Enter"; break;
            }
            if (label == nil)
                continue;
            if (!active && (k->special == SPEC_RS || k->special == SPEC_ENTER))
                [atts setObject:UIColor.whiteColor forKey:NSForegroundColorAttributeName];
            else
                [atts setObject:[UIColor colorWithCGColor:text] forKey:NSForegroundColorAttributeName];
            CGSize size = [label sizeWithAttributes:atts];
            [label drawAtPoint:CGPointMake(r.origin.x + (r.size.width - size.width) / 2, r.origin.y + (r.size.height - font.ascender + font.descender) / 2) withAttributes:atts];
        }
    }
}

- (void) drawCharKey:(CGRect)r forChar:(const char *)c color:(CGColorRef)color {
    UIBezierPath *path = [UIBezierPath bezierPathWithRoundedRect:r cornerRadius:bRadius];
    [path fill];
    char bits[5];
    core_get_char_pixels(c, bits);
    CGContext *myContext = UIGraphicsGetCurrentContext();
    CGContextSetFillColorWithColor(myContext, color);
    CGFloat ps = r.size.height / (portrait ? 16 : 12);
    CGFloat x = r.origin.x + (r.size.width - 5 * ps) / 2;
    CGFloat y = r.origin.y + (r.size.height - 8 * ps) / 2;
    for (int v = 0; v < 8; v++)
        for (int h = 0; h < 5; h++)
            if (((bits[h] >> v) & 1) != 0)
                CGContextFillRect(myContext, CGRectMake(x + h * ps, y + v * ps, ps, ps));
}

- (UIBezierPath *) makeBubble:(CGRect)r width:(CGFloat)width cpos:(CGPoint *)cpos {
    CGFloat bl, br;
    CGFloat ew = r.size.width * 0.15;
    if (ew < 2 * bRadius)
        ew = 2 * bRadius;
    bl = r.origin.x - ew;
    if (width == 0) {
        br = r.origin.x + r.size.width + ew;
        width = br - bl;
    } else
        br = bl + width;
    bool leftFlush = false;
    bool rightFlush = false;

    if (bl < 5) {
        bl = 5;
        br = 5 + width;
        if (bl > r.origin.x - 2 * bRadius) {
            bl = r.origin.x;
            br = bl + width;
            leftFlush = true;
        }
    } else if (br > self.bounds.size.width - 5) {
        br = self.bounds.size.width - 5;
        bl = br - width;
        if (br < r.origin.x + r.size.width + 2 * bRadius) {
            br = r.origin.x + r.size.width;
            bl = br - width;
            rightFlush = true;
        }
    }
    
    CGFloat bb = r.origin.y - r.size.height * 7 / 40;
    CGFloat bt = bb - 1.3 * r.size.height * (portrait ? 1 : 1.333);
    CGFloat R = bRadius;
    
    expKeyRect = CGRectMake(r.origin.x, bb, r.size.width, r.origin.y + r.size.height - bb);
    expKeyRectValid = true;
    CGFloat ext = bb - bt;
    expBubbleRect = CGRectMake(bl - ext, bt - ext, br - bl + 2 * ext, bb - bt + 2 * ext);

    UIBezierPath *path = [UIBezierPath bezierPath];
    [path moveToPoint:CGPointMake(bl, bt + R)];
    [path addArcWithCenter:CGPointMake(bl + R, bt + R) radius:R startAngle:M_PI endAngle:M_PI * 1.5 clockwise:YES];
    [path addLineToPoint:CGPointMake(br - R, bt)];
    [path addArcWithCenter:CGPointMake(br - R, bt + R) radius:R startAngle:M_PI * 1.5 endAngle:0 clockwise:YES];
    if (rightFlush) {
        [path addLineToPoint:CGPointMake(br, r.origin.y + r.size.height - R)];
    } else {
        [path addLineToPoint:CGPointMake(br, bb - R)];
        [path addArcWithCenter:CGPointMake(br - R, bb - R) radius:R startAngle:0 endAngle:M_PI / 2 clockwise:YES];
        [path addLineToPoint:CGPointMake(r.origin.x + r.size.width + R, bb)];
        [path addArcWithCenter:CGPointMake(r.origin.x + r.size.width + R, bb + R) radius:R startAngle:M_PI * 1.5 endAngle:M_PI clockwise:NO];
        [path addLineToPoint:CGPointMake(r.origin.x + r.size.width, r.origin.y + r.size.height - R)];
    }
    [path addArcWithCenter:CGPointMake(r.origin.x + r.size.width - R, r.origin.y + r.size.height - R) radius:R startAngle:0 endAngle:M_PI / 2 clockwise:YES];
    [path addLineToPoint:CGPointMake(r.origin.x + R, r.origin.y + r.size.height)];
    [path addArcWithCenter:CGPointMake(r.origin.x + R, r.origin.y + r.size.height - R) radius:R startAngle:M_PI / 2 endAngle:M_PI clockwise:YES];
    if (!leftFlush) {
        [path addLineToPoint:CGPointMake(r.origin.x, bb + R)];
        [path addArcWithCenter:CGPointMake(r.origin.x - R, bb + R) radius:R startAngle:0 endAngle:M_PI * 1.5 clockwise:NO];
        [path addLineToPoint:CGPointMake(bl + R, bb)];
        [path addArcWithCenter:CGPointMake(bl + R, bb - R) radius:R startAngle:M_PI /2 endAngle:M_PI clockwise:YES];
    }
    [path closePath];
    
    cpos->x = (bl + br) / 2;
    cpos->y = (bt + bb) / 2;
    return path;
}

- (void) drawSmallBubble:(CGRect)r forChar:(const char *)c shadowColor:(CGColorRef)shadowColor textColor:(CGColorRef)textColor {
    CGFloat width = r.size.width * 1.3;
    if (width < r.size.width + 4 * bRadius)
        width = r.size.width + 4 * bRadius;
    CGPoint cpos;
    UIBezierPath *path = [self makeBubble:r width:0 cpos:&cpos];
    [path fill];
    CGContext *myContext = UIGraphicsGetCurrentContext();
    CGContextSetStrokeColorWithColor(myContext, shadowColor);
    [path setLineWidth:1];
    [path stroke];
    char bits[5];
    core_get_char_pixels(c, bits);
    CGContextSetFillColorWithColor(myContext, textColor);
    CGFloat ps = r.size.height * 1.3 / (portrait ? 16 : 12);
    CGFloat x = cpos.x - 2.5 * ps;
    CGFloat y = cpos.y - 4 * ps;
    for (int v = 0; v < 8; v++)
        for (int h = 0; h < 5; h++)
            if (((bits[h] >> v) & 1) != 0)
                CGContextFillRect(myContext, CGRectMake(x + h * ps, y + v * ps, ps, ps));
}

- (void) drawLargeBubble:(CGRect)r forChars:(NSString *)s shadowColor:(CGColorRef)shadowColor textColor:(CGColorRef)textColor blueColor:(CGColorRef)blueColor {
    CGFloat ps = r.size.height / (portrait ? 16 : 12);
    CGFloat space = ps * 5 * (portrait ? 1 : 2);
    CGFloat width = (ps * 5 + space) * [s length] + space + 2 * bRadius;
    if (width < r.size.width + 4 * bRadius)
        width = r.size.width + 4 * bRadius;
    CGPoint cpos;
    UIBezierPath *path = [self makeBubble:r width:width cpos:&cpos];
    [path fill];
    CGContext *myContext = UIGraphicsGetCurrentContext();
    CGContextSetStrokeColorWithColor(myContext, shadowColor);
    [path setLineWidth:1];
    [path stroke];
    
    expSpacing = 5 * ps + space;
    expOffset = cpos.x - ([s length] - 1) * expSpacing / 2;
    expMaxIndex = [s length] - 1;
    CGFloat hw = expSpacing * 0.9;
    path = [UIBezierPath bezierPathWithRoundedRect:CGRectMake(expOffset + expCurrentIndex * expSpacing - hw / 2, cpos.y - 6 * ps, hw, 12 * ps) cornerRadius:ps];
    CGContextSetFillColorWithColor(myContext, blueColor);
    [path fill];
    char bits[5];
    for (int i = 0; i < [s length]; i++) {
        core_get_char_pixels([[s substringWithRange:NSMakeRange(i, 1)] UTF8String], bits);
        CGContextSetFillColorWithColor(myContext, i == expCurrentIndex ? UIColor.whiteColor.CGColor : textColor);
        CGFloat x = expOffset + i * expSpacing - 2.5 * ps;
        CGFloat y = cpos.y - 4 * ps;
        for (int v = 0; v < 8; v++)
            for (int h = 0; h < 5; h++)
                if (((bits[h] >> v) & 1) != 0)
                    CGContextFillRect(myContext, CGRectMake(x + h * ps, y + v * ps, ps, ps));
    }
}

- (BOOL) pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    // Pass events outside the keyboard rect to the CalcView
    return CGRectContainsPoint(kbRect, point);
}

- (void) touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(expandBubble) object:nil];

    CGFloat xs = kbRect.size.width / KB_WIDTH;
    CGFloat ys = kbRect.size.height / KB_HEIGHT;
    UITouch *touch = [touches anyObject];
    CGPoint p = [touch locationInView:self];
    p.x -= kbRect.origin.x;
    p.y -= kbRect.origin.y;
    
    int kn = findKey(p, xs, ys);
    if (kn == -1)
        return;
    currentKey = kn;
    keyExpanded = false;
    expKeyRectValid = false;
    expStillOnKey = true;
    [CalcView keyFeedback];
    
    const key *k = kbMap + kn;
    if (k->special == SPEC_ALT) {
        num = !num;
        shift = false;
        lock = false;
        goto done;
        done2:
        if (!lock && !num)
            shift = false;
        done:
        [self setNeedsDisplay];
        return;
    }
    if (k->special == SPEC_SHIFT) {
        if (!num && shiftReleased != 0 && millitime() < shiftReleased + 250) {
            shift = true;
            lock = true;
        } else {
            shift = !shift;
            lock = false;
        }
        goto done;
    }
    if (k->special == SPEC_BACKSPACE || k->special == SPEC_RS) {
        int keyCode = k->special == SPEC_BACKSPACE ? 17 : 36;
        [CalcView alphaKeyboardDown:keyCode];
        goto done2;
    }
    if (k->special == SPEC_SPACE) {
        [CalcView alphaKeyboardAlpha:' '];
        goto done2;
    }
    if (k->special == SPEC_ESC) {
        if (shift && !lock && !num)
            [CalcView alphaKeyboardAlpha:27]; // [ESC] character
        else
            [CalcView alphaKeyboardDown:33]; // EXIT key
        goto done2;
    }
    if (k->special == SPEC_ENTER) {
        if (shift && !lock && !num)
            [CalcView alphaKeyboardAlpha:10]; // [LF] character
        else
            [CalcView alphaKeyboardDown:13]; // ENTER key
        goto done2;
    }
    
    /* At this point, the key must be a normal character key.
     * We don't type the characters on key-down; we make bubbles
     * appear on the screen, and type a character only when the
     * user ends the touch in the right spot.
     */
    const char *keyText = num ? shift ? k->sym : k->num : shift ? k->shifted : k->normal;
    NSString *s = [NSString stringWithUTF8String:keyText];
    if ([s length] > 1)
        [self performSelector:@selector(expandBubble) withObject:nil afterDelay:0.375];
    goto done;
}

- (void) touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(expandBubble) object:nil];
    
    if (currentKey == -1)
        return;
    
    if (false) {
        done:
        currentKey = -1;
        shiftReleased = 0;
        [self setNeedsDisplay];
        return;
    }
    
    if (currentKey != -1 && kbMap[currentKey].special == SPEC_NONE) {
        if (!keyExpanded)
            expCurrentIndex = 0;
        const key *k = kbMap + currentKey;
        const char *chars = num ? shift ? k->sym : k->num : shift ? k->shifted : k->normal;
        NSString *s = [NSString stringWithUTF8String:chars];
        unsigned short c = [s characterAtIndex:expCurrentIndex];
        [CalcView alphaKeyboardAlpha:c];
        [CalcView alphaKeyboardUp];
        if (!lock && !num)
            shift = false;
        goto done;
    }

    const key *k = kbMap + currentKey;
    if (k->special == SPEC_SHIFT) {
        shiftReleased = num ? 0 : millitime();
        currentKey = -1;
        [self setNeedsDisplay];
        return;
    }
    if (k->special != SPEC_NONE) {
        [CalcView alphaKeyboardUp];
        goto done;
    }
    
    if (!lock && !num)
        shift = false;
    goto done;
}

- (void) touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if (currentKey == -1 || kbMap[currentKey].special != SPEC_NONE || !expKeyRectValid)
        return;
    CGPoint p = [[touches anyObject] locationInView:self];
    if (p.y > expKeyRect.origin.y + expKeyRect.size.height * (portrait ? 1 : 1.333) + 5) {
        currentKey = -1;
        [self setNeedsDisplay];
        return;
    }
    if (!keyExpanded)
        return;
    int idx = expCurrentIndex;
    if (expStillOnKey && CGRectContainsPoint(expKeyRect, p)) {
        idx = 0;
    } else {
        expStillOnKey = false;
        if (CGRectContainsPoint(expBubbleRect, p)) {
            idx = (int) ((p.x - expOffset) / expSpacing + 0.5);
            if (idx < 0)
                idx = 0;
            else if (idx > expMaxIndex)
                idx = expMaxIndex;
        }
    }
    if (expCurrentIndex != idx) {
        [self setNeedsDisplay];
        expCurrentIndex = idx;
    }
}

- (void) expandBubble {
    keyExpanded = true;
    expCurrentIndex = 0;
    [self setNeedsDisplay];
}

@end
