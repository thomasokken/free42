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

package com.thomasokken.free42;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.view.View;

public class AlphaKeyboardView extends View {
    private static final int SPEC_NONE = 0;
    private static final int SPEC_SHIFT = 1;
    private static final int SPEC_BACKSPACE = 2;
    private static final int SPEC_ALT = 3;
    private static final int SPEC_ESC = 4;
    private static final int SPEC_SPACE = 5;
    private static final int SPEC_RS = 6;
    private static final int SPEC_ENTER = 7;

    private static final int KB_WIDTH = 1062;
    private static final int KB_HEIGHT = 320;

    private static class key {
        public int x, y, w, h;
        public int special;
        public String normal, shifted, num, sym;
        public key(int x, int y, int w, int h, int special) {
            this.x = x;
            this.y = y;
            this.w = w;
            this.h = h;
            this.special = special;
        }
        public key(int x, int y, int w, int h, int special, String normal, String shifted, String num, String sym) {
            this(x, y, w, h, special);
            this.normal = normal;
            this.shifted = shifted;
            this.num = num;
            this.sym = sym;
        }
    }

    private static final key[] kbMap = {
        new key(  9,   7,  92, 66, SPEC_NONE, "Q", "q", "1", "["),
        new key(113,   7,  94, 66, SPEC_NONE, "W", "w", "2", "]"),
        new key(219,   7,  94, 66, SPEC_NONE, "Eᴇ", "e", "3", "{"),
        new key(325,   7,  94, 66, SPEC_NONE, "R", "r", "4", "}"),
        new key(431,   7,  94, 66, SPEC_NONE, "T", "t", "5", "#"),
        new key(537,   7,  92, 66, SPEC_NONE, "Yʏ", "y", "6", "%"),
        new key(641,   7,  94, 66, SPEC_NONE, "UÜ", "u", "7", "↑↓←→↵"),
        new key(747,   7,  94, 66, SPEC_NONE, "I", "i", "8", "*×"),
        new key(853,   7,  94, 66, SPEC_NONE, "OÖ", "o", "9", "+"),
        new key(959,   7,  94, 66, SPEC_NONE, "P", "pπ", "0°", "=≠"),
        new key( 60,  87,  94, 66, SPEC_NONE, "AÄÅÆ", "a", "-•", "_"),
        new key(166,  87,  94, 66, SPEC_NONE, "SΣ", "s", "/\\÷", "\\"),
        new key(272,  87,  94, 66, SPEC_NONE, "D", "d", ":∶", "|├"),
        new key(378,  87,  94, 66, SPEC_NONE, "F", "f", ";", "~"),
        new key(484,  87,  94, 66, SPEC_NONE, "G", "g", "(", "<≤"),
        new key(590,  87,  94, 66, SPEC_NONE, "H", "h", ")", ">≥"),
        new key(696,  87,  94, 66, SPEC_NONE, "J", "j", "√∫", "∡▸▒"),
        new key(802,  87,  94, 66, SPEC_NONE, "K", "k", "&", "&"),
        new key(908,  87,  94, 66, SPEC_NONE, "L", "l", "@", "@"),
        new key(  9, 167, 126, 66, SPEC_SHIFT),
        new key(166, 167,  94, 66, SPEC_NONE, "Z", "z", ".…", ".…"),
        new key(272, 167,  94, 66, SPEC_NONE, "X", "x", ",", ","),
        new key(378, 167,  94, 66, SPEC_NONE, "C", "c", "?¿", "?¿"),
        new key(484, 167,  94, 66, SPEC_NONE, "V", "v", "!", "!"),
        new key(590, 167,  94, 66, SPEC_NONE, "B", "b", "'`", "'`"),
        new key(696, 167,  94, 66, SPEC_NONE, "NÑ", "n", "\"", "\""),
        new key(802, 167,  94, 66, SPEC_NONE, "M", "mμ", "$£", "£"),
        new key(927, 167, 126, 66, SPEC_BACKSPACE),
        new key(  9, 247, 126, 66, SPEC_ALT),
        new key(147, 247, 126, 66, SPEC_ESC),
        new key(285, 247, 446, 66, SPEC_SPACE),
        new key(743, 247, 149, 66, SPEC_RS),
        new key(904, 247, 149, 66, SPEC_ENTER)
    };

    Rect kbRect;
    boolean portrait;
    int currentKey;
    boolean keyExpanded;
    int shiftReleased;
    boolean num;
    boolean shift;
    boolean lock;
    Rect expKeyRect;
    Rect expBubbleRect;
    Float expOffset;
    Float expSpacing;
    int expCurrentIndex;
    int expMaxIndex;
    boolean expStillOnKey;

    /* core_get_char_pixels()
     *
     * Gets character pixels from the calculator's 8x5 characters, for the
     * given UTF-8 encoded character, provided as a null-terminated string.
     */
    private native byte[] core_get_char_pixels(char c);

    public AlphaKeyboardView(Context context) {
        super(context);
        // self.multipleTouchEnabled = false;
        setBackgroundColor(0x00000000); // transparent
    }

//    protected void onDraw(Canvas canvas) {
//        Paint paint = new Paint();
//        paint.setStyle(Paint.Style.FILL);
//        paint.setColor(Color.RED);
//        canvas.drawCircle(50, 50, 25, paint);
//    }
//
    private static int findKey(Point p, double xs, double ys) {
        int x = (int) (p.x / xs);
        int y = (int) (p.y / ys);
        double bestDistance = Double.MAX_VALUE;
        int bestKey = -1;
        int n = -1;
        for (key k : kbMap) {
            n++;
            if (x >= k.x && y >= k.y && x < k.x + k.w && y < k.y + k.h)
                return n;
            double xd = xs * (x < k.x ? k.x - x : x >= k.x + k.w ? x + 1 - k.x - k.w : 0);
            double yd = ys * (y < k.y ? k.y - y : y >= k.y + k.h ? y + 1 - k.y - k.h : 0);
            double d = xd == 0 ? yd : yd == 0 ? xd : (int) (Math.sqrt(xd * xd + yd * yd) + 0.5);
            if (d < bestDistance) {
                bestKey = n;
                bestDistance = d;
            }
        }
        return bestDistance < 7 ? bestKey : -1;
    }
    public void raised() {
        currentKey = -1;
        num = false;
        shift = false;
        lock = false;
        invalidate();
    }

    public void layout(int left, int top, int right, int bottom) {
        super.layout(left, top, right, bottom);
        kbRect = new Rect(left, top, right, bottom);
        portrait = right - left < bottom - top;
        double ah = (kbRect.bottom - kbRect.top) / (portrait ? 3f : 2f);
        kbRect.top = kbRect.bottom - (int) ah;

        invalidate();
    }

    static private final int bRadius = 5;

    protected void onDraw(Canvas canvas) {
        boolean dark = false;
        int back, cap, grayCap, blueCap, shadow, text;
        if (dark) {
            back = 0xff202020;
            cap = 0xff585858;
            text = 0xffffffff;
            shadow = 0xff141414;
            grayCap = 0xff363636;
            blueCap = 0xff0a60ff;
        } else {
            back = 0xffc5c9d1;
            cap = 0xffffffff;
            text = 0xff000000;
            shadow = 0xff74767b;
            grayCap = 0xff999fae;
            blueCap = 0xff0a60ff;
        }

        float xs = ((float) (kbRect.right - kbRect.left)) / KB_WIDTH;
        float ys = ((float) (kbRect.bottom - kbRect.top)) / KB_HEIGHT;

        Paint paint = new Paint();
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(back);
        canvas.drawRect(kbRect, paint);

        int kn = -1;
        RectF r = new RectF();
        Rect tr = new Rect();
        for (key k : kbMap) {
            kn++;
            boolean active = kn == currentKey;
            final int sw = 1;
            r.left = k.x * xs + kbRect.left;
            r.top = k.y * ys + kbRect.top + sw;
            r.right = r.left + k.w * xs;
            r.bottom = r.top + k.h * ys - sw;
            paint.setColor(shadow);
            canvas.drawRoundRect(r, bRadius, bRadius, paint);
            r.top -= sw;
            r.bottom -= sw;

            if (k.special == SPEC_NONE) {
                String chars = num ? shift ? k.sym : k.num : shift ? k.shifted : k.normal;
                paint.setColor(cap);
                if (!active) {
                    drawCharKey(r, chars.charAt(0), text, canvas, paint);
                } else if (!keyExpanded) {
                    drawSmallBubble(r, chars.charAt(0), shadow, text, paint);
                } else {
                    drawLargeBubble(r, chars, shadow, text, blueCap, paint);
                }
            } else {
                if (k.special == SPEC_RS || k.special == SPEC_ENTER)
                    paint.setColor(active ? cap : blueCap);
                else if (k.special == SPEC_SHIFT && shift && !num)
                    paint.setColor(cap);
                else if (k.special == SPEC_SHIFT || k.special == SPEC_ALT)
                    paint.setColor(grayCap);
                else
                    paint.setColor(active ? cap : grayCap);
                canvas.drawRoundRect(r, bRadius, bRadius, paint);
                float fontSize;
                if ((!num && k.special == SPEC_SHIFT) || k.special == SPEC_BACKSPACE)
                    fontSize = (r.bottom - r.top) / 1.7f;
                else
                    fontSize = (r.bottom - r.top) / 2.5f;
                paint.setTypeface(Typeface.create(Typeface.SANS_SERIF, Typeface.BOLD));
                paint.setTextSize(fontSize);
                String label = null;
                switch (k.special) {
                    case SPEC_SHIFT: label = num ? shift ? "123" : "#+=" : lock ? "\u21ea" : "\u21e7"; break;
                    case SPEC_BACKSPACE: label = "\u232b"; break;
                    case SPEC_ALT: label = num ? "ABC" : "123"; break;
                    case SPEC_ESC: label = "Esc"; break;
                    case SPEC_SPACE: label = "Space"; break;
                    case SPEC_RS: label = "R\u2009/\u2009S"; break;
                    case SPEC_ENTER: label = "Enter"; break;
                }
                if (label == null)
                    continue;
                if (!active && (k.special == SPEC_RS || k.special == SPEC_ENTER))
                    paint.setColor(Color.WHITE);
                else
                    paint.setColor(text);
                paint.getTextBounds(label, 0, label.length(), tr);
                Paint.FontMetrics fm = paint.getFontMetrics();
                canvas.drawText(label,
                             (r.left + r.right - (tr.left + tr.right)) / 2,
                             (r.top + r.bottom - (fm.descent + fm.ascent)) / 2,
                                paint);
            }
        }
    }

    void drawCharKey(RectF r, char c, int color, Canvas canvas, Paint paint) {
        canvas.drawRoundRect(r, bRadius, bRadius, paint);
        byte[] bits = core_get_char_pixels(c);
        paint.setColor(color);
        float ps = (r.bottom - r.top) / (portrait ? 16 : 12);
        float x = r.left + ((r.right - r.left) - 5 * ps) / 2;
        float y = r.top + ((r.bottom - r.top) - 8 * ps) / 2;
        for (int v = 0; v < 8; v++)
            for (int h = 0; h < 5; h++)
                if (((bits[h] >> v) & 1) != 0) {
                    float left = x + h * ps;
                    float top = y + v * ps;
                    float right = left + ps;
                    float bottom = top + ps;
                    canvas.drawRect(left, top, right, bottom, paint);
                }
    }

    /*
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
    */

    void drawSmallBubble(RectF r, char c, int shadowColor, int textColor, Paint paint) {
        /*
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
        */
    }

    void drawLargeBubble(RectF r, String s, int shadowColor, int textColor, int blueColor, Paint paint) {
        /*
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
        */
    }

/*
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
        if (!num && shiftReleased != 0 && System.currentTimeMillis() < shiftReleased + 250) {
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
        if (shift && !num)
            [CalcView alphaKeyboardAlpha:27]; // [ESC] character
        else
            [CalcView alphaKeyboardDown:33]; // EXIT key
        goto done2;
    }
    if (k->special == SPEC_ENTER) {
        if (shift && !num)
            [CalcView alphaKeyboardAlpha:10]; // [LF] character
        else
            [CalcView alphaKeyboardDown:13]; // ENTER key
        goto done2;
    }
    
    // At this point, the key must be a normal character key.
    // We don't type the characters on key-down; we make bubbles
    // appear on the screen, and type a character only when the
    // user ends the touch in the right spot.
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
        shiftReleased = num ? 0 : System.currentTimeMillis();
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
    if (currentKey == -1 || kbMap[currentKey].special != SPEC_NONE)
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
    */
}
