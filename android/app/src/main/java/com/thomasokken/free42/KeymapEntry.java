/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2024  Thomas Okken
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

import java.io.ByteArrayOutputStream;
import java.text.NumberFormat;
import java.util.StringTokenizer;

public class KeymapEntry {
    private static final int KEYMAP_MAX_MACRO_LENGTH = 31;
    public boolean ctrl;
    public boolean alt;
    public boolean numpad;
    public boolean shift;
    public boolean cshift;
    public String keychar;
    public byte[] macro;

    public static KeymapEntry parse(String line, int lineno) {
        int p = line.indexOf('#');
        if (p != -1)
            line = line.substring(0, p);
        p = line.indexOf('\n');
        if (p != -1)
            line = line.substring(0, p);
        p = line.indexOf('\r');
        if (p != -1)
            line = line.substring(0, p);

        p = line.indexOf(':');
        if (p != -1) {
            StringTokenizer t = new StringTokenizer(line.substring(0, p), " \t", false);
            String val = line.substring(p + 1);
            String tok;
            boolean ctrl = false;
            boolean alt = false;
            boolean numpad = false;
            boolean shift = false;
            boolean cshift = false;
            String keychar = null;
            boolean done = false;
            ByteArrayOutputStream macro = new ByteArrayOutputStream();
            int macrolen = 0;
            
            /* Parse keycode */
            while (t.hasMoreTokens()) {
                tok = t.nextToken();
                if (done) {
                    System.err.println("Keymap, line " + lineno + ": Excess tokens in key spec.");
                    return null;
                }
                if (tok.equalsIgnoreCase("ctrl"))
                    ctrl = true;
                else if (tok.equalsIgnoreCase("alt"))
                    alt = true;
                else if (tok.equalsIgnoreCase("numpad"))
                    numpad = true;
                else if (tok.equalsIgnoreCase("shift"))
                    shift = true;
                else if (tok.equalsIgnoreCase("cshift"))
                    cshift = true;
                else {
                    boolean success = false;
                    try {
                        if (tok.length() > 2 && tok.substring(0, 2).equalsIgnoreCase("0x")) {
                            keychar = "" + (char) Integer.parseInt(tok.substring(2), 16);
                            success = true;
                        }
                    } catch (NumberFormatException e) {}
                    if (!success) {
                        keychar = tok;
                    }
                    done = true;
                }
            }
            if (!done) {
                System.err.println("Keymap, line " + lineno + ": Unrecognized keycode.");
                return null;
            }
            
            /* Parse macro */
            t = new StringTokenizer(val, " \t");
            while (t.hasMoreTokens()) {
                tok = t.nextToken();
                short k;
                try {
                    k = Short.parseShort(tok);
                } catch (NumberFormatException e) {
                    System.err.println("Keymap, line " + lineno + ": Bad value (" + tok + ") in macro.");
                    return null;
                }
                if (k < 1 || k > 255) {
                    System.err.println("Keymap, line " + lineno + ": Bad value (" + tok + ") in macro.");
                    return null;
                } else if (macrolen == KEYMAP_MAX_MACRO_LENGTH) {
                    System.err.println("Keymap, line " + lineno + ": Macro too long (max=" + KEYMAP_MAX_MACRO_LENGTH + ").");
                    return null;
                } else
                    macro.write(k);
            }

            KeymapEntry entry = new KeymapEntry();
            entry.ctrl = ctrl;
            entry.alt = alt;
            entry.numpad = numpad;
            entry.shift = shift;
            entry.cshift = cshift;
            entry.keychar = keychar;
            entry.macro = macro.toByteArray();
            return entry;
        } else
            return null;
    }
}
