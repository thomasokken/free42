/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2010  Thomas Okken
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
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;

public class SkinLayout {

	/**************************/
	/* Skin description stuff */
	/**************************/

	private static class SkinPoint {
		int x, y;
	}

	private static class SkinRect {
		int x, y, width, height;
	}

	private static class SkinKey {
	    int code, shifted_code;
	    SkinRect sens_rect;
	    SkinRect disp_rect;
	    SkinPoint src;
	}

	private static class SkinMacro {
		int code;
		byte[] macro;
	}

	private static class SkinAnnunciator {
		SkinRect disp_rect;
		SkinPoint src;
	}

	private SkinRect skin = new SkinRect();
	private SkinPoint display_loc = new SkinPoint();
	private SkinPoint display_scale = new SkinPoint();
	private int display_bg, display_fg;
	private SkinKey[] keylist = null;
	private SkinMacro[] macrolist = null;
	private SkinAnnunciator[] annunciators = new SkinAnnunciator[7];

	public SkinLayout(InputStream is) throws IOException {
		BufferedReader reader = new BufferedReader(new InputStreamReader(is));
		String line;
		int lineno = 0;
		List<SkinKey> tempkeylist = new ArrayList<SkinKey>();
		List<SkinMacro> tempmacrolist = new ArrayList<SkinMacro>();
		lineloop:
		while ((line = reader.readLine()) != null) {
			lineno++;
			line = line.trim();
			if (line.length() == 0 || line.charAt(0) == '#')
				continue;
			String lcline = line.toLowerCase();
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
					int xscale = Integer.parseInt(tok.nextToken());
					int yscale = Integer.parseInt(tok.nextToken());
					int bg = Integer.parseInt(tok.nextToken(), 16);
					int fg = Integer.parseInt(tok.nextToken(), 16);
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
					if (blist.isEmpty() ? (n < 1 || n > 37) : (n < 38 || n > 255))
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
			    		SkinAnnunciator ann = annunciators[annnum - 1];
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
//			    keymap_entry *entry = parse_keymap_entry(line, lineno);
//			    if (entry != NULL) {
//					if (keymap_length == kmcap) {
//					    kmcap += 50;
//					    keymap = (keymap_entry *)
//							realloc(keymap, kmcap * sizeof(keymap_entry));
//					    // TODO - handle memory allocation failure
//					}
//					memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
//			    }
			}
		}
		keylist = tempkeylist.toArray(new SkinKey[0]);
		macrolist = tempmacrolist.toArray(new SkinMacro[0]);
    }
	
	public void repaint_annunciator(Canvas canvas, Bitmap skin, int which, boolean state) {
		SkinAnnunciator ann = annunciators[which - 1];
		Paint p = new Paint();
		if (state) {
			Rect src = new Rect(ann.src.x, ann.src.y, ann.src.x + ann.disp_rect.width, ann.src.y + ann.disp_rect.height);
			Rect dst = new Rect(ann.disp_rect.x, ann.disp_rect.y, ann.disp_rect.x + ann.disp_rect.width, ann.disp_rect.y + ann.disp_rect.height);
			canvas.drawBitmap(skin, src, dst, p);
		} else {
			Rect r = new Rect(ann.disp_rect.x, ann.disp_rect.y, ann.disp_rect.x + ann.disp_rect.width, ann.disp_rect.y + ann.disp_rect.height);
			canvas.drawBitmap(skin, r, r, p);
		}
	}

	public void skin_find_key(boolean menu_active, int x, int y, boolean cshift, IntHolder skey, IntHolder ckey) {
		if (menu_active
			    && x >= display_loc.x
			    && x < display_loc.x + 131 * display_scale.x
			    && y >= display_loc.y + 9 * display_scale.y
			    && y < display_loc.y + 16 * display_scale.y) {
			int softkey = (x - display_loc.x) / (22 * display_scale.x) + 1;
			skey.value = -1 - softkey;
			ckey.value = softkey;
			return;
	    }
		int i = 0;
		for (SkinKey k : keylist) {
			int rx = x - k.sens_rect.x;
			int ry = y - k.sens_rect.y;
			if (rx >= 0 && rx < k.sens_rect.width
					&& ry >= 0 && ry < k.sens_rect.height) {
			    skey.value = i++;
			    ckey.value = cshift ? k.shifted_code : k.code;
			    return;
			}
	    }
	    skey.value = -1;
	    ckey.value = 0;
	}

	public int skin_find_skey(int ckey) {
	    int i;
	    for (i = 0; i < keylist.length; i++)
		if (keylist[i].code == ckey || keylist[i].shifted_code == ckey)
		    return i;
	    return -1;
	}

	public byte[] skin_find_macro(int ckey) {
		for (SkinMacro macro : macrolist)
			if (macro.code == ckey)
				return macro.macro;
		return null;
	}

//unsigned char *skin_keymap_lookup(guint keyval, bool printable,
//				  bool ctrl, bool alt, bool shift, bool cshift,
//				  bool *exact) {
//    int i;
//    unsigned char *macro = NULL;
//    for (i = 0; i < keymap_length; i++) {
//	keymap_entry *entry = keymap + i;
//	if (ctrl == entry->ctrl
//		&& alt == entry->alt
//		&& (printable || shift == entry->shift)
//		&& keyval == entry->keyval) {
//	    macro = entry->macro;
//	    if (cshift == entry->cshift) {
//		*exact = true;
//		return macro;
//	    }
//	}
//    }
//    *exact = false;
//    return macro;
//}
//
//void skin_repaint_key(int key, bool state) {
//    SkinKey *k;
//
//    if (key >= -7 && key <= -2) {
//	/* Soft key */
//	if (!display_enabled)
//	    // Should never happen -- the display is only disabled during macro
//	    // execution, and softkey events should be impossible to generate
//	    // in that state. But, just staying on the safe side.
//	    return;
//	key = -1 - key;
//	int x = (key - 1) * 22 * display_scale.x;
//	int y = 9 * display_scale.y;
//	int width = 21 * display_scale.x;
//	int height = 7 * display_scale.y;
//	if (state) {
//	    // Construct a temporary pixbuf, create the inverted version of
//	    // the affected screen rectangle there, and blit it
//	    GdkPixbuf *tmpbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
//					       width, height);
//	    int s_bpl = gdk_pixbuf_get_rowstride(disp_image);
//	    int d_bpl = gdk_pixbuf_get_rowstride(tmpbuf);
//	    guchar *s1 = gdk_pixbuf_get_pixels(disp_image) + x * 3 + s_bpl * y;
//	    guchar *d1 = gdk_pixbuf_get_pixels(tmpbuf);
//	    for (int v = 0; v < height; v++) {
//		guchar *src = s1;
//		guchar *dst = d1;
//		for (int h = 0; h < width; h++) {
//		    unsigned char r = *src++;
//		    unsigned char g = *src++;
//		    unsigned char b = *src++;
//		    if (r == display_bg.r && g == display_bg.g && b == display_bg.b) {
//			*dst++ = display_fg.r;
//			*dst++ = display_fg.g;
//			*dst++ = display_fg.b;
//		    } else {
//			*dst++ = display_bg.r;
//			*dst++ = display_bg.g;
//			*dst++ = display_bg.b;
//		    }
//		}
//		s1 += s_bpl;
//		d1 += d_bpl;
//	    }
//	    gdk_draw_pixbuf(calc_widget->window, NULL, tmpbuf,
//			    0, 0,
//			    display_loc.x + x, display_loc.y + y,
//			    width, height,
//			    GDK_RGB_DITHER_NONE, 0, 0);
//	} else {
//	    // Repaint the screen
//	    gdk_draw_pixbuf(calc_widget->window, NULL, disp_image,
//			    x, y,
//			    display_loc.x + x, display_loc.y + y,
//			    width, height,
//			    GDK_RGB_DITHER_NONE, 0, 0);
//	}
//	return;
//    }
//
//    if (key < 0 || key >= nkeys)
//	return;
//    k = keylist + key;
//    if (state)
//	gdk_draw_pixbuf(calc_widget->window, NULL, skin_image,
//			k->src.x, k->src.y,
//			k->disp_rect.x, k->disp_rect.y,
//			k->disp_rect.width, k->disp_rect.height,
//			GDK_RGB_DITHER_MAX,
//			k->disp_rect.x, k->disp_rect.y);
//    else
//	gdk_draw_pixbuf(calc_widget->window, NULL, skin_image,
//			k->disp_rect.x, k->disp_rect.y,
//			k->disp_rect.x, k->disp_rect.y,
//			k->disp_rect.width, k->disp_rect.height,
//			GDK_RGB_DITHER_MAX,
//			k->disp_rect.x, k->disp_rect.y);
//}
//
//void skin_display_blitter(const char *bits, int bytesperline, int x, int y,
//				     int width, int height) {
//    guchar *pix = gdk_pixbuf_get_pixels(disp_image);
//    int disp_bpl = gdk_pixbuf_get_rowstride(disp_image);
//    int sx = display_scale.x;
//    int sy = display_scale.y;
//
//    for (int v = y; v < y + height; v++)
//	for (int h = x; h < x + width; h++) {
//	    SkinColor c;
//	    if ((bits[v * bytesperline + (h >> 3)] & (1 << (h & 7))) != 0)
//		c = display_fg;
//	    else
//		c = display_bg;
//	    for (int vv = v * sy; vv < (v + 1) * sy; vv++) {
//		guchar *p = pix + disp_bpl * vv;
//		for (int hh = h * sx; hh < (h + 1) * sx; hh++) {
//		    guchar *p2 = p + hh * 3;
//		    p2[0] = c.r;
//		    p2[1] = c.g;
//		    p2[2] = c.b;
//		}
//	    }
//	}
//    if (allow_paint && display_enabled)
//	gdk_draw_pixbuf(calc_widget->window, NULL, disp_image,
//			x * sx, y * sy,
//			display_loc.x + x * sx, display_loc.y + y * sy,
//			width * sx, height * sy,
//			GDK_RGB_DITHER_NONE, 0, 0);
//}
//
//void skin_repaint_display() {
//    if (display_enabled)
//	gdk_draw_pixbuf(calc_widget->window, NULL, disp_image,
//			0, 0, display_loc.x, display_loc.y,
//			131 * display_scale.x, 16 * display_scale.y,
//			GDK_RGB_DITHER_NONE, 0, 0);
//}
//
//void skin_display_set_enabled(bool enable) {
//    display_enabled = enable;
//}
}
