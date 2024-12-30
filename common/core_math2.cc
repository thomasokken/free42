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

    // Calculating asinh(x) as i * asin(x / i)
    phloat zre = xim;
    phloat zim = -xre;

    phloat are, aim, bre, bim;
    math_sqrt(zre + 1, zim, &are, &aim);
    math_sqrt(-zre + 1, -zim, &bre, &bim);
    phloat x2 = atan(zre / (are * bre - aim * bim));
    phloat y2 = asinh(are * bim - aim * bre);

    *yre = -copysign(y2, zim);
    *yim = copysign(x2, zre);
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

    phloat are, aim, bre, bim;

    /* a = sqrt(x - 1) */
    math_sqrt(xre - 1, xim, &are, &aim);
    /* b = sqrt(x + 1) */
    math_sqrt(xre + 1, xim, &bre, &bim);

    *yre = asinh(are * bre + aim * bim);
    *yim = atan(aim / bre) * 2;

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

    phloat x = fabs(xim);
    phloat y = fabs(xre);

#ifdef BCD_MATH
    const Phloat BIG(1000000000000000000LL);
#else
    const double BIG = 0x8000000; // 2^27
#endif

    if (x >= BIG || y >= BIG) { // atan(1/z) ~= 1/z
        math_inv(x, y, &x, &y); // atan(x+y*I) ~= pi/2 + t
        x += PI / 2;            // |re(t^3/3)/re(t)| <= 0x1p-54
                                // |im(t^3/3)/im(t)| <= 0x1p-54
    } else {
        phloat x2 = x * x;
        phloat ym = 1 - y;
        x = atan2(2 * x, (1 + y) * ym - x2) / 2;
        y = log1p(4 * y / (ym * ym + x2)) / 4;
    }

    x = fabs(x);
    if (xim < 0)
        x = -x;
    y = fabs(y);
    if (xre < 0)
        y = -y;

    *yre = y;
    *yim = x;
    return ERR_NONE;
}

int math_ln(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    if (xim == 0) {
        if (xre == 0)
            return ERR_INVALID_DATA;
        if (xre > 0) {
            *yre = log(xre);
            *yim = 0;
        } else {
            *yre = log(-xre);
            *yim = PI;
        }
        return ERR_NONE;
    } else if (xre == 0) {
        if (xim > 0) {
            *yre = log(xim);
            *yim = PI / 2;
        } else {
            *yre = log(-xim);
            *yim = -PI / 2;
        }
        return ERR_NONE;
    } else {
        phloat h = xre * xre + xim * xim;
        phloat a = atan2(xim, xre);
        if (h > 0.5 && h < 3.0) {
            if ((h = fabs(xre)) < (xim = fabs(xim))) {
                h = xim;
                xim = xre;
            }
            h -= 1; /* x^2-1 = 2*(x-1) + (x-1)^2 */
            h = log1p(2 * h + h * h + xim * xim) / 2;
        } else if (p_isnormal(h)) {
            h = log(h) / 2;
        } else {
            #ifdef BCD_MATH
            phloat m = scalbn(phloat(1), 38);
            phloat b = -log(phloat(10)) * 38;
            #else
            phloat m = 0x2000000000000000; // 2^61
            phloat b = -log(phloat(2)) * 61;
            #endif
            if (p_isinf(h)) {
                m = 1 / m;
                b = -b;
            }
            h = log(hypot(m * xre, m * xim)) + b;
        }
        *yre = h;
        *yim = a;
        return ERR_NONE;
    }
}

int math_sqrt(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    if (xre == 0) {
        if (xim == 0) {
            *yre = 0;
            *yim = 0;
        } else {
            bool neg = xim < 0;
            if (neg)
                xim = -xim;
            phloat r;
            if (xim > 1)
                r = sqrt(xim / 2);
            else
                r = sqrt(xim * 2) / 2;
            *yre = r;
            *yim = neg ? -r : r;
        }
        return ERR_NONE;
    } else if (xim == 0) {
        if (xre > 0) {
            *yre = sqrt(xre);
            *yim = 0;
        } else {
            *yre = 0;
            *yim = sqrt(-xre);
        }
        return ERR_NONE;
    }

    phloat r = hypot(xre, xim);
    phloat a = sqrt((r + fabs(xre)) / 2);
    phloat b = xim / (a * 2);

    if (p_isinf(a)) {
        xre /= 100;
        xim /= 100;
        r = hypot(xre, xim);
        a = sqrt((r + fabs(xre)) / 2);
        b = xim / (a * 2);
        a *= 10;
        b *= 10;
    }

    if (xre >= 0) {
        *yre = a;
        *yim = b;
    } else if (xim >= 0) {
        *yre = b;
        *yim = a;
    } else {
        *yre = -b;
        *yim = -a;
    }
    return ERR_NONE;
}

int math_inv(phloat xre, phloat xim, phloat *yre, phloat *yim) {
    phloat r, t, rre, rim;
    int inf;
    if (xre == 0 && xim == 0)
        return ERR_DIVIDE_BY_0;
    if (fabs(xim) <= fabs(xre)) {
        r = xim / xre;
        t = 1 / (xre + xim * r);
        if (r == 0) {
            rre = t;
            rim = -xim * (1 / xre) * t;
        } else {
            rre = t;
            rim = -r * t;
        }
    } else {
        r = xre / xim;
        t = 1 / (xre * r + xim);
        if (r == 0) {
            rre = xre * (1 / xim) * t;
            rim = -t;
        } else {
            rre = r * t;
            rim = -t;
        }
    }
    inf = p_isinf(rre);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rre = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    inf = p_isinf(rim);
    if (inf != 0) {
        if (flags.f.range_error_ignore)
            rim = inf == 1 ? POS_HUGE_PHLOAT : NEG_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    *yre = rre;
    *yim = rim;
    return ERR_NONE;
}
