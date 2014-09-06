#include <stdio.h>

int main() {
    int c;
    char *esc;

    while ((c = getchar()) != EOF) {
        /* First, undo 42S-to-82240A (Roman8) translations: */
        switch (c) {
            case   4: c =  10; break; // line feed
            case 127: c =  30; break; // gray2
            case 129: c =   0; break; // divide
            case 130: c =   1; break; // multiply
            case 131: c =   2; break; // sqrt
            case 132: c =   3; break; // integral
            case 133: c =   5; break; // Sigma
            case 134: c =   6; break; // goose
            case 135: c =   7; break; // pi
            case 137: c =   9; break; // lessequal
            case 138: c =  11; break; // greaterequal
            case 139: c =  12; break; // notequal
            case 141: c =  15; break; // arrowright
            case 142: c =  16; break; // arrowleft
            case 143: c =  17; break; // mu
            case 144: c = 138; break; // LF symbol
            case 148: c = 127; break; // append
            case 155: c =  26; break; // ellipsis
            case 160: c =  23; break; // angle
            case 162: c =  20; break; // Aring
            case 175: c =  18; break; // sterling
            case 179: c =  19; break; // degree
            case 182: c =  21; break; // Ntilde
            case 185: c =   8; break; // upside-down question mark
            case 211: c =  25; break; // AE
            case 216: c =  22; break; // Aumlaut
            case 218: c =  28; break; // Oumlaut
            case 219: c =  29; break; // Uumlaut
            case 242: c =  31; break; // bullet
        }

        /* Next, 42S-to-ASCII translation, exactly as performed by Free42: */
        if (c >= 130 && c != 138)
            c &= 127;
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
             * ellipsis and bullet codes, and maybe others). Then again,
             * maybe not. :-)
             */
            case    0: esc = "\367"; break;
            case    1: esc = "\327"; break;
            case    2: esc = "\\sqrt"; break;
            case    3: esc = "\\int"; break;
            case    4: esc = "\\gray1"; break;
            case    5: esc = "\\Sigma"; break;
            case    6: esc = ">"; break;
            case    7: esc = "\\pi"; break;
            case    8: esc = "\277"; break;
            case    9: esc = "<="; break;
            case   11: esc = ">="; break;
            case   12: esc = "!="; break;
            case   13: esc = "\\r"; break;
            case   14: esc = "v"; break;
            case   15: esc = "->"; break;
            case   16: esc = "<-"; break;
            case   17: esc = "\265"; break;
            case   18: esc = "\243"; break;
            case   19: esc = "\260"; break;
            case   20: esc = "\305"; break;
            case   21: esc = "\321"; break;
            case   22: esc = "\304"; break;
            case   23: esc = "\\angle"; break;
            case   24: esc = "E"; break;
            case   25: esc = "\306"; break;
            case   26: esc = "..."; break;
            case   27: esc = "\\esc"; break;
            case   28: esc = "\326"; break;
            case   29: esc = "\334"; break;
            case   30: esc = "\\gray2"; break;
            case   31: esc = "\\bullet"; break;
            case '\\': esc = "\\\\"; break;
            case  127: esc = "|-"; break;
            case  128: esc = ":"; break;
            case  129: esc = "y"; break;
            case  138: esc = "\\LF"; break;
            default: putchar(c); continue;
        }
        fputs(esc, stdout);
    }
    return 0;
}
