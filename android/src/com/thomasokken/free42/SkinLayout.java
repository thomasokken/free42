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
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.List;
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
	private SkinPoint display_scale = new SkinPoint();
	private int display_bg, display_fg;
	private SkinKey[] keylist = null;
	private SkinMacro[] macrolist = null;
	private SkinAnnunciator[] annunciators = new SkinAnnunciator[7];
	private boolean display_enabled = true;
	private Bitmap skinBitmap;
	private Bitmap display = Bitmap.createBitmap(131, 16, Bitmap.Config.ARGB_8888);
	private IntBuffer display_buffer = IntBuffer.allocate(131 * 16);
	private boolean[] ann_state = new boolean[7];
	private int active_key = -1;

	public SkinLayout(String skinName) {
		try {
			InputStream is = getClass().getResourceAsStream(skinName + ".gif");
			if (is == null)
				throw new IllegalArgumentException("No image found for skin " + skinName);
	    	skinBitmap = new BitmapDrawable(is).getBitmap();
			is = getClass().getResourceAsStream(skinName + ".layout");
			if (is == null)
				throw new IllegalArgumentException("No layout found for skin " + skinName);
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
	//			    keymap_entry *entry = parse_keymap_entry(line, lineno);
	//			    if (entry != NULL) {
	//					if (keymap_length == kmcap) {
	//					    kmcap += 50;
	//					    keymap = (keymap_entry *)
	//							realloc(keymap, kmcap * sizeof(keymap_entry));
	//					}
	//					memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
	//			    }
				}
			}
			keylist = tempkeylist.toArray(new SkinKey[0]);
			macrolist = tempmacrolist.toArray(new SkinMacro[0]);
		} catch (IOException e) {
			throw new IllegalArgumentException(e);
		}
    }
	
	public void skin_repaint_annunciator(Canvas canvas, Bitmap skin, int which, boolean state) {
		if (!display_enabled)
			return;
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
			int x = (-2 - skey) * 22 * display_scale.x + display_loc.x;
			int y = 9 * display_scale.y + display_loc.y;
			return new Rect(x, y, x + 21 * display_scale.x, y + 7 * display_scale.y);
		} else if (skey >= 0 && skey < keylist.length) {
			SkinRect r = keylist[skey].disp_rect;
			return new Rect(r.x, r.y, r.x + r.width, r.y + r.height);
		} else
			return null;
	}

	public void skin_find_key(boolean menu_active, int x, int y, IntHolder skey, IntHolder ckey) {
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
	
	private void skin_repaint_key(Canvas canvas, Bitmap skin, int key, boolean state) {
	    if (key >= -7 && key <= -2) {
			/* Soft key */
			if (!display_enabled)
			    // Should never happen -- the display is only disabled during macro
			    // execution, and softkey events should be impossible to generate
			    // in that state. But, just staying on the safe side.
			    return;
			key = -1 - key;
			Rect dst = new Rect(display_loc.x + (key - 1) * 22 * display_scale.x,
								display_loc.y + 9 * display_scale.y,
								display_loc.x + ((key - 1) * 22 + 21) * display_scale.x,
								display_loc.y + 16 * display_scale.y);
			if (state) {
			    // Construct a temporary Bitmap, create the inverted version of
			    // the affected screen rectangle there, and blit it
				Bitmap invSoftkey = Bitmap.createBitmap(21, 7, Bitmap.Config.ARGB_8888);
				for (int v = 0; v < 7; v++)
					for (int h = 0; h < 21; h++) {
						int color = display.getPixel(h + (key - 1) * 22, v + 9);
						color = color == display_bg ? display_fg : display_bg;
						invSoftkey.setPixel(h, v, color);
					}
				Rect src = new Rect(0, 0, 21, 7);
				Paint p = new Paint();
				canvas.drawBitmap(invSoftkey, src, dst, p);
			} else {
			    // Repaint the screen
				Rect src = new Rect((key - 1) * 22, 9, (key - 1) * 22 + 21, 16);
				Paint p = new Paint();
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
	    canvas.drawBitmap(skin, src, dst, p);
	}
	
	public Rect skin_display_blitter(byte[] bits, int bytesperline, int x, int y, int width, int height) {
    	int hmax = x + width;
    	int vmax = y + height;
    	int[] buf = display_buffer.array();
    	for (int v = y; v < vmax; v++)
    		for (int h = x; h < hmax; h++) {
    			boolean bitSet = (bits[(h >> 3) + v * bytesperline] & (1 << (h & 7))) != 0;
    			buf[v * 131 + h] = bitSet ? display_fg : display_bg;
    		}
    	display.copyPixelsFromBuffer(display_buffer);
    	
    	// Return a Rect telling the caller what part of the View needs to be invalidated
    	return new Rect(display_loc.x + x * display_scale.x,
    					display_loc.y + y * display_scale.y,
    					display_loc.x + (x + width) * display_scale.x,
    					display_loc.y + (y + height) * display_scale.y);
	}
	
	public Rect skin_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
		int minx = Integer.MAX_VALUE;
		int miny = Integer.MAX_VALUE;
		int maxx = Integer.MIN_VALUE;
		int maxy = Integer.MIN_VALUE;
		
		int[] ann_arg = new int[] { updn, shf, prt, run, -1, g, rad };
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
	
	public void skin_repaint(Canvas canvas) {
	    Rect clip = canvas.getClipBounds();
		boolean paintDisplay = false;
		boolean paintSkin = false;
    	Rect disp = new Rect(display_loc.x,
							 display_loc.y,
							 display_loc.x + 131 * display_scale.x,
							 display_loc.y + 16 * display_scale.y);
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
			canvas.drawBitmap(skinBitmap, 0, 0, p);
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
						canvas.drawBitmap(skinBitmap, src, dst, p);
					}
			if (active_key >= 0)
				skin_repaint_key(canvas, skinBitmap, active_key, true);
		}
		
		if (paintDisplay) {
			Rect src = new Rect(0, 0, 131, 16);
	    	Rect dst = new Rect(display_loc.x,
	    						display_loc.y,
	    						display_loc.x + 131 * display_scale.x,
	    						display_loc.y + 16 * display_scale.y);
	    	canvas.drawBitmap(display, src, dst, p);
			if (active_key >= -7 && active_key <= -2)
				skin_repaint_key(canvas, skinBitmap, active_key, true);
		}
	}
	
	public Rect skin_display_set_enabled(boolean enable) {
		if (display_enabled)
			return null;
	    display_enabled = enable;
	    Rect r = new Rect(display_loc.x, display_loc.y, display_loc.x + 131 * display_scale.x, display_loc.y + 16 * display_scale.y);
	    for (int i = 0; i < 7; i++) {
	    	SkinRect a = annunciators[i].disp_rect;
	    	Rect ra = new Rect(a.x, a.y, a.x + a.width, a.y + a.height);
	    	r.union(ra);
	    }
	    return r;
	}
}
