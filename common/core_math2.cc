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

#include "core_globals.h"
#include "core_math2.h"

phloat math_random() {
    if (random_number_low == 0 && random_number_high == 0) {
        random_number_high = 0;
        random_number_low = 2787;
        // The RPL RAND/RDZ functions differ from the HP-42S RAN/SEED
        // only in their initial seed, which is this:
        // random_number_high = 9995003;
        // random_number_low = 33083533;
    }
    int8 temp = random_number_low * 30928467;
    random_number_high = (random_number_low * 28511 + random_number_high * 30928467 + temp / 100000000) % 10000000;
    random_number_low = temp % 100000000;
    if (random_number_high >= 1000000) {
        temp = random_number_low / 1000;
        #ifdef BCD_MATH
            const Phloat n1(1000000000000LL);
            const Phloat n2(10000000);
            return Phloat(temp) / n1 + Phloat(random_number_high) / n2;
        #else
            return temp / 1000000000000.0 + random_number_high / 10000000.0;
        #endif
    } else if (random_number_high >= 100000) {
        temp = random_number_low / 100;
        #ifdef BCD_MATH
            const Phloat n1(10000000000000LL);
            const Phloat n2(10000000);
            return Phloat(temp) / n1 + Phloat(random_number_high) / n2;
        #else
            return temp / 10000000000000.0 + random_number_high / 10000000.0;
        #endif
    } else if (random_number_high >= 10000) {
        temp = random_number_low / 10;
        #ifdef BCD_MATH
            const Phloat n1(100000000000000LL);
            const Phloat n2(10000000);
            return Phloat(temp) / n1 + Phloat(random_number_high) / n2;
        #else
            return temp / 100000000000000.0 + random_number_high / 10000000.0;
        #endif
    } else {
        #ifdef BCD_MATH
            const Phloat n1(1000000000000000LL);
            const Phloat n2(10000000);
            return Phloat(random_number_low) / n1 + Phloat(random_number_high) / n2;
        #else
            return random_number_low / 1000000000000000.0 + random_number_high / 10000000.0;
        #endif
    }
}

int math_tan(phloat x, phloat *y, bool rad) {
    if (rad || flags.f.rad) {
        *y = tan(x);
    } else if (flags.f.grad) {
        bool neg = false;
        if (x < 0) {
            x = -x;
            neg = true;
        }
        // [0 200[
        x = fmod(x, 200);
        if (x == 100)
            goto infinite;
        // TAN(x+100gon) = -TAN(100gon-x)
        if (x > 100) {
            x = 200 - x;
            neg = !neg;
        }
        // to improve accuracy for x close to 100gon
        if (x > 89)
            *y = 1 / tan((100 - x) / (200 / PI));
        else
            *y = tan(x / (200 / PI)); 
        if (neg)
            *y = -(*y);
    } else {
        bool neg = false;
        if (x < 0) {
            x = -x;
            neg = true;
        }
        // [0 180[
        x = fmod(x, 180);
        if (x == 90)
            goto infinite;
        // TAN(x+90°) = -TAN(90°-x)
        if (x > 90) {
            x = 180 - x;
            neg = !neg;
        }
        // to improve accuracy for x close to 90°
        if (x > 80)
            *y = 1 / tan((90 - x) / (180 / PI));
        else
            *y = tan(x / (180 / PI)); 
        if (neg)
            *y = -(*y);
    }
    if (p_isnan(*y) || p_isinf(*y) != 0) {
        infinite:
        if (flags.f.range_error_ignore)
            *y = POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    return ERR_NONE;
}

int math_asinh(phloat xre, phloat xim, phloat *yre, phloat *yim) {

    if (xim == 0) {
        *yre = asinh(xre);
        *yim = 0;
        return ERR_NONE;
    } else if (xre == 0) {
        if (xim > 1) {
            *yre = acosh(xim);
            *yim = PI / 2;
        } else if (xim < -1) {
            *yre = -acosh(-xim);
            *yim = -PI / 2;
        } else {
            *yre = 0;
            *yim = asin(xim);
        }
        return ERR_NONE;
    }

    /* TODO: review; and deal with overflows in intermediate results */
    phloat are, aim, br, bphi;

    /* If re(x)<0, we calculate asinh(x)=-asinh(-x); this avoids the loss of
     * significance in x+sqrt(x^2+1) when x is large and sqrt(x^2+1) approaches
     * -x.
     */
    int neg = xre < 0;
    if (neg) {
        xre = -xre;
        xim = -xim;
    }

    /* a = x ^ 2 + 1 */
    are = xre * xre - xim * xim + 1;
    aim = 2 * xre * xim;

    /* b = sqrt(a) */
    br = sqrt(hypot(are, aim));
    bphi = atan2(aim, are) / 2;

    /* a = b + x */
    p_sincos(bphi, &aim, &are);
    are = are * br + xre;
    aim = aim * br + xim;

    /* y = log(a) */
    *yre = log(hypot(are, aim));
    *yim = atan2(aim, are);
    if (neg) {
        *yre = -*yre;
        *yim = -*yim;
    }
    return ERR_NONE;
}

int math_acosh(phloat xre, phloat xim, phloat *yre, phloat *yim) {

    if (xim == 0) {
        if (xre >= 1) {
            *yre = acosh(xre);
            *yim = 0;
        } else if (xre <= -1) {
            *yre = acosh(-xre);
            *yim = PI;
        } else {
            *yre = 0;
            *yim = acos(xre);
        }
        return ERR_NONE;
    } else if (xre == 0) {
        if (xim > 0) {
            *yre = asinh(xim);
            *yim = PI / 2;
        } else {
            *yre = -asinh(xim);
            *yim = -PI / 2;
        }
        return ERR_NONE;
    }

    /* TODO: review; and deal with overflows in intermediate results */
    phloat ar, aphi, are, aim, br, bphi, bre, bim, cre, cim;

    /* a = sqrt(x + 1) */
    ar = sqrt(hypot(xre + 1, xim));
    aphi = atan2(xim, xre + 1) / 2;
    p_sincos(aphi, &aim, &are);
    are *= ar;
    aim *= ar;

    /* b = sqrt(x - 1) */
    br = sqrt(hypot(xre - 1, xim));
    bphi = atan2(xim, xre - 1) / 2;
    p_sincos(bphi, &bim, &bre);
    bre *= br;
    bim *= br;

    /* c = x + a * b */
    cre = xre + are * bre - aim * bim;
    cim = xim + are * bim + aim * bre;

    /* y = log(c) */
    *yre = log(hypot(cre, cim));
    *yim = atan2(cim, cre);
    return ERR_NONE;
}

int math_atanh(phloat xre, phloat xim, phloat *yre, phloat *yim) {

    if (xim == 0) {
        if (xre == 1 || xre == -1)
            return ERR_INVALID_DATA;
        else if (xre > -1 && xre < 1) {
            *yre = atanh(xre);
            *yim = 0;
            return ERR_NONE;
        } else {
            *yre = atanh(1 / xre);
            if (xre > 1)
                *yim = -PI / 2;
            else
                *yim = PI / 2;
            return ERR_NONE;
        }
    } else if (xre == 0) {
        *yre = 0;
        *yim = atan(xim);
        return ERR_NONE;
    }

    phloat are, aim, bre, bim, cre, cim, h;

    /* TODO: review, and deal with overflows in intermediate results */

    /* a = 1 + x */
    are = 1 + xre;
    aim = xim;

    /* b = 1 - x */
    bre = 1 - xre;
    bim = - xim;

    /* c = a / b */
    h = hypot(bre, bim);
    cre = (are * bre + aim * bim) / h / h;
    cim = (aim * bre - are * bim) / h / h;

    /* y = log(c) / 2 */
    *yre = log(hypot(cre, cim)) / 2;
    *yim = atan2(cim, cre) / 2;

    /* Theoretically, you could go out of range, but in practice,
     * you can't get close enough to the critical values to cause
     * trouble.
     */
    return ERR_NONE;
}
