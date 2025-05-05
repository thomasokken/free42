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

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.security.Key;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.TreeSet;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;

public class SkinLayout {

    /**************************/
    /* Skin description stuff */
    /**************************/

    private static class SkinPoint {
        int x, y;
    }

    private static class SkinScale {
        double x, y;
    }

    private static class SkinRect {
        int x, y, width, height;
    }

    private static class SkinKey {
        int code, shifted_code;
        SkinRect sens_rect = new SkinRect();
        SkinRect disp_rect = new SkinRect();
        SkinPoint src = new SkinPoint();
    }

    private static class SkinMacro {
        int code;
        Object macro;
        String macro2;
        boolean secondIsText;
    }

    private static class SkinAnnunciator {
        SkinRect disp_rect = new SkinRect();
        SkinPoint src = new SkinPoint();
    }

    private SkinRect skin = new SkinRect();
    private SkinPoint display_loc = new SkinPoint();
    private SkinScale display_scale = new SkinScale();
    private int display_bg, display_fg;
    private SkinKey[] keylist = null;
    private SkinMacro[] macrolist = null;
    private SkinAnnunciator[] annunciators = new SkinAnnunciator[7];
    private KeymapEntry[] keymap;
    private Bitmap skinBitmap;
    private Bitmap display = Bitmap.createBitmap(131, 16, Bitmap.Config.ARGB_8888);
    private int[] display_buffer = new int[131 * 16];
    private boolean[] ann_state;
    private int active_key = -1;
    private boolean skinSmoothing;
    private boolean displaySmoothing;
    private boolean maintainSkinAspect;
    private KeymapEntry[] calcKeymap;

    public SkinLayout(Context ctx, String skinName, boolean skinSmoothing, boolean displaySmoothing, boolean maintainSkinAspect, KeymapEntry[] calcKeymap) {
        this(ctx, skinName, skinSmoothing, displaySmoothing, maintainSkinAspect, calcKeymap, null);
    }
    
    public SkinLayout(Context ctx, String skinName, boolean skinSmoothing, boolean displaySmoothing, boolean maintainSkinAspect, KeymapEntry[] calcKeymap, boolean[] ann_state) {
        this.skinSmoothing = skinSmoothing;
        this.displaySmoothing = displaySmoothing;
        this.maintainSkinAspect = maintainSkinAspect;
        this.calcKeymap = calcKeymap;
        if (ann_state == null)
            this.ann_state = new boolean[7];
        else
            this.ann_state = ann_state;
        
        InputStream is = null;
        try {
            if (skinName.startsWith("/")) {
                is = new FileInputStream(skinName + ".gif");
            } else {
                try {
                    is = new FileInputStream(ctx.getFilesDir() + "/" + skinName + ".gif");
                } catch (FileNotFoundException e) {
                    is = ctx.getAssets().open(skinName + ".gif");
                }
            }
            if (is == null)
                throw new IOException();
            skinBitmap = new BitmapDrawable(is).getBitmap();
        } catch (IOException e) {
            throw new IllegalArgumentException("No image found for skin " + skinName);
        } finally {
            if (is != null)
                try {
                    is.close();
                } catch (IOException e) {}
        }

        is = null;
        try {
            try {
                if (skinName.startsWith("/")) {
                    is = new FileInputStream(skinName + ".layout");
                } else {
                    try {
                        is = new FileInputStream(ctx.getFilesDir() + "/" + skinName + ".layout");
                    } catch (FileNotFoundException e) {
                        is = ctx.getAssets().open(skinName + ".layout");
                    }
                }
                if (is == null)
                    throw new IOException();
            } catch (IOException e) {
                throw new IllegalArgumentException("No layout found for skin " + skinName);
            }
            
            BufferedReader reader = new BufferedReader(new InputStreamReader(is, "UTF-8"));
            String line;
            int lineno = 0;
            List<SkinKey> tempkeylist = new ArrayList<SkinKey>();
            List<SkinMacro> tempmacrolist = new ArrayList<SkinMacro>();
            List<KeymapEntry> keymapList = new ArrayList<KeymapEntry>();
            lineloop:
            while ((line = reader.readLine()) != null) {
                lineno++;
                line = line.replace('\t', ' ');
                int pound = line.indexOf('#');
                if (pound != -1)
                    line = line.substring(0, pound);
                line = line.trim();
                if (line.length() == 0)
                    continue;
                String lcline = line.toLowerCase(Locale.US);
                if (lcline.startsWith("skin:")) {
                    StringTokenizer tok = new StringTokenizer(line.substring(5), ", ");
                    try {
                        int x = Integer.parseInt(tok.nextToken());
                        int y = Integer.parseInt(tok.nextToken());
                        int width = Integer.parseInt(tok.nextToken());
                        int height = Integer.parseInt(tok.nextToken());
                        skin.x = x;
                        skin.y = y;
                        skin.width = width;
                        skin.height = height;
                    } catch (NoSuchElementException e) {
                        // ignore
                    } catch (NumberFormatException e) {
                        // ignore
                    }
                } else if (lcline.startsWith("display:")) {
                    StringTokenizer tok = new StringTokenizer(line.substring(8), ", ");
                    try {
                        int x = Integer.parseInt(tok.nextToken());
                        int y = Integer.parseInt(tok.nextToken());
                        double xscale = Double.parseDouble(tok.nextToken());
                        double yscale = Double.parseDouble(tok.nextToken());
                        int bg = Integer.parseInt(tok.nextToken(), 16) | 0xff000000;
                        int fg = Integer.parseInt(tok.nextToken(), 16) | 0xff000000;
                        display_loc.x = x;
                        display_loc.y = y;
                        display_scale.x = xscale;
                        display_scale.y = yscale;
                        display_bg = bg;
                        display_fg = fg;
                    } catch (NoSuchElementException e) {
                        // ignore
                    } catch (NumberFormatException e) {
                        // ignore
                    }
                } else if (lcline.startsWith("key:")) {
                    int keynum, shifted_keynum;
    
                    line = line.substring(4).trim();
                    int sp = line.indexOf(' ');
                    if (sp == -1)
                        continue;
                    String keynumstr = line.substring(0, sp);
                    int comma = keynumstr.indexOf(',');
                    try {
                        if (comma == -1) {
                            keynum = shifted_keynum = Integer.parseInt(keynumstr);
                        } else {
                            keynum = Integer.parseInt(keynumstr.substring(0, comma));
                            shifted_keynum = Integer.parseInt(keynumstr.substring(comma + 1));
                        }
                    } catch (NumberFormatException e) {
                        continue;
                    }
    
                    StringTokenizer tok = new StringTokenizer(line.substring(sp + 1), ", ");
                    try {
                        int sens_x = Integer.parseInt(tok.nextToken());
                        int sens_y = Integer.parseInt(tok.nextToken());
                        int sens_width = Integer.parseInt(tok.nextToken());
                        int sens_height = Integer.parseInt(tok.nextToken());
                        int disp_x = Integer.parseInt(tok.nextToken());
                        int disp_y = Integer.parseInt(tok.nextToken());
                        int disp_width = Integer.parseInt(tok.nextToken());
                        int disp_height = Integer.parseInt(tok.nextToken());
                        int act_x = Integer.parseInt(tok.nextToken());
                        int act_y = Integer.parseInt(tok.nextToken());
                        SkinKey key = new SkinKey();
                        key.code = keynum;
                        key.shifted_code = shifted_keynum;
                        key.sens_rect.x = sens_x;
                        key.sens_rect.y = sens_y;
                        key.sens_rect.width = sens_width;
                        key.sens_rect.height = sens_height;
                        key.disp_rect.x = disp_x;
                        key.disp_rect.y = disp_y;
                        key.disp_rect.width = disp_width;
                        key.disp_rect.height = disp_height;
                        key.src.x = act_x;
                        key.src.y = act_y;
                        tempkeylist.add(key);
                    } catch (NoSuchElementException e) {
                        // ignore
                    } catch (NumberFormatException e) {
                        // ignore
                    }
                } else if (lcline.startsWith("macro:")) {
                    int quot1 = line.indexOf('"');
                    if (quot1 != -1) {
                        int quot2 = line.indexOf('"', quot1 + 1);
                        if (quot2 != -1) {
                            int len = quot2 - quot1 - 1;
                            try {
                                int n = Integer.parseInt(line.substring(6, quot1).trim());
                                if (n >= 38 && n <= 255) {
                                    SkinMacro macro = new SkinMacro();
                                    macro.code = n;
                                    macro.macro = line.substring(quot1 + 1, quot2);
                                    quot1 = line.indexOf('"', quot2 + 1);
                                    if (quot1 == -1)
                                        quot1 = line.indexOf('\'', quot2 + 1);
                                    if (quot1 != -1) {
                                        char q = line.charAt(quot1);
                                        quot2 = line.indexOf(q, quot1 + 1);
                                        if (quot2 != -1) {
                                            macro.macro2 = line.substring(quot1 + 1, quot2);
                                            macro.secondIsText = q == '\'';
                                        }
                                    }
                                    tempmacrolist.add(macro);
                                }
                            } catch (Exception e) {
                                // Ignore
                            }
                        }
                    } else {
                        StringTokenizer tok = new StringTokenizer(line.substring(6), " ");
                        List<Byte> blist = new ArrayList<Byte>();
                        while (tok.hasMoreTokens()) {
                            String t = tok.nextToken();
                            int n;
                            try {
                                n = Integer.parseInt(t);
                            } catch (NumberFormatException e) {
                                continue lineloop;
                            }
                            if (blist.isEmpty() ? (n < 38 || n > 255) : (n < 1 || n > 37))
                                /* Macro code out of range; ignore this macro */
                                continue lineloop;
                            blist.add((byte) n);
                        }
                        if (blist.isEmpty())
                            continue;
                        SkinMacro macro = new SkinMacro();
                        macro.code = blist.remove(0) & 255;
                        byte[] codes = new byte[blist.size()];
                        int i = 0;
                        for (byte b : blist)
                            codes[i++] = b;
                        macro.macro = codes;
                        tempmacrolist.add(macro);
                    }
                } else if (lcline.startsWith("annunciator:")) {
                    StringTokenizer tok = new StringTokenizer(line.substring(12), ", ");
                    try {
                        int annnum = Integer.parseInt(tok.nextToken());
                        int disp_x = Integer.parseInt(tok.nextToken());
                        int disp_y = Integer.parseInt(tok.nextToken());
                        int disp_width = Integer.parseInt(tok.nextToken());
                        int disp_height = Integer.parseInt(tok.nextToken());
                        int act_x = Integer.parseInt(tok.nextToken());
                        int act_y = Integer.parseInt(tok.nextToken());
                        if (annnum >= 1 && annnum <= 7) {
                            SkinAnnunciator ann = new SkinAnnunciator();
                            annunciators[annnum - 1] = ann;
                            ann.disp_rect.x = disp_x;
                            ann.disp_rect.y = disp_y;
                            ann.disp_rect.width = disp_width;
                            ann.disp_rect.height = disp_height;
                            ann.src.x = act_x;
                            ann.src.y = act_y;
                        }
                    } catch (NoSuchElementException e) {
                        // ignore
                    } catch (NumberFormatException e) {
                        // ignore
                    }
                } else if (lcline.startsWith("droidkey:")) {
                    KeymapEntry entry = KeymapEntry.parse(line.substring(9), lineno);
                    if (entry != null)
                        keymapList.add(entry);
                }
            }
            keylist = tempkeylist.toArray(new SkinKey[0]);
            macrolist = tempmacrolist.toArray(new SkinMacro[0]);
            keymap = keymapList.toArray(new KeymapEntry[0]);
        } catch (IOException e) {
            throw new IllegalArgumentException(e);
        } finally {
            if (is != null)
                try {
                    is.close();
                } catch (IOException e) {}
        }
    }
    
    public void setSmoothing(boolean skinSmoothing, boolean displaySmoothing) {
        this.skinSmoothing = skinSmoothing;
        this.displaySmoothing = displaySmoothing;
    }
    
    public int getWidth() {
        return skin.width;
    }
    
    public int getHeight() {
        return skin.height;
    }
    
    public void setMaintainSkinAspect(boolean maintainSkinAspect) {
        this.maintainSkinAspect = maintainSkinAspect;
    }
    
    public boolean getMaintainSkinAspect() {
        return maintainSkinAspect;
    }
    
    public boolean[] getAnnunciators() {
        return ann_state;
    }
    
    public Rect set_active_key(int skey) {
        Rect r1 = getKeyRect(active_key);
        Rect r2 = getKeyRect(skey);
        active_key = skey;
        if (r1 == null)
            return r2;
        if (r2 == null)
            return r1;
        r1.union(r2);
        return r1;
    }
    
    private Rect getKeyRect(int skey) {
        if (skey >= -7 && skey <= -2) {
            int x = (int) ((-2 - skey) * 22 * display_scale.x + display_loc.x);
            int y = (int) (9 * display_scale.y + display_loc.y);
            return new Rect(x, y, (int) Math.ceil(x + 21 * display_scale.x),
                                  (int) Math.ceil(y + 7 * display_scale.y));
        } else if (skey >= 0 && skey < keylist.length) {
            SkinRect r = keylist[skey].disp_rect;
            return new Rect(r.x, r.y, r.x + r.width, r.y + r.height);
        } else
            return null;
    }
    
    public int in_menu_area(int x, int y) {
        if (y < display_loc.y + display_scale.y * 16 / 2)
            if (x > display_loc.x + display_scale.x * 131 / 2)
                return 2;
            else
                return 1;
        else
            return 0;
    }

    public void find_key(boolean menu_active, int x, int y, IntHolder skey, IntHolder ckey) {
        if (menu_active
                && x >= display_loc.x
                && x < display_loc.x + 131 * display_scale.x
                && y >= display_loc.y + 9 * display_scale.y
                && y < display_loc.y + 16 * display_scale.y) {
            int softkey = (int) ((x - display_loc.x) / (22 * display_scale.x) + 1);
            skey.value = -1 - softkey;
            ckey.value = softkey;
            return;
        }
        for (int i = 0; i < keylist.length; i++) {
            SkinKey k = keylist[i];
            int rx = x - k.sens_rect.x;
            int ry = y - k.sens_rect.y;
            if (rx >= 0 && rx < k.sens_rect.width
                    && ry >= 0 && ry < k.sens_rect.height) {
                skey.value = i;
                ckey.value = ann_state[1] ? k.shifted_code : k.code;
                return;
            }
        }
        skey.value = -1;
        ckey.value = 0;
    }

    public int find_skey(int ckey, boolean cshift) {
        int fuzzy_match = -1;
        for (int i = 0; i < keylist.length; i++)
            if (keylist[i].code == ckey || keylist[i].shifted_code == ckey)
                if ((cshift ? keylist[i].shifted_code : keylist[i].code) == ckey)
                    return i;
                else if (fuzzy_match == -1)
                    fuzzy_match = i;
        return fuzzy_match;
    }

    public Object find_macro(int ckey) {
        for (SkinMacro macro : macrolist)
            if (macro.code == ckey) {
                if (macro.macro instanceof byte[])
                    return macro.macro;
                else if (macro.macro2 == null || !Free42Activity.instance.core_alpha_menu())
                    return new Object[] { macro.macro, false };
                else
                    return new Object[] { macro.macro2, macro.secondIsText };
            }
        return null;
    }

    public byte[] keymap_lookup(String keychar, boolean printable,
                                boolean ctrl, boolean alt, boolean numpad, boolean shift,
                                boolean cshift, BooleanHolder exact) {
        byte[] macro = null;
        for (KeymapEntry entry : keymap) {
            if (ctrl == entry.ctrl
                    && alt == entry.alt
                    && (printable || shift == entry.shift)
                    && keychar.equals(entry.keychar)) {
                if ((!numpad || shift == entry.shift) && numpad == entry.numpad && cshift == entry.cshift) {
                    exact.value = true;
                    return entry.macro;
                }
                if ((numpad || !entry.numpad) && (cshift || !entry.cshift))
                    macro = entry.macro;
            }
        }
        exact.value = false;
        return macro;
    }

    private void repaint_key(Canvas canvas, Bitmap skin, int key, boolean state) {
        if (key >= -7 && key <= -2) {
            /* Soft key */
            key = -1 - key;
            Rect dst = new Rect((int) (display_loc.x + (key - 1) * 22 * display_scale.x),
                                (int) (display_loc.y + 9 * display_scale.y),
                                (int) Math.ceil(display_loc.x + ((key - 1) * 22 + 21) * display_scale.x),
                                (int) Math.ceil(display_loc.y + 16 * display_scale.y));
            if (state) {
                // Construct a temporary Bitmap, create the inverted version of
                // the affected screen rectangle there, and blit it
                Bitmap invSoftkey = Bitmap.createBitmap(21, 7, Bitmap.Config.ARGB_8888);
                int[] invSoftkeyBuf = new int[21 * 7];
                for (int v = 0; v < 7; v++)
                    for (int h = 0; h < 21; h++) {
                        int color = display.getPixel(h + (key - 1) * 22, v + 9);
                        color = color == display_bg ? display_fg : display_bg;
                        invSoftkeyBuf[h + 21 * v] = color;
                    }
                invSoftkey.setPixels(invSoftkeyBuf, 0, 21, 0, 0, 21, 7);
                Rect src = new Rect(0, 0, 21, 7);
                Paint p = new Paint();
                p.setAntiAlias(displaySmoothing);
                p.setFilterBitmap(displaySmoothing);
                canvas.drawBitmap(invSoftkey, src, dst, p);
            } else {
                // Repaint the screen
                Rect src = new Rect((key - 1) * 22, 9, (key - 1) * 22 + 21, 16);
                Paint p = new Paint();
                p.setAntiAlias(displaySmoothing);
                p.setFilterBitmap(displaySmoothing);
                canvas.drawBitmap(display, src, dst, p);
            }
            return;
        }
    
        if (key < 0 || key >= keylist.length)
            return;
        SkinKey k = keylist[key];
        Rect dst = new Rect(k.disp_rect.x,
                            k.disp_rect.y,
                            k.disp_rect.x + k.disp_rect.width,
                            k.disp_rect.y + k.disp_rect.height);
        Rect src;
        if (state)
            src = new Rect(k.src.x,
                           k.src.y,
                           k.src.x + k.disp_rect.width,
                           k.src.y + k.disp_rect.height);
        else
            src = dst;
        Paint p = new Paint();
        p.setAntiAlias(skinSmoothing);
        p.setFilterBitmap(skinSmoothing);
        canvas.drawBitmap(skin, src, dst, p);
    }
    
    public Rect display_blitter(byte[] bits, int bytesperline, int x, int y, int width, int height) {
        int hmax = x + width;
        int vmax = y + height;
        for (int v = y; v < vmax; v++)
            for (int h = x; h < hmax; h++) {
                boolean bitSet = (bits[(h >> 3) + v * bytesperline] & (1 << (h & 7))) != 0;
                display_buffer[v * 131 + h] = bitSet ? display_fg : display_bg;
            }
        display.setPixels(display_buffer, x + 131 * y, 131, x, y, width, height);
        
        // Return a Rect telling the caller what part of the View needs to be invalidated
        return new Rect((int) (display_loc.x + x * display_scale.x),
                        (int) (display_loc.y + y * display_scale.y),
                        (int) Math.ceil(display_loc.x + (x + width) * display_scale.x),
                        (int) Math.ceil(display_loc.y + (y + height) * display_scale.y));
    }
    
    public Rect update_annunciators(int updn, int shf, int prt, int run, int lowbat, int g, int rad) {
        int minx = Integer.MAX_VALUE;
        int miny = Integer.MAX_VALUE;
        int maxx = Integer.MIN_VALUE;
        int maxy = Integer.MIN_VALUE;
        
        int[] ann_arg = new int[] { updn, shf, prt, run, lowbat, g, rad };
        for (int i = 0; i < 7; i++) {
            int newState = ann_arg[i];
            if (newState == (ann_state[i] ? 0 : 1)) {
                ann_state[i] = !ann_state[i];
                SkinRect ann_rect = annunciators[i].disp_rect;
                if (minx > ann_rect.x)
                    minx = ann_rect.x;
                if (miny > ann_rect.y)
                    miny = ann_rect.y;
                if (maxx < ann_rect.x + ann_rect.width)
                    maxx = ann_rect.x + ann_rect.width;
                if (maxy < ann_rect.y + ann_rect.height)
                    maxy = ann_rect.y + ann_rect.height;
            }
        }
        if (minx == Integer.MAX_VALUE)
            return null;
        else
            return new Rect(minx, miny, maxx, maxy);
    }
    
    public void repaint(Canvas canvas, boolean shortcuts) {
        Rect clip = canvas.getClipBounds();
        boolean paintDisplay = false;
        boolean paintSkin = false;
        Rect disp = new Rect(display_loc.x,
                             display_loc.y,
                             (int) Math.ceil(display_loc.x + 131 * display_scale.x),
                             (int) Math.ceil(display_loc.y + 16 * display_scale.y));
        if (disp.contains(clip))
            paintDisplay = true;
        else {
            Rect sk = new Rect(0, 0, skin.width, skin.height);
            paintSkin = Rect.intersects(clip, sk);
            paintDisplay = Rect.intersects(clip, disp);
        }
        if (!paintDisplay && !paintSkin)
            return;
        Paint p = new Paint();
        
        if (paintSkin) {
            p.setAntiAlias(skinSmoothing);
            p.setFilterBitmap(skinSmoothing);
            canvas.drawBitmap(skinBitmap, new Rect(skin.x, skin.y, skin.x + skin.width, skin.y + skin.height),
                                          new Rect(0, 0, skin.width, skin.height), p);
            for (int i = 0; i < 7; i++)
                if (ann_state[i]) {
                    SkinAnnunciator ann = annunciators[i];
                    Rect src = new Rect(ann.src.x,
                                        ann.src.y,
                                        ann.src.x + ann.disp_rect.width,
                                        ann.src.y + ann.disp_rect.height);
                    Rect dst = new Rect(ann.disp_rect.x,
                                        ann.disp_rect.y,
                                        ann.disp_rect.x + ann.disp_rect.width,
                                        ann.disp_rect.y + ann.disp_rect.height);
                    p.setAntiAlias(displaySmoothing);
                    p.setFilterBitmap(displaySmoothing);
                    canvas.drawBitmap(skinBitmap, src, dst, p);
                }
            if (active_key >= 0)
                repaint_key(canvas, skinBitmap, active_key, true);
        }
        
        if (paintDisplay) {
            Rect src = new Rect(0, 0, 131, 16);
            Rect dst = new Rect(display_loc.x,
                                display_loc.y,
                                (int) Math.ceil(display_loc.x + 131 * display_scale.x),
                                (int) Math.ceil(display_loc.y + 16 * display_scale.y));
            p.setAntiAlias(displaySmoothing);
            p.setFilterBitmap(displaySmoothing);
            canvas.drawBitmap(display, src, dst, p);
            if (active_key >= -7 && active_key <= -2)
                repaint_key(canvas, skinBitmap, active_key, true);
        }

        if (shortcuts) {
            Paint transparentWhite = new Paint();
            transparentWhite.setARGB(127, 255, 255, 255);
            canvas.drawRect(0, 0, skin.width, skin.height, transparentWhite);
            List<KeyShortcutInfo> ksinfolist = getShortcutInfo();
            Paint opaqueBlack = new Paint();
            opaqueBlack.setColor(Color.BLACK);
            opaqueBlack.setTypeface(Typeface.create(Typeface.SANS_SERIF, Typeface.BOLD));
            opaqueBlack.setTextSize((float) Math.sqrt(((double) skin.width) * skin.height) / 30);
            for (KeyShortcutInfo ksinfo : ksinfolist) {
                canvas.drawRect(ksinfo.x + 2, ksinfo.y + 2, ksinfo.x + ksinfo.width - 2, ksinfo.y + ksinfo.height - 2, transparentWhite);
                drawTextInRect(canvas, ksinfo.text(), ksinfo.x + 4, ksinfo.y + 4, ksinfo.width - 8, ksinfo.height - 8, opaqueBlack);
            }
        }
    }

    private class KeyShortcutInfo {
        int x, y, width, height;
        String unshifted, shifted;

        KeyShortcutInfo(SkinKey k) {
            x = k.sens_rect.x;
            y = k.sens_rect.y;
            width = k.sens_rect.width;
            height = k.sens_rect.height;
            unshifted = "";
            shifted = "";
        }

        boolean sameRect(SkinKey that) {
            return x == that.sens_rect.x
                    && y == that.sens_rect.y
                    && width == that.sens_rect.width
                    && height == that.sens_rect.height;
        }

        void add(String entryStr, boolean shift) {
            if (shift)
                shifted = entryStr + " " + shifted;
            else
                unshifted = entryStr + " " + unshifted;
        }

        String text() {
            String u, s;
            if (unshifted.length() == 0)
                u = "n/a";
            else
                u = unshifted.substring(0, unshifted.length() - 1);
            if (shifted.length() == 0)
                s = "n/a";
            else
                s = shifted.substring(0, shifted.length() - 1);
            return s + "\n" + u;
        }
    }

    private static String entry_to_text(KeymapEntry e) {
        String c;
        boolean numpad = e.numpad;
        String k = e.keychar;
        if (k.startsWith("NUMPAD_")) {
            k = k.substring(7);
            numpad = true;
        }
        if (e.keychar.equals("ESCAPE"))
            c = "Esc";
        else if (e.keychar.equals("\n"))
            c = "Enter";
        else if (e.keychar.equals("DEL"))
            c = "\u232b";
        else if (e.keychar.equals("DPAD_UP"))
            c = "\u2191";
        else if (e.keychar.equals("DPAD_DOWN"))
            c = "\u2193";
        else if (e.keychar.equals("DPAD_LEFT"))
            c = "\u2190";
        else if (e.keychar.equals("DPAD_RIGHT"))
            c = "\u2192";
        else if (e.keychar.equals("INSERT"))
            c = "Ins";
        else if (e.keychar.equals("FORWARD_DEL"))
            c = "\u2326";
        else if (e.keychar.equals("PAGE_UP"))
            c = "PgUp";
        else if (e.keychar.equals("PAGE_DOWN"))
            c = "PgDn";
        else
            c = e.keychar;
        if (numpad)
            c = "Kp" + c;
        String mods = "";
        boolean printable = !e.ctrl && !e.alt && c.length() == 1 && c.charAt(0) >= 33 && c.charAt(0) <= 126;
        if (e.ctrl)
            mods += "^";
        if (e.alt)
            mods += "\u2325";
        if (e.shift && !printable)
            mods += "\u21e7";
        return mods + c;
    }

    private List<KeyShortcutInfo> getShortcutInfo() {
        ArrayList<KeyShortcutInfo> list = new ArrayList<KeyShortcutInfo>();
        TreeSet<String> seen = new TreeSet<String>();
        for (int km = 0; km < 2; km++) {
            KeymapEntry[] kmap = km == 0 ? keymap : calcKeymap;
            for (int i = kmap.length - 1; i >= 0; i--) {
                KeymapEntry e = kmap[i];
                if (e.cshift)
                    continue;
                int key;
                boolean shifted;
                if (e.macro.length == 1) {
                    key = e.macro[0];
                    shifted = false;
                } else if (e.macro[0] == 28 && e.macro.length == 2) {
                    key = e.macro[1];
                    shifted = true;
                } else
                    continue;
                SkinKey k = null;
                for (int j = 0; j < keylist.length; j++) {
                    k = keylist[j];
                    if (key == k.code)
                        break;
                    if (key == k.shifted_code) {
                        shifted = true;
                        break;
                    }
                    k = null;
                }
                if (k == null)
                    continue;
                String entryStr = entry_to_text(e);
                if (seen.contains(entryStr))
                    continue;
                seen.add(entryStr);
                boolean exists = false;
                for (KeyShortcutInfo p : list) {
                    if (p.sameRect(k)) {
                        p.add(entryStr, shifted);
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    KeyShortcutInfo ki = new KeyShortcutInfo(k);
                    ki.add(entryStr, shifted);
                    list.add(ki);
                }
            }
        }
        return list;
    }

    private void drawTextInRect(Canvas canvas, String text, int x, int y, int width, int height, Paint paint) {
        Paint.FontMetrics metrics = paint.getFontMetrics();
        float spacing = -metrics.ascent + metrics.descent + metrics.leading;
        char[] chars = text.toCharArray();
        float ypos = y - paint.ascent();
        int pos = 0;
        canvas.save();
        canvas.clipRect(x, y, x + width, y + height);
        while (pos < chars.length) {
            int lf = text.indexOf('\n', pos);
            if (lf == -1)
                lf = chars.length;
            int len = paint.breakText(chars, pos, lf - pos, width, null);
            canvas.drawText(chars, pos, len, x, ypos, paint);
            pos += len;
            if (pos < chars.length && (chars[pos] == ' ' || chars[pos] == '\n'))
                pos++;
            ypos += spacing;
        }
        canvas.restore();
    }
}
