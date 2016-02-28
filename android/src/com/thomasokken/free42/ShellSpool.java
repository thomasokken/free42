/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2016  Thomas Okken
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

import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;

public class ShellSpool {

    public static boolean printToTxt;
    public static String printToTxtFileName = "";
    public static boolean rawText;
    public static boolean printToGif;
    public static String printToGifFileName = "";
    public static int maxGifHeight = 256;

    public static String hp2ascii(byte[] src) {
        StringBuffer buf = new StringBuffer();
        for (int i = 0; i < src.length; i++) {
            int c = src[i] & 255;
            if (c >= 130 && c != 138)
                c &= 127;
            String esc;
            switch (c) {
                /* NOTE: this code performs the following 12 translations
                 * that are not ASCII, but seem to be widely accepted --
                 * that is, they looked OK when I tried them in several
                 * fonts in Windows and Linux, and in Memo Pad on the Palm:
                 *
                 *   0: 247 (0367) divide
                 *   1: 215 (0327) multiply
                 *   8: 191 (0277) upside-down question mark
                 *  17: 181 (0265) lowercase mu
                 *  18: 163 (0243) sterling
                 *  19: 176 (0260) degree
                 *  20: 197 (0305) Aring
                 *  21: 209 (0321) Ntilde
                 *  22: 196 (0304) Aumlaut
                 *  25: 198 (0306) AE
                 *  28: 214 (0326) Oumlaut
                 *  29: 220 (0334) Uumlaut
                 *
                 * Two additional candidates are these:
                 *
                 *  26: 133 (0205) ellipsis
                 *  31: 149 (0225) bullet
                 *
                 * I'm not using those last two because support for them is not
                 * as good: they exist in Times New Roman and Adobe Courier
                 * (tested on Windows and Linux, respectively) and on the Palm,
                 * but are missing from Windows Fixedsys (the default Notepad
                 * font, so that is a concern!) and X11 lucidatypewriter and
                 * fixed.
                 * Note that 133 and 149 are both in the 128-159 range, that
                 * is, the Ctrl+Meta range, which is unused in many fonts.
                 * Eventually, I should probably support several translation
                 * modes: raw, pure ASCII (only emit codes 32-126 and 10),
                 * non-pure as below, and more aggressive non-pure (using the
                 * ellipsis and fatdot codes, and maybe others). Then again,
                 * maybe not. :-)
                 */
                case  0:   esc = "\367"; break;
                case  1:   esc = "\327"; break;
                case  2:   esc = "\\sqrt"; break;
                case  3:   esc = "\\int"; break;
                case  4:   esc = "\\gray1"; break;
                case  5:   esc = "\\Sigma"; break;
                case  6:   esc = ">"; break;
                case  7:   esc = "\\pi"; break;
                case  8:   esc = "\277"; break;
                case  9:   esc = "<="; break;
                case 11:   esc = ">="; break;
                case 12:   esc = "!="; break;
                case 13:   esc = "\\r"; break;
                case 14:   esc = "v"; break;
                case 15:   esc = "->"; break;
                case 16:   esc = "<-"; break;
                case 17:   esc = "\265"; break;
                case 18:   esc = "\243"; break;
                case 19:   esc = "\260"; break;
                case 20:   esc = "\305"; break;
                case 21:   esc = "\321"; break;
                case 22:   esc = "\304"; break;
                case 23:   esc = "\\angle"; break;
                case 24:   esc = "E"; break;
                case 25:   esc = "\306"; break;
                case 26:   esc = "..."; break;
                case 27:   esc = "\\esc"; break;
                case 28:   esc = "\326"; break;
                case 29:   esc = "\334"; break;
                case 30:   esc = "\\gray2"; break;
                case 31:   esc = "\\bullet"; break;
                case '\\': esc = "\\\\"; break;
                case 127:  esc = "|-"; break;
                case 128:  esc = ":"; break;
                case 129:  esc = "y"; break;
                case 138:  esc = "\\LF"; break;
                default:   buf.append((char) c); continue;
            }
            buf.append(esc);
        }
        return buf.toString();
    }

    public static void shell_spool_txt(byte[] text, OutputStream stream) throws IOException {
        if (rawText)
            stream.write(text);
        else
            stream.write(hp2ascii(text).getBytes());
        stream.write(10);
    }

    private static class GifData {
        int codesize;
        int bytecount;
        byte[] buf = new byte[255];
    
        short[] prefix_table = new short[4096];
        short[] code_table = new short[4096];
        short[] hash_next = new short[4096];
        short[] hash_head = new short[256];
    
        int maxcode;
        int clear_code;
        int end_code;
    
        int curr_code_size;
        int prefix;
        int currbyte;
        int bits_needed;
        boolean initial_clear;
        boolean really_done;
    
        int height;
    }

    private static GifData g = new GifData();

    public static void shell_start_gif(RandomAccessFile file, int provisional_height) throws IOException {
        byte[] buf = new byte[29];
        int p = 0;
        byte c;
        int width = 143;
        int height = provisional_height;
        int i;
    
        /* NOTE: the height will be set to the *actual* height once we know
         * what that is, i.e., when shell_finish_gif() is called. We populate
         * it using the maximum height (as set in the preferences dialog) so
         * that even incomplete GIF files will be viewable, just in case the
         * user is impatient and wants to take a peek.
         */
    
    
        /* GIF Header */
    
        buf[p++] = 'G';
        buf[p++] = 'I';
        buf[p++] = 'F';
        buf[p++] = '8';
        buf[p++] = '7';
        buf[p++] = 'a';
    
        /* Screen descriptor */
    
        buf[p++] = (byte) (width & 255);
        buf[p++] = (byte) (width >> 8);
        buf[p++] = (byte) (height & 255);
        buf[p++] = (byte) (height >> 8);
        buf[p++] = (byte) 0xF0;
        buf[p++] = 0;
        buf[p++] = 0;
    
        /* Global color map */
    
        buf[p++] = (byte) 255;
        buf[p++] = (byte) 255;
        buf[p++] = (byte) 255;
        buf[p++] = 0;
        buf[p++] = 0;
        buf[p++] = 0;
    
        /* Image Descriptor */
    
        buf[p++] = ',';
        buf[p++] = 0;
        buf[p++] = 0;
        buf[p++] = 0;
        buf[p++] = 0;
        buf[p++] = (byte) (width & 255);
        buf[p++] = (byte) (width >> 8);
        buf[p++] = (byte) (height & 255);
        buf[p++] = (byte) (height >> 8);
        buf[p++] = 0x00;
    
        /* Write GIF header & descriptors */
    
        file.write(buf);
    
    
        /* Initialize GIF encoder */
    
        g.codesize = 2;
        g.bytecount = 0;
        g.maxcode = 1 << g.codesize;
        for (i = 0; i < g.maxcode; i++) {
            g.prefix_table[i] = -1;
            g.code_table[i] = (short) i;
            g.hash_next[i] = -1;
        }
        for (i = 0; i < 256; i++)
            g.hash_head[i] = -1;
    
        g.clear_code = g.maxcode++;
        g.end_code = g.maxcode++;
    
        g.curr_code_size = g.codesize + 1;
        g.prefix = -1;
        g.currbyte = 0;
        g.bits_needed = 8;
        g.initial_clear = true;
        g.really_done = false;
    
        g.height = 0;
    
        c = (byte) g.codesize;
        file.write(c);
    }

    public static void shell_spool_gif(byte[] bits, int bytesperline,
             int x, int y, int width, int height,
             RandomAccessFile file) throws IOException {
        int v, h;
        g.height += height;
    
        /* Encode Image Data */
    
        for (v = y; v < y + height || (v == y && height == 0); v++) {
            boolean done = v == y && height == 0;
            hloop: for (h = 0; h < 143; h++) {
                int new_code = 0;
                byte hash_code;
                int hash_index;
                int pixel;
        
                if (g.really_done) {
                    new_code = g.end_code;
                } else if (done) {
                    new_code = g.prefix;
                }
        
                if (!g.really_done && !done) {
                    if (h < width)
                        pixel = ((bits[bytesperline * v + (h >> 3)]) >> (h & 7)) & 1;
                    else
                        pixel = 0;
            
                    /* Look for concat(prefix, pixel) in string table */
                    if (g.prefix == -1) {
                        g.prefix = pixel;
                        continue hloop;
                    }
                    
                    /* Compute hash code
                     * TODO: There's a lot of room for improvement here!
                     * I'm getting search percentages of over 30%; looking for
                     * something in single digits.
                     */
                    {
                        long xx = (((long) g.prefix) << 20) + (((long) pixel) << 12);
                        byte b1, b2, b3;
                        xx /= 997;
                        b1 = (byte) (xx >> 16);
                        b2 = (byte) (xx >> 8);
                        b3 = (byte) xx;
                        hash_code = (byte) (b1 ^ b2 ^ b3);
                    }
                    hash_index = g.hash_head[hash_code & 255];
                    while (hash_index != -1) {
                        if (g.prefix_table[hash_index] == g.prefix
                             && g.code_table[hash_index] == pixel) {
                            g.prefix = hash_index;
                            continue hloop;
                        }
                        hash_index = g.hash_next[hash_index];
                    }
                    
                    /* Not found: */
                    if (g.maxcode < 4096) {
                        g.prefix_table[g.maxcode] = (short) g.prefix;
                        g.code_table[g.maxcode] = (short) pixel;
                        g.hash_next[g.maxcode] = g.hash_head[hash_code & 255];
                        g.hash_head[hash_code & 255] = (short) g.maxcode;
                        g.maxcode++;
                    }
                    new_code = g.prefix;
                    g.prefix = pixel;
                }
                
                emit: while (true) {
                    int outcode = g.initial_clear ? g.clear_code
                                : g.really_done ? g.end_code : new_code;
                    int bits_available = g.curr_code_size;
                    while (bits_available != 0) {
                        int bits_copied = g.bits_needed < bits_available ?
                            g.bits_needed : bits_available;
                        int bits2 = outcode >> (g.curr_code_size - bits_available);
                        bits2 &= 255 >> (8 - bits_copied);
                        g.currbyte |= bits2 << (8 - g.bits_needed);
                        bits_available -= bits_copied;
                        g.bits_needed -= bits_copied;
                        if (g.bits_needed == 0 ||
                                (bits_available == 0 && g.really_done)) {
                            g.buf[g.bytecount++] = (byte) g.currbyte;
                            if (g.bytecount == 255) {
                                file.write(g.bytecount);
                                file.write(g.buf, 0, g.bytecount);
                                g.bytecount = 0;
                            }
                            if (bits_available == 0 && g.really_done)
                                return;
                            g.currbyte = 0;
                            g.bits_needed = 8;
                        }
                    }
            
                    if (done) {
                        done = false;
                        g.really_done = true;
                        continue emit;
                    }
                    if (g.initial_clear) {
                        g.initial_clear = false;
                        continue emit;
                    } else {
                        if (g.maxcode > (1 << g.curr_code_size)) {
                            g.curr_code_size++;
                        } else if (new_code == g.clear_code) {
                            int i;
                            g.maxcode = (1 << g.codesize) + 2;
                            g.curr_code_size = g.codesize + 1;
                            for (i = 0; i < 256; i++)
                                g.hash_head[i] = -1;
                        } else if (g.maxcode == 4096) {
                            new_code = g.clear_code;
                            continue emit;
                        }
                    }
                    break;
                }
            }
        }
    }

    public static void shell_finish_gif(RandomAccessFile file) throws IOException {
        /* Flush the encoder and write any remaining data */
    
        shell_spool_gif(null, 0, 0, 0, 0, 0, file);
    
        if (g.bytecount > 0) {
            file.write(g.bytecount);
            file.write(g.buf, 0, g.bytecount);
        }
        file.write(0);
    
        /* GIF Trailer */
    
        file.write(';');
    
        /* Update the 'height' fields in the header, now that at last
         * we know what the final height is */

        file.seek(8);
        file.write(g.height & 255);
        file.write(g.height >> 8);
    
        file.seek(26);
        file.write(g.height & 255);
        file.write(g.height >> 8);
    
        /* All done! */
    }
}
