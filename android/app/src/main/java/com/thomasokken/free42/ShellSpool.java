/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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
    public static boolean printToGif;
    public static String printToGifFileName = "";
    public static int maxGifHeight = 256;

    public static byte[] hp2utf8(byte[] src) {
        StringBuffer buf = new StringBuffer();
        for (int i = 0; i < src.length; i++) {
            int c = src[i] & 255;
            if (c >= 130 && c != 138)
                c &= 127;
            String esc;
            switch (c) {
                case  0:   esc = "\u00f7"; break; // division sign
                case  1:   esc = "\u00d7"; break; // multiplication sign
                case  2:   esc = "\u221a"; break; // square root sign
                case  3:   esc = "\u222b"; break; // integral sign
                case  4:   esc = "\u2592"; break; // gray rectangle
                case  5:   esc = "\u03a3"; break; // uppercase sigma
                case  6:   esc = "\u25b8"; break; // small right-pointing triangle
                case  7:   esc = "\u03c0"; break; // lowercase pi
                case  8:   esc = "\u00bf"; break; // upside-down question mark
                case  9:   esc = "\u2264"; break; // less-than-or-equals sign
                case 11:   esc = "\u2265"; break; // greater-than-or-equals sign
                case 12:   esc = "\u2260"; break; // not-equals sign
                case 13:   esc = "\u21b5"; break; // down-then-left arrow
                case 14:   esc = "\u2193"; break; // downward-pointing arrow
                case 15:   esc = "\u2192"; break; // right-pointing arrow
                case 16:   esc = "\u2190"; break; // left-pointing arrow
                case 17:   esc = "\u03bc"; break; // lowercase mu
                case 18:   esc = "\u00a3"; break; // pound sterling sign
                case 19:   esc = "\u00b0"; break; // degree symbol
                case 20:   esc = "\u00c5"; break; // uppercase a with ring
                case 21:   esc = "\u00d1"; break; // uppercase n with tilde
                case 22:   esc = "\u00c4"; break; // uppercase a with umlaut
                case 23:   esc = "\u2221"; break; // measured angle symbol
                case 24:   esc = "\u1d07"; break; // small-caps e
                case 25:   esc = "\u00c6"; break; // uppercase ae ligature
                case 26:   esc = "\u2026"; break; // ellipsis
                case 27:   esc = "[ESC]"; break;  // EC symbol
                case 28:   esc = "\u00d6"; break; // uppercase o with umlaut
                case 29:   esc = "\u00dc"; break; // uppercase u with umlaut
                case 30:   esc = "\u2592"; break; // gray rectangle
                case 31:   esc = "\u2022"; break; // bullet
                case 94:   esc = "\u2191"; break; // upward-pointing arrow
                case 127:  esc = "\u251c"; break; // append sign
                case 128:  esc = ":"; break;      // thin colon
                case 129:  esc = "\u028f"; break; // small-caps y
                case 138:  esc = "[LF]"; break;   // LF symbol
                default:   buf.append((char) c); continue;
            }
            buf.append(esc);
        }
        try {
            return buf.toString().getBytes("UTF-8");
        } catch (IOException e) {
            // Won't happen
            return null;
        }
    }

    public static void shell_spool_txt(byte[] text, OutputStream stream) throws IOException {
        stream.write(hp2utf8(text));
        stream.write(13);
        stream.write(10);
    }

    public static void shell_spool_bitmap_to_txt(byte[] bits, int bytesperline, int x, int y,
            int width, int height, OutputStream stream) throws IOException {
        StringBuffer buf = new StringBuffer();
        for (int v = 0; v < height; v += 2) {
            for (int h = 0; h < width; h += 2) {
                int k = 0;
                for (int vv = 0; vv < 2; vv++) {
                    int vvv = v + vv + y;
                    if (vvv >= height)
                        break;
                    for (int hh = 0; hh < 2; hh++) {
                        int hhh = h + hh + x;
                        if (hhh >= width)
                            break;
                        if ((bits[vvv * bytesperline + (hhh >> 3)] & (1 << (hhh & 7))) != 0)
                            k += 1 << (hh + 2 * vv);
                    }
                }
                switch (k) {
                    case  0: buf.append("\u00a0"); break;
                    case  1: buf.append("\u2598"); break;
                    case  2: buf.append("\u259d"); break;
                    case  3: buf.append("\u2580"); break;
                    case  4: buf.append("\u2596"); break;
                    case  5: buf.append("\u258c"); break;
                    case  6: buf.append("\u259e"); break;
                    case  7: buf.append("\u259b"); break;
                    case  8: buf.append("\u2597"); break;
                    case  9: buf.append("\u259a"); break;
                    case 10: buf.append("\u2590"); break;
                    case 11: buf.append("\u259c"); break;
                    case 12: buf.append("\u2584"); break;
                    case 13: buf.append("\u2599"); break;
                    case 14: buf.append("\u259f"); break;
                    case 15: buf.append("\u2588"); break;
                }
                stream.write(buf.toString().getBytes("UTF-8"));
                buf.delete(0, buf.length());
            }
            stream.write(13);
            stream.write(10);
        }
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
