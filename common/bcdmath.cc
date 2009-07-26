/**
 * Copyright (c) 2005-2009 voidware ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "bcdmath.h"
#include "bcd2.h"

#define BCD_CONST_PI    0
#define BCD_CONST_PI2   1
#define BCD_CONST_PIBY2 2
#define BCD_CONST_PIBY32 3
#define BCD_CONST_SINTAB 4
#define BCD_CONST_COSTAB (BCD_CONST_SINTAB+8)
#define BCD_CONST_LN2    (BCD_CONST_COSTAB+8)
#define BCD_CONST_ONEOTWO (BCD_CONST_LN2+1)
#define BCD_CONST_LN10    (BCD_CONST_ONEOTWO+1)
#define BCD_CONST_ATANLIM (BCD_CONST_LN10+1)
#define BCD_CONST_PIBY32A (BCD_CONST_ATANLIM+1)
#define BCD_CONST_PIBY32B (BCD_CONST_PIBY32A+1)
#define BCD_CONST_HUNDREDTH (BCD_CONST_PIBY32B+1)
#define BCD_CONST_HALF (BCD_CONST_HUNDREDTH+1)
#define BCD_CONST_TWOHALF (BCD_CONST_HALF+1)
#define BCD_CONST_TEN      (BCD_CONST_TWOHALF + 1)
#define BCD_CONST_SINPOLY (BCD_CONST_TEN+1)
#define BCD_CONST_COSPOLY (BCD_CONST_SINPOLY+7)
#define BCD_CONST_LOGPOLY (BCD_CONST_COSPOLY+7)
#define BCD_CONST_LOGCK   (BCD_CONST_LOGPOLY+4)


typedef unsigned short Dig[P+1];
typedef unsigned short Dig2[P2+1];

static const Dig constTable[] = 
{
    { 3, 1415, 9265, 3589, 7932, 3846, 2643, 1 }, // pi
    { 6, 2831, 8530, 7179, 5864, 7692, 5287, 1 }, // 2pi
    { 1, 5707, 9632, 6794, 8966, 1923, 1322, 1 }, // pi/2
    { 981, 7477, 424, 6810, 3870, 1957, 6057, 0 }, // pi/32

    /* table for sin & cos */
    { 980, 1714, 329, 5606, 199, 4195, 5639, 0 }, // sin(pi/32)
    { 1950, 9032, 2016, 1282, 6784, 8284, 8685, 0 }, // sin(pi/16)
    { 2902, 8467, 7254, 4623, 6763, 6192, 3758, 0 }, // sin(3pi/32)
    { 3826, 8343, 2365, 897, 7172, 8459, 9840, 0 }, // sin(pi/8)
    { 4713, 9673, 6825, 9976, 4855, 6387, 6259, 0 }, // sin(5pi/32)
    { 5555, 7023, 3019, 6022, 2474, 2830, 8139, 0 }, // sin(3pi/16)
    { 6343, 9328, 4163, 6454, 9821, 5171, 6132, 0 }, // sin(7pi/32)
    { 7071, 678, 1186, 5475, 2440, 844, 3621, 0 }, // sin(pi/4)

    { 9951, 8472, 6672, 1968, 8624, 4836, 9531, 0 }, // cos(pi/32)
    { 9807, 8528, 403, 2304, 4912, 6182, 2361, 0 }, // cos(pi/16)
    { 9569, 4033, 5732, 2088, 6493, 5797, 8870, 0 }, // cos(3pi/32)
    { 9238, 7953, 2511, 2867, 5612, 8183, 1894, 0 }, // cos(pi/8)
    { 8819, 2126, 4348, 3550, 2971, 2756, 8637, 0 }, // cos(5pi/32)
    { 8314, 6961, 2302, 5452, 3707, 8788, 3776, 0 }, // cos(3pi/16)
    { 7730, 1045, 3362, 7369, 6081, 906, 6098, 0 }, // cos(7pi/32)
    { 7071, 678, 1186, 5475, 2440, 844, 3621, 0 }, // cos(pi/4)

    { 6931, 4718, 559, 9453, 941, 7232, 1215, 0 }, // ln(2)
    { 1, 200, 0, 0, 0, 0, 0, 1 }, // 1.02
    { 2, 3025, 8509, 2994, 456, 8401, 7991, 1 }, // ln(10)
    { 1000, 0, 0, 0, 0, 0, 0, 0 },  // 0.1 atan limit

    { 981, 7477, 424, 0, 0, 0, 0, 0 }, // pi/32 part A
    { 6810, 3870, 1957, 6057, 2748, 4465, 1312, (-3)&EXPMASK }, // pi/32 part B NOTE NEG!!
    { 200, 0, 0, 0, 0, 0, 0, 0 },  // 0.02 limit used in ln1p
    { 5000, 0, 0, 0, 0, 0, 0, 0 },  // 0.5
    { 2, 5000, 0, 0, 0, 0, 0, 1 },  // 2.5
    { 10, 0, 0, 0, 0, 0, 0, 1 },  // 10

#if 0
    // Lanczos terms for gamma
    { 3, 7948, 6229, 8882, 1576, 6137, 571, 2 },
    { 6, 1191, 9133, 3435, 268, 9475, 3696, 32770&EXPMASKNEG },
    { 3, 1935, 3139, 9365, 7178, 9732, 6587, 2 },
    { 1, 648, 2204, 9659, 4145, 5971, 637, 32770&EXPMASKNEG },
    { 2215, 7659, 2545, 9700, 1065, 2640, 839, 1 },
    { 277, 3434, 9002, 3102, 315, 6808, 5633, 32769&EXPMASKNEG },
    { 19, 7504, 4798, 8896, 954, 2464, 2454, 1 },
    { 7351, 4584, 5326, 3110, 3427, 1541, 972, 32768&EXPMASKNEG },
    { 125, 201, 6315, 9372, 8926, 576, 1395, 0 },
    { 7710, 2871, 8096, 9904, 7327, 526, 2403, 65535&EXPMASKNEG },
    { 10, 9373, 7115, 9701, 7175, 1506, 3503, 32767&EXPMASKNEG },
    { 11, 2406, 1022, 3182, 8735, 6453, 7307, 65534&EXPMASKNEG },
    { 2, 7709, 5759, 7224, 6395, 7358, 7375, 32766&EXPMASKNEG },
#endif
    
    // sin(x) polynomial
    { 1666, 6666, 6666, 6666, 6666, 6666, 6365, 32768&EXPMASKNEG },
    { 83, 3333, 3333, 3333, 3333, 3303, 1985, 0 },
    { 1, 9841, 2698, 4126, 9840, 7789, 3780, 32768&EXPMASKNEG },
    { 275, 5731, 9223, 9828, 9145, 4611, 7555, 32767&EXPMASKNEG },
    { 2, 5052, 1083, 7671, 5305, 495, 7411, 65535&EXPMASKNEG },
    { 160, 5903, 801, 9414, 1262, 5005, 7297, 32766&EXPMASKNEG },
    { 7637, 5075, 3356, 6847, 5161, 2933, 1632, 65533&EXPMASKNEG },

    // cos(x) polynomial
    { 4999, 9999, 9999, 9999, 9999, 9999, 4857, 32768&EXPMASKNEG },
    { 416, 6666, 6666, 6666, 6666, 6153, 8238, 0 },
    { 13, 8888, 8888, 8888, 8880, 5372, 4713, 32768&EXPMASKNEG },
    { 2480, 1587, 3015, 8220, 374, 6779, 0, 32767&EXPMASKNEG },
    { 27, 5573, 1920, 9146, 3795, 4381, 778, 65535&EXPMASKNEG },
    { 2087, 6734, 8250, 8148, 2157, 8695, 5067, 32766&EXPMASKNEG },
    { 11, 4543, 2933, 7975, 7024, 4861, 9252, 65534&EXPMASKNEG },

    // log(x) polynomial
    { 833, 3333, 3333, 3333, 3333, 3200, 4980, 0 },
    { 125, 0, 0, 0, 1767, 3000, 0, 0 },
    { 22, 3214, 2856, 3634, 1290, 0, 0, 0 },
    { 4, 3404, 1799, 7690, 0, 0, 0, 0 },

    // log(ck) constants, ck = 1+k/64, k = 1..64;
    { 155, 418, 6535, 9652, 5415, 854, 460, 0 },
    { 307, 7165, 8666, 7536, 8837, 1028, 2075, 0 },
    { 458, 953, 6031, 2942, 316, 6679, 2676, 0 },
    { 606, 2462, 1816, 4348, 4258, 606, 1320, 0 },
    { 752, 2342, 1237, 5875, 2569, 8605, 3399, 0 },
    { 896, 1215, 8689, 6871, 3261, 9951, 4693, 0 },
    { 1037, 9679, 3681, 6435, 6482, 6061, 8037, 0 },
    { 1177, 8303, 5656, 3834, 5453, 8794, 1094, 0 },
    { 1315, 7635, 7788, 7192, 7258, 8716, 1286, 0 },
    { 1451, 8200, 9844, 4978, 9728, 1935, 637, 0 },
    { 1586, 503, 176, 6385, 8409, 3371, 1746, 0 },
    { 1718, 5025, 6926, 6592, 2234, 98, 9460, 0 },
    { 1849, 2233, 8494, 119, 9266, 3903, 5926, 0 },
    { 1978, 2574, 3329, 9198, 8036, 2572, 711, 0 },
    { 2105, 6476, 9107, 3496, 3766, 9552, 8127, 0 },
    { 2231, 4355, 1314, 2097, 5576, 6295, 903, 0 },
    { 2355, 6607, 1312, 7669, 907, 7588, 2189, 0 },
    { 2478, 3616, 3904, 5812, 5678, 602, 7657, 0 },
    { 2599, 5752, 4436, 9260, 6697, 2079, 4945, 0 },
    { 2719, 3371, 5483, 6417, 5883, 1669, 4945, 0 },
    { 2837, 6817, 3130, 6445, 9834, 6901, 2223, 0 },
    { 2954, 6421, 2893, 8358, 7638, 6681, 9060, 0 },
    { 3070, 2503, 5294, 9118, 6207, 5124, 5405, 0 },
    { 3184, 5373, 1118, 5346, 1581, 247, 2135, 0 },
    { 3297, 5328, 6372, 4679, 8181, 4422, 8119, 0 },
    { 3409, 2658, 6970, 5932, 1030, 5089, 1997, 0 },
    { 3519, 7642, 3157, 1781, 8465, 5447, 4562, 0 },
    { 3629, 549, 3689, 3684, 5313, 7824, 3459, 0 },
    { 3737, 1640, 9793, 5840, 8082, 1016, 8327, 0 },
    { 3844, 1169, 8910, 3320, 3973, 4790, 624, 0 },
    { 3949, 9380, 8240, 8689, 7810, 6394, 363, 0 },
    { 4054, 6510, 8108, 1643, 8197, 8013, 1154, 0 },
    { 4158, 2789, 5143, 7109, 6561, 3328, 8929, 0 },
    { 4260, 8439, 5310, 9000, 6312, 4544, 8795, 0 },
    { 4362, 3676, 6774, 9180, 7034, 9041, 3230, 0 },
    { 4462, 8710, 2628, 4195, 1153, 2590, 1806, 0 },
    { 4562, 3743, 3481, 5875, 9438, 805, 5381, 0 },
    { 4660, 8972, 9924, 5992, 2455, 8619, 2475, 0 },
    { 4758, 4590, 4869, 9639, 1426, 5209, 5863, 0 },
    { 4855, 781, 5781, 7008, 780, 1791, 771, 0 },
    { 4950, 7726, 6797, 8515, 1459, 7964, 5848, 0 },
    { 5045, 5601, 752, 3952, 8705, 8308, 5317, 0 },
    { 5139, 4575, 1102, 2343, 1680, 1006, 882, 0 },
    { 5232, 4814, 3764, 5478, 3651, 6807, 2249, 0 },
    { 5324, 6479, 8869, 4718, 4387, 3923, 7234, 0 },
    { 5415, 9728, 2432, 7443, 7157, 6542, 3039, 0 },
    { 5506, 4711, 7952, 6622, 7925, 9948, 1792, 0 },
    { 5596, 1578, 7935, 4226, 8627, 888, 5005, 0 },
    { 5685, 473, 5352, 6687, 1207, 8738, 7648, 0 },
    { 5773, 1536, 5034, 8236, 431, 8112, 615, 0 },
    { 5860, 4904, 5003, 5782, 890, 4119, 4362, 0 },
    { 5947, 710, 7746, 6927, 8951, 4343, 5465, 0 },
    { 6032, 9085, 1438, 842, 6234, 585, 1866, 0 },
    { 6118, 154, 1105, 9929, 352, 9889, 7664, 0 },
    { 6202, 4040, 9751, 8575, 2885, 1494, 6325, 0 },
    { 6286, 865, 9422, 3741, 3774, 4308, 2057, 0 },
    { 6369, 746, 2237, 692, 3162, 494, 4271, 0 },
    { 6451, 3796, 1373, 5847, 166, 5228, 4961, 0 },
    { 6533, 127, 2012, 7456, 3875, 8615, 8812, 0 },
    { 6613, 9848, 2245, 3650, 826, 235, 8387, 0 },
    { 6694, 3065, 3942, 6292, 6729, 8885, 2709, 0 },
    { 6773, 9882, 3591, 8061, 4080, 9682, 6099, 0 },
    { 6853, 400, 3098, 9194, 1654, 4048, 789, 0 },
    { 6931, 4718, 559, 9453, 941, 7232, 1214, 0 },

};

#define BCD2_CONST_LANCZOS 0

static const Dig2 constTable2[] = 
{
    // double size Lanczos terms for gamma

    //3.7948,6229,8882,1576,6137,0571,8385,6087,7748,2487,6086,1116,8266,2325,9950,6316,10
    { 3, 7948, 6229, 8882, 1576, 6137, 571,
      8385,6087,7748,2487,6086,1116,8266,2325,
      2 },

    //-6.1191,9133,3435,0268,9475,3695,8619,2303,8586,6169,7188,5413,0739,6546,3038,1464,6
    { 6, 1191, 9133, 3435, 268, 9475, 3695,
      8619,2303,8586,6169,7188,5413,739,6546,
      32770 },
    //3.1935,3139,9365,7178,9732,6587,2516,1815,7710,7349,5270,6969,8696,7561,8425,94
    { 3, 1935, 3139, 9365, 7178, 9732, 6587,
      2516,1815,7710,7349,5270,6969,8696,7562,
      2 },

    //-1.0648,2204,9659,4145,5971,0636,6767,3045,5636,6427,7798,2642,4908,6556,1951,4320,785
    { 1, 648, 2204, 9659, 4145, 5971, 636,
      6767,3045,5636,6427,7798,2642,4908,6556,
      32770},

    // 2.215,7659,2545,9700,1065,2640,0839,0578,0196,6530,8981,3632,5470,7872,885
    { 2215, 7659, 2545, 9700, 1065, 2640, 839,
      578,196,6530,8981,3632,5470,7872,885,
      1 },

    //-2.77,3434,9002,3102,0315,6808,5633,2632,6001,3314,1546,7408,3072,4481,5716,8215,697
    { 277, 3434, 9002, 3102, 315, 6808, 5633,
      2632,6001,3314,1546,7408,3072,4481,5717,
      32769 },

    //1.9,7504,4798,8896,0954,2464,2454,3527,7206,3456,0350,8210,3826,3387,9892,0783,963
    { 19, 7504, 4798, 8896, 954, 2464, 2454,
      3527,7206,3456,350,8210,3826,3387,9892,
      1 },

    // -7.351,4584,5326,3110,3427,1541,0972,9593,4702,3887,1286,0444,6989,7138,7333,92
    { 7351, 4584, 5326, 3110, 3427, 1541, 972,
      9593,4702,3887,1286,444,6989,7138,7334,
      32768 },

    // 1.25,0201,6315,9372,8926,0576,1395,0748,8131,6554,2830,4183,3646,7890,817
    { 125, 201, 6315, 9372, 8926, 576, 1395,
      748,8131,6554,2830,4183,3646,7890,817,
      0 },

    // -7.710,2871,8096,9904,7327,0526,2403,7596,8997,0952,8489,4696,4202,3767,5221,521
    { 7710, 2871, 8096, 9904, 7327, 526, 2403,
      7596,8997,952,8489,4696,4202,3767,5222,
      40959 },

    //1.0,9373,7115,9701,7175,1506,3503,3523,6971,8098,6730,4848,3630,9464,6226,8883,4744,5305
    { 10, 9373, 7115, 9701, 7175, 1506, 3503,
      3523,6971,8098,6730,4848,3630,9464,6227,
      8191 },

    //-1.1,2406,1022,3182,8735,6453,7307,4629,1188,2930,0182,3911,2707,2798,2287,6267,78
    { 11, 2406, 1022, 3182, 8735, 6453, 7307,
      4629,1188,2930,182,3911,2707,2798,2288,
      40958 },

    //-2.77,0957,5972,2463,9573,5873,7503,6522,6385,2612,8760,1919,8791,4529,9276,1905,83

    { 277,957,5972,2463,9573,5873,7503,
      6522,6385,2612,8760,1919,8791,4529,9276,
      40956 },
};

BCD pi()
{
    return *(const BCDFloat*)(constTable + BCD_CONST_PI);
}

// 2pi
BCD pi2()
{
    return *(const BCDFloat*)(constTable + BCD_CONST_PI2);
}

static void horner(const BCDFloat* coef, BCD& x, int n, BCD& res)
{
    int i;
    const BCDFloat* cp = coef + n;
    res = 0;
    for (i = 0; i < n; ++i)
    {
        res = res * x;
        res += BCD(*--cp);
    }
}

static void sinPoly(const BCD& a, BCD& sa)
{
    /* calculate sin(a) for |a| <= pi/32 
     *
     * p(y) = 
     -0.166666666666666666666666636459634636681163368103
     + 0.00833333333333333333330319847154774103100919421 *y
     - 0.000198412698412698407789377962065944968466530592 * y**2
     + 2.755731922398289145461175553432349911142734e-6 * y**3
     - 2.5052108376715305049574108322138191235704e-8 *y**4
     + 1.60590308019414126250057297007175565961045303085e-10 *y**5
     - 7.63750753356684751612933163194608455774099589e-13 *y**6

     * then sin(x) ~ x(1+x^2*p(x^2)). relative error <= 1.25*10^-28.
     *
     * NOTES:
     * MiniMaxApproximation[
     If[y > 0, (Sin[Sqrt[y]] - Sqrt[y])/y^(3/2), -1/6], {y, {0, Pi/32}, 6, 0}, 
     WorkingPrecision -> 30][[2, 1]]
     *
     */

    BCD y = a*a;
    BCD py;
    horner((const BCDFloat*)(constTable + BCD_CONST_SINPOLY), y, 7, py);
    sa = a*(1+y*py);
}

static void cosPoly(const BCD& a, BCD& ca)
{
    /*
    * calculate sin(a) for |a| <= pi/32 
    *
    * p(y) =
    -0.499999999999999999999999485707905851100423176682
    + 0.041666666666666666666153823812611360817535526463*y
    -0.001388888888888888805372471283461577523825976975*Power(y,2)
    + 0.000024801587301582200374677934077121988251491037*Power(y,3)
    - 2.75573192091463795438107775847721847637672e-7*Power(y,4)
    + 2.087673482508148215786955066969762215737996819883e-9*Power(y,5)
    - 1.1454329337975702448619252226154423156415441444e-11*Power(y,6)
    *
    * then cos(x) + 1+x^2*p(x^2). relative error 3*10^-26.
    *
    * NOTES:
     MiniMaxApproximation[
     If[y > 0, (Cos[Sqrt[y]] - 1)/y, -1/2], {y, {0, Pi/32}, 6, 0}, 
     WorkingPrecision -> 30][[2, 1]]
     *
     */

    BCD y = a*a;
    BCD py;
    horner((const BCDFloat*)(constTable + BCD_CONST_COSPOLY), y, 7, py);
    ca = 1+y*py;
}

void sincos(const BCD& v, BCD* sinv, BCD* cosv)
{
    /* calculate sin(v), cos(v) or both as requested.
     */
    BCD res;
    BCD a;
    int k;
    int neg = v.isNeg();

    /* arrange a >= 0 */
    a = v;
    a._v.clearSign();

    /* reduce argument to 0 <= a < 2pi using special means,
     * taking care of large arguments (eg sin(1e22)). its possible
     * that the answer cannot be calculated accurately, in which case
     * modtwopi bails out with NAN.
     */
    a = modtwopi(a);
    if (a.isSpecial()) 
    {
        if (sinv) *sinv = a;
        if (cosv) *cosv = a;
        return;
    }

    /*
     * reduce to k*pi/32 + a, where a < pi/32. use a lookup table
     * for sin(k*pi/32) and cos(k*pi/32). require 8 entries for each.
     */
    BCD piby32(*(const BCDFloat*)(constTable + BCD_CONST_PIBY32));
    k = ifloor(a/piby32);
    if (k > 0) 
    {
        // subtract in two parts for accuracy.
        BCD piby32a(*(const BCDFloat*)(constTable + BCD_CONST_PIBY32A));
        BCD piby32b(*(const BCDFloat*)(constTable + BCD_CONST_PIBY32B));
        a = (a - k*piby32a) - k*piby32b;
    }

    /* now a <= pi/32, we use the polynomial approximations */
    BCD sa;
    BCD ca;

    BCD sina, cosa;
    BCD* sap = 0;
    BCD* cap = 0;

    k &= 64-1;  // wrap around at 2pi
    int q = k/16; // q is the quadrant.

    k &= 15; // index into table.

    if (k) // sap & cap = 0 for k = 0.
    {
        /* require both sin and cos */
        sinPoly(a, sa);
        cosPoly(a, ca);

        if (k < 8) 
        {
            BCD sk(*(const BCDFloat*)(constTable + BCD_CONST_SINTAB-1+k));
            BCD ck(*(const BCDFloat*)(constTable + BCD_CONST_COSTAB-1+k));
            sina = sk*ca + ck*sa;
            cosa = ck*ca - sk*sa;
        }
        else 
        {
            BCD sk(*(const BCDFloat*)(constTable + BCD_CONST_SINTAB+15-k));
            BCD ck(*(const BCDFloat*)(constTable + BCD_CONST_COSTAB+15-k));
            sina = ck*ca + sk*sa;
            cosa = sk*ca - ck*sa;
        }
        sap = &sina;
        cap = &cosa;
    }

    if (sinv)
    {
        if ((q&1) == 0)
        {
            if (!sap) { sinPoly(a, sina); sap = &sina; }
            *sinv = sina;
        }
        else
        {
            if (!cap) { cosPoly(a, cosa); cap = &cosa; }
            *sinv = cosa;
        }
        if (neg ^ (q > 1)) *sinv = -*sinv;
    }

    if (cosv)
    {
        if ((q&1) == 0)
        {
            if (!cap) { cosPoly(a, cosa); }
            *cosv = cosa;
        }
        else
        {
            if (!sap) { sinPoly(a, sina); }
            *cosv = sina;
        }

        if (q == 1 || q == 2) *cosv = -*cosv;
    }
}

void sinhcosh(const BCD& a, BCD* sinha, BCD* cosha)
{
    BCD v = exp(a);
    if (sinha) *sinha = (v - 1/v)/2;  // XX unstable
    if (cosha) *cosha = (v + 1/v)/2;
}

BCD sin(const BCD& v)
{
    BCD sinv;
    if (v.isSpecial()) return v;

    sincos(v, &sinv, 0);
    return sinv;
}

BCD cos(const BCD& v)
{
    BCD cosv;
    if (v.isSpecial()) return v;

    sincos(v, 0, &cosv);
    return cosv;
}

BCD tan(const BCD& v)
{
    BCD s, c;
    if (v.isSpecial()) return v;

    sincos(v, &s, &c);
    return s/c;
}

static BCD expTaylor(const BCD& a, int n)
{
    BCD t = a;
    BCD s = t + 1;
    int i = 1;
    while (i < n) 
    {
        t = t*a/(++i);
        s += t;
    }
    return s;
}


BCD pow(const BCD& a, int4 n)
{
    int4 m;
    if (n == 0) return 1;
    m = (n < 0) ? -n : n;

    BCD s;
    if (m > 1) 
    {
        BCD r = a;
        s = 1;
        /* Use binary exponentiation */
        for (;;) 
        {
            if (m & 1) s *= r;
            m >>= 1;
            if (!m) break;
            r *= r;
        }
    } else { s = a; }

    /* Compute the reciprocal if n is negative. */
    if (n < 0) 
        return 1/s;

    return s;
}

BCD exp(const BCD& v)
{
    /* write v = k*r + n*ln(2)
     * where |k*r| < ln(2)
     *
     * let k be some power of 2, eg 64 so that
     *
     * exp(v) = exp(kr + nln(2)) = exp(kr)*2^n
     *        = exp(r)^64 * 2^n
     *
     * then r is small enough for taylor series.
     */
    if (v.isSpecial()) 
    {
        if (v.isInf() && v.isNeg()) return 0;  // exp(-inf) = 0
        return v;
    }
    
    if (v.isZero()) return 1;

    bool neg = v.isNeg();

    BCD ln2(*(const BCDFloat*)(constTable + BCD_CONST_LN2));
    BCD n = trunc(v/ln2);

    if (n > 33218)
    {
        /* overflow */
        return BCDFloat::posInf();
    }
    else if (n < -33218)
    {
        /* underflow */
        return 0;
    }

    int k = 64;
    BCD r = (v - n*ln2)/k;
    
    /* error bounded by x^10/10! where x = ln(2)/k */
    BCD er = expTaylor(r, 9);
    return pow(er, k)*pow(BCD(2), n);
}

static BCD _ln1p(const BCD& a)
{
    /* otherwise use a series that converges for small arguments */
    BCD s;
    BCD t;
    BCD s1;
    BCD x;
    BCD c;
    
    c = 2;
    s = a;
    x = -a;
    t = x;

    for (;;) 
    {
        t *= x;
        s1 = s - t/c;
        if (s1 == s) break;
        c += 1;
        s = s1;
    }
    return s1;
}

static BCD logPoly(const BCD& r)
{
    /*
      MiniMaxApproximation[
      If[r > 0, Log[(1 + Sqrt[r]/2)/(1 - Sqrt[r]/2)]/Sqrt[r], 
      1], {r, {0, 1/128}, 3, 0}, WorkingPrecision -> 30]

      produces p(r), so ln((1+r/2)/(1-r/2)) ~ p(r^2)*r
      
      BUT, it's not very good, seeing about |e| ~ 1.25e-14
    */

    /* p(r) =
       r + 0.08333333333333335805 r^3 + 0.0124999999978878 r^5 + 
       0.002232197165 r^7
       max error <= 0.27e-24.
       Elementary Algorithms. Jean-Michel Muller. p57.
    */

    /*
      p(r) = 
    r + 0.0833333333333333333332004980000 r^3 + 
        0.0125000000000000176730000000000 r^5 + 
        0.00223214285636341290000000000000 r^7 + 
        0.000434041799769000000000000000000 r^9

        max error <= 0.80e-30.
        Elementary Algorithms. Jean-Michel Muller. p57.
        relative error max of 3e-28.
    */


    BCD r2 = r*r;
    BCD pr;
    horner((const BCDFloat*)(constTable + BCD_CONST_LOGPOLY), r2, 4, pr);
    return (pr*r2 + 1)*r;
}

BCD log(const BCD& v)
{
    /* natural logarithm */
    if (v.isNeg()) return BCDFloat::nan();
    if (v.isSpecial()) return v;
    if (v.isZero()) return BCDFloat::negInf();
    
    BCD lna;
    BCD2 a2;
    bool negAnswer = false;

    if (v < 1)
    {
        // v in (0,1) map to (1,inf) and negate final answer
        a2 = BCD2(1)/v;
        negAnswer = true;
    }
    else 
        a2 = v;

    // have a in [1,inf)

    // extract the power of 10 from the exponent
    int p10 = (a2.exponent()-1)*4;
    a2.setExponent(1);

    // divide out the biggest digit to leave a number < 10
    unsigned int d = a2.digit(0);
    int dnorm = 1;
    while (d >= 10)
    {
        ++p10; // adjust the 10's count
        dnorm *= 10;
        d /= 10;
    }
        
    // divide by 2 until < 2, keep a count of the 2's
    int p2 = 0;
    while (d > 1)
    {
        ++p2; // adjust the 2s count
        dnorm <<= 1;
        d >>= 1;
    }

    // finally divide the original number by the dnorm to take out
    // the 10s and 2s, leaving a value not bigger than 2.
    if (dnorm > 1)
        a2 /= dnorm;

    // we have a in (1+delta,2]
    // ln(v) = ln(a) + p10*ln10 + p2*ln2;

    // find ck where ck = 1+k/64, k=1,2,..64 with |x-ck| <= 1/128
    BCD2 a21 = a2 - 1;
    BCD half(*(const BCDFloat*)(constTable + BCD_CONST_HALF));
    int4 k = ifloor(a21.asBCD()*64 + half); // round to nearest k
    
    // set r = 2*(a - ck)/(a + ck)
    BCD2 dk = BCD(k)/64;
    BCD r = ((a21 - dk)/(a2 + 1 + dk)).asBCD();

    // now, ln(a/ck) = ln((1+r/2)/(1-r/2)) = logPoly(r);
    lna = logPoly(r*2);

    // ln(a) = ln(ck) + ln(a/ck);
    if (k)
    {
        BCD lnck(*(const BCDFloat*)(constTable + BCD_CONST_LOGCK + k - 1));
        lna += lnck;
    }
    
    BCD ln10(*(const BCDFloat*)(constTable + BCD_CONST_LN10));
    BCD ln2(*(const BCDFloat*)(constTable + BCD_CONST_LN2));
    lna += p10*ln10 + p2*ln2;

    if (negAnswer) lna.negate();
    return lna;
}

BCD atan(const BCD& v)
{
    bool neg = v.isNeg();

    if (v.isSpecial()) 
    {
        if (v.isInf()) 
        {
            BCD piby2(*(const BCDFloat*)(constTable + BCD_CONST_PIBY2));
            if (neg) return (piby2*3)/2;
            return piby2;
        }
        return v;
    }

    /* arrange for a >= 0 */
    BCD a;
    if (neg) 
        a = -v;
    else a = v;

    /* reduce range to 0 <= a < 1, using atan(x) = pi/2 - atan(1/x)
     */
    bool invert = (a > 1);
    if (invert)
        a = 1/a;
    
    /* reduce to small enough limit to use taylor series.
     * using
     *  tan(x/2) = tan(x)/(1+sqrt(1+tan(x)^2))
     */
    BCD atanlim(*(const BCDFloat*)(constTable + BCD_CONST_ATANLIM));
    int doubles = 0;
    while (a > atanlim) {
        ++doubles;
        a = a/(1+sqrt(1+a*a));  // at most 3 iterations.
    }

    /* now use taylor series
     * tan(x) = x(1-x^2/3+x^4/5-x^6/7...)
     */
    
    BCD a2 = a*a;
    BCD t = a2;
    BCD s = 1 - t/3;
    int i;
    int j = 5;
    /* perform 9 more terms, error will be the first term not used.
     * ie x^21/21.
     */
    for (i = 2; i < 11; ++i) {
        t *= a2;
        if ((i & 1) == 0) s += t/j;
        else s -= t/j;
        j += 2;
    }
    s = s*a;

    while (doubles) {
        s = s + s;
        --doubles;
    }

    if (invert) {
        BCD piby2(*(const BCDFloat*)(constTable + BCD_CONST_PIBY2));
        s = piby2 - s;
    }

    if (neg) {
        s = -s;
    }
    return s;
}

BCD atan2(const BCD& y, const BCD& x)
{
    BCD r;
    if (x.isZero()) {
        r = BCD(*(const BCDFloat*)(constTable + BCD_CONST_PIBY2));
        if (y.isNeg()) { r = -r; }
    }
    else {
        r = atan(y/x);
        if (x.isNeg()) {
            BCD pi(*(const BCDFloat*)(constTable + BCD_CONST_PI));
            if (y.isNeg()) {
                r = r - pi;
            }
            else {
                r = r + pi;
            }
        }
    }
    return r;
}

BCD asin(const BCD& v)
{
    if (v.isSpecial()) return v;

    if (v < -1 || v > 1) {
        return BCDFloat::nan();
    }
    
    BCD r = atan(v/(1+sqrt(1-v*v)));
    r = r + r;
    return r;
}

BCD acos(const BCD& v)
{
    if (v.isSpecial()) return v;

    if (v < -1 || v > 1) {
        return BCDFloat::nan();
    }
    
    if (v == 1) return 0;
    
    BCD r = atan((1-v)/sqrt(1-v*v));
    r = r + r;
    return r;
}

BCD pow(const BCD& x, const BCD& y)
{
    if (y.isZero()) 
    {
        if (x.isZero()) return BCDFloat::nan();
        return 1;
    }
    
    /* check for x^n */
    if (y.isInteger()) 
    {
        int4 n = ifloor(y);
        if (n) return pow(x, n);
        
        /* otherwise power is too large */
        if (x == 1) return x;
        
        bool a = (fabs(x) > 1);
        if (a == y.isNeg()) return 0;
        return BCDFloat::posInf();
    }

    /* otherwise use logs */
    return exp(log(x)*y);
} 

/* here are the first 1060 decimal digits of 1/2pi */
static const unsigned short inv2pi[] = {
    1591, 5494, 3091, 8953, 3576, 8883,
    7633, 7251, 4362,  344, 5964, 5740, 4564, 4874, 7667, 3440, 5889,
    6797, 6342, 2653, 5090, 1138,  276, 6253,  859, 5607, 2842, 7267,
    5795, 8036, 8929, 1184, 6114, 5786, 5287, 7967, 4107, 3169, 9839,
    2292, 3996, 6937, 4090, 7757, 3077, 7463, 9692, 5307, 6887, 1739,
    2896, 2173, 9766, 1693, 3623, 9024, 1723, 6290, 1183, 2380, 1142,
    2269, 9755, 7159, 4046, 1890,  869,  267, 3956, 1204, 8941,  936,
    9378, 4408, 5528, 7230, 9994, 6443, 4002, 4867, 2347, 7394, 5961,
     898, 3230, 9678, 3074, 9061, 6698, 6462, 8046, 9944, 8652, 1878,
    8157, 4786, 5669, 6424, 1038, 9958, 7413, 9348, 6099, 8386, 8099,
    1999, 6244, 2875, 5851, 7117, 8858, 4311, 1751, 8767, 1605, 4654,
    7536, 9880,  973, 9460, 3647, 5933, 3768,  593,  249, 4496, 6353,
     532, 7156, 7755,  322,  324, 7778, 1639, 7166,  229, 4674, 8119,
    5981, 6584,  606,  168,  303, 5998, 1339, 1198, 7498, 8327, 8665,
    4435, 2797, 5507,   16, 2406, 7756, 4388, 8495, 7131,  880, 1221,
    9937, 6147, 6813, 7776, 4737, 8906, 3306, 8046, 4579, 7848, 1761,
    3124, 2731, 4069, 9607, 7502, 4500, 2977, 5985, 7089,  569,  279,
    6785, 1315, 2521,   16, 3177, 4602,  924, 8116,  624,  561, 4562,
     314, 6484,  892, 4845, 9191, 4352, 1157, 5407, 5562,   87, 1526,
    6068,  221, 7159, 1407, 5747, 4582, 7225, 9774, 6285, 3998, 7515,
    5329, 3908, 1398, 1772, 4093, 5825, 4797,  733, 2871, 9040, 6999,
    7590, 7657, 7078, 4934, 7039, 3589, 8280, 8717, 3425, 6403, 6689,
    5116, 6254, 5705, 9433, 2763, 1268, 6500, 2612, 2717, 9711, 5321,
    1259, 9504, 3866, 7945,  376, 2556,  836, 3171, 1695, 2597, 5812,
    8224, 9416, 2333, 4314, 5106, 1235
};

/* double precision version of 2pi */
static const unsigned short pi2dp[] = {
 6, 2831, 8530, 7179, 5864, 7692, 5286,
 7665, 5900, 5768, 3943, 3879, 8750, 2116, 4194
};

BCD modtwopi(const BCD& a)
{
    BCD pi2(*(const BCDFloat*)(constTable + BCD_CONST_PI2));
    if (a < pi2) return a;

    unsigned short xd[2*P+1];
    int i;

    /* copy digits of manstissa as double precision */
    for (i = 0; i < P; ++i) {
        xd[i] = a.digit(i);
    }
    while (i <= 2*P) { // clear extended digits.
        xd[i] = 0;
        ++i;
    }
    int ex = a.exponent();

    unsigned short bd[2*P+1];
    int eb;
    int excess = 0;

    /* see if the exponent is large enough to consider it separately */
    if (ex > P) {

        /* yes. consider our number as X * BASE^E where X is an integer.
         * in this case we can calculate f = (BASE^E) mod 2pi accurately
         * and form X*f.
         */

        excess = ex - P;  // remaining exponent
        ex = P;

        unsigned short fd[2*P+1];
        int ef;

        /* do we have enough table? */
        if (excess + 2*P+1 > (int)(sizeof(inv2pi)/sizeof(inv2pi[0]))) {
            /* oh dear, digits required off end of table. give up.
             */
            return BCDFloat::nan();
        }

        /* find base^ex mod 2pi using the table */
        BCDFloat::mul2(inv2pi + excess, 0, pi2dp, 1, fd, ef);

        /* multiply exponent excess mod 2pi into X. note, all 
         * calculations here must be double precision. this is because
         * we are only interested in the fractional part and the
         * integer part is the size of 1 precision already.
         */
        BCDFloat::mul2(fd, ef, xd, ex, bd, eb);

        for (i = 0; i < 2*P+1; ++i) xd[i] = bd[i];

        ex = eb;
    }


    /* need to divide by 2pi and extract the fractional part.
     * this is done by multiplying by 1/2pi using our table digits
     * we had to have for the exponent extraction.
     */
    BCDFloat::mul2(xd, ex, inv2pi, 0, bd, eb);

    BCD b;
    /* pick out final single precision fraction */
    for (i = 0; i < P; ++i) b.digit(i, bd[i+eb]);
    b.digit(P, 0); // no exponent.

    return b * pi2;
}

BCD log10(const BCD& v)
{
    BCD ln10(*(const BCDFloat*)(constTable + BCD_CONST_LN10));
    return log(v) / ln10;
}

BCD hypot(const BCD& a, const BCD& b)
{
    // return sqrt(a^2 + b^2)
    // use numerically stable method, Numerical Recipies 3rd p226

    BCD fa, fb, t;

    fa = fabs(a);
    fb = fabs(b);

    if (a.isZero()) return fb;
    if (b.isZero()) return fa;

    if (fa >= fb)
    {
        t = b/a;
        return fa*sqrt(t*t+1);
    }
    else
    {
        t = a/b;
        return fb*sqrt(t*t+1);
    }
}

BCD fmod(const BCD& a, const BCD& b)
{
    BCD c = a - b * trunc(a / b);
    if (a == trunc(a) && b == trunc(b) && !(c == trunc(c))) {
	// Numerator and denominator are both integral;
	// in this case we force the result to be integral as well.
	BCD half(*(const BCDFloat*)(constTable + BCD_CONST_HALF));
	if (c < 0)
	    c = trunc(c - half);
	else
	    c = trunc(c + half);
    }
    return c;
}


BCD ln1p(const BCD& a)
{
    /* calculate log(a+1) without numerical instability near
     * a == 0.
     */

    /* first test special cases. */
    if (a.isSpecial()) return a;
    if (a.isZero()) return 0; // ln(1) == 0
    if (a == -1) return BCDFloat::negInf(); // ln(0) = -inf
    //if (a < -1) return BCDFloat::nan(); // ln(-x) = nan

    /* if x > 0.01, then the usual log calculation will give correct
     * results.
     */
    BCD pointzerotwo(*(const BCDFloat*)(constTable + BCD_CONST_HUNDREDTH));
    if (fabs(a) > pointzerotwo) return log(1+a);

    /* otherwise use a series that converges for small arguments */
    return _ln1p(a);
}


#define K 12
#define GG 12

static void _gammaFactorialAux(const BCD& z,
                               BCD& t1, BCD& t2, BCD& s)
{
    /* calculate gamma(z+1) = z! for z > 0
     * using lanczos expansion with precomputed coefficients.
     *
     * NOTE: accuracy degrades as z increases.
     *
     * calculate t1, t2 and s where:
     * gamma(z + 1) = exp(t1*log(t2)-t2)*s;
     */

    BCD2 t, s2;
    int i;

    const BCDFloat2* lancz = (const BCDFloat2*)(constTable2 + BCD2_CONST_LANCZOS);
    s2 = lancz[0];

    t = 1;
    for (i = 1; i <= K; ++i)
    {
        t *= (z+(1-i))/(z + i);
        s2 += t*lancz[i];
    }

    s = s2.asBCD();
    s *= 2;

    BCD half(*(const BCDFloat*)(constTable + BCD_CONST_HALF));
    t1 = z + half;
    t2 = t1 + GG;
}


static BCD _gammaFactorial(const BCD& z)
{
    BCD t1, t2, s;
    _gammaFactorialAux(z, t1, t2, s);
    return exp(t1*log(t2)-t2)*s;
}

BCD gammaFactorial(const BCD& c)
{
    /* return c! (c factorial) and also the gamma function
     * where c! = gamma(c+1).
     */

    /* deal with special cases */
    if (c.isSpecial()) return c;

    if (c >= 3249) return BCDFloat::posInf();

    if (c.isInteger()) 
    {
        if (c.isZero()) return 1;
        if (c.isNeg()) return BCDFloat::nan();
        int v = ifloor(c);
        
        if (!v)
            /* too large for integer. answer must be infinite */
            return BCDFloat::posInf();

        /* calculate integer factorial */
        BCD f = c;
        BCD x = c;
        while (v > 1) 
        {
            --x;
            f *= x;
            --v;
        }
        return f;
    }

    if (c.isNeg()) 
    {
        /* use reflection formula */
        BCD z1 = -c;
        BCD pi(*(const BCDFloat*)(constTable + BCD_CONST_PI));
        BCD z2 = z1*pi;
        return z2/sin(z2)/_gammaFactorial(z1);
    }
    else 
    {
        return _gammaFactorial(c);
    }
}

BCD expm1(const BCD& a)
{
    /* first test special cases. */
    if (a.isSpecial() || a.isZero()) return a;  // exp(0)-1 == 0

    /* if |x| > 0.01, then the usual calculation will give correct
     * results.
     */
    BCD pointzerotwo(*(const BCDFloat*)(constTable + BCD_CONST_HUNDREDTH));
    if (fabs(a) >= pointzerotwo)
        return exp(a)-1;

    BCD t = 1;
    BCD d = 2;
    BCD s = 1;
    BCD s1;

    for (;;) 
    {
        t = t*a/d;
        s1 = s + t;
        if (s1 == s) break;
        s = s1;
        ++d;
    }
    return s*a;
}

BCD sinh(const BCD& a)
{
    BCD v = exp(a);
    return (v - 1/v)/2; // XX unstable
}

BCD cosh(const BCD& a)
{
    BCD v = exp(a);
    return (v + 1/v)/2;
}


BCD tanh(const BCD& a)
{
    BCD v = exp(a);
    v = v*v;
    return (v - 1)/(v + 1);
}

BCD ceil(const BCD& a)
{
    if (a.isInteger()) return a;
    return floor(a+1);
}
