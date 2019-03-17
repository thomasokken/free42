/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
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
        byte[] macro;
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
    private boolean display_enabled = true;
    private Bitmap skinBitmap;
    private Bitmap display = Bitmap.createBitmap(131, 16, Bitmap.Config.ARGB_8888);
    private int[] display_buffer = new int[131 * 16];
    private boolean[] ann_state;
    private int active_key = -1;
    private boolean skinSmoothing;
    private boolean displaySmoothing;
    private boolean maintainSkinAspect;

    public SkinLayout(String skinName, boolean skinSmoothing, boolean displaySmoothing, boolean maintainSkinAspect) {
        this(skinName, skinSmoothing, displaySmoothing, maintainSkinAspect, null);
    }
    
    public SkinLayout(String skinName, boolean skinSmoothing, boolean displaySmoothing, boolean maintainSkinAspect, boolean[] ann_state) {
        this.skinSmoothing = skinSmoothing;
        this.displaySmoothing = displaySmoothing;
        this.maintainSkinAspect = maintainSkinAspect;
        if (ann_state == null)
            this.ann_state = new boolean[7];
        else
            this.ann_state = ann_state;
        
        InputStream is = null;
        try {
            if (skinName.startsWith("/"))
                is = new FileInputStream(skinName + ".gif");
            else
                is = getClass().getResourceAsStream(skinName + ".gif");
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
                if (skinName.startsWith("/"))
                    is = new FileInputStream(skinName + ".layout");
                else
                    is = getClass().getResourceAsStream(skinName + ".layout");
                if (is == null)
                    throw new IOException();
            } catch (IOException e) {
                throw new IllegalArgumentException("No layout found for skin " + skinName);
            }
            
            BufferedReader reader = new BufferedReader(new InputStreamReader(is));
            String line;
            //int lineno = 0;
            List<SkinKey> tempkeylist = new ArrayList<SkinKey>();
            List<SkinMacro> tempmacrolist = new ArrayList<SkinMacro>();
            lineloop:
            while ((line = reader.readLine()) != null) {
                //lineno++;
                int pound = line.indexOf('#');
                if (pound != -1)
                    line = line.substring(0, pound);
                line = line.trim();
                if (line.length() == 0)
                    continue;
                String lcline = line.toLowerCase(Locale.US);
                if (lcline.startsWith("skin:")) {
                    StringTokenizer tok = new StringTokenizer(line.substring(5), ", \t");
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
                    StringTokenizer tok = new StringTokenizer(line.substring(8), ", \t");
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
    
                    StringTokenizer tok = new StringTokenizer(line.substring(sp + 1), ", \t");
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
                    StringTokenizer tok = new StringTokenizer(line.substring(6), " \t");
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
                    macro.macro = new byte[blist.size()];
                    int i = 0;
                    for (byte b : blist)
                        macro.macro[i++] = b;
                    tempmacrolist.add(macro);
                } else if (lcline.startsWith("annunciator:")) {
                    StringTokenizer tok = new StringTokenizer(line.substring(12), ", \t");
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
                } else if (line.indexOf(':') != -1) {
                    // TODO: Embedded keyboard mappings
    //              keymap_entry *entry = parse_keymap_entry(line, lineno);
    //              if (entry != NULL) {
    //                  if (keymap_length == kmcap) {
    //                      kmcap += 50;
    //                      keymap = (keymap_entry *)
    //                          realloc(keymap, kmcap * sizeof(keymap_entry));
    //                  }
    //                  memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
    //              }
                }
            }
            keylist = tempkeylist.toArray(new SkinKey[0]);
            macrolist = tempmacrolist.toArray(new SkinMacro[0]);
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
    
    public boolean in_menu_area(int x, int y) {
        return y < display_loc.y + display_scale.y * 8;
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

    public int find_skey(int ckey) {
        int i;
        for (i = 0; i < keylist.length; i++)
        if (keylist[i].code == ckey || keylist[i].shifted_code == ckey)
            return i;
        return -1;
    }

    public byte[] find_macro(int ckey) {
        for (SkinMacro macro : macrolist)
            if (macro.code == ckey)
                return macro.macro;
        return null;
    }

//unsigned char *skin_keymap_lookup(guint keyval, bool printable,
//                bool ctrl, bool alt, bool shift, bool cshift,
//                bool *exact) {
//    int i;
//    unsigned char *macro = NULL;
//    for (i = 0; i < keymap_length; i++) {
//  keymap_entry *entry = keymap + i;
//  if (ctrl == entry->ctrl
//      && alt == entry->alt
//      && (printable || shift == entry->shift)
//      && keyval == entry->keyval) {
//      macro = entry->macro;
//      if (cshift == entry->cshift) {
//      *exact = true;
//      return macro;
//      }
//  }
//    }
//    *exact = false;
//    return macro;
//}
    
    private void repaint_key(Canvas canvas, Bitmap skin, int key, boolean state) {
        if (key >= -7 && key <= -2) {
            /* Soft key */
            if (!display_enabled)
                // Should never happen -- the display is only disabled during macro
                // execution, and softkey events should be impossible to generate
                // in that state. But, just staying on the safe side.
                return;
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
    
    public void repaint(Canvas canvas) {
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
        paintDisplay = paintDisplay && display_enabled;
        if (!paintDisplay && !paintSkin)
            return;
        Paint p = new Paint();
        
        if (paintSkin) {
            p.setAntiAlias(skinSmoothing);
            p.setFilterBitmap(skinSmoothing);
            canvas.drawBitmap(skinBitmap, new Rect(skin.x, skin.y, skin.x + skin.width, skin.y + skin.height),
                                          new Rect(0, 0, skin.width, skin.height), p);
            if (display_enabled)
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
    }
    
    public Rect set_display_enabled(boolean enable) {
        if (display_enabled == enable)
            return null;
        display_enabled = enable;
        if (!display_enabled)
            return null;
        Rect r = new Rect(display_loc.x, display_loc.y,
                            (int) Math.ceil(display_loc.x + 131 * display_scale.x),
                            (int) Math.ceil(display_loc.y + 16 * display_scale.y));
        for (int i = 0; i < 7; i++) {
            SkinRect a = annunciators[i].disp_rect;
            Rect ra = new Rect(a.x, a.y, a.x + a.width, a.y + a.height);
            r.union(ra);
        }
        return r;
    }
}
