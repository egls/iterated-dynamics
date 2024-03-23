/*
FRACTALS.C, FRACTALP.C and CALCFRAC.C actually calculate the fractal
images (well, SOMEBODY had to do it!).  The modules are set up so that
all logic that is independent of any fractal-specific code is in
CALCFRAC.C, the code that IS fractal-specific is in FRACTALS.C, and the
structure that ties (we hope!) everything together is in FRACTALP.C.
Original author Tim Wegner, but just about ALL the authors have
contributed SOME code to this routine at one time or another, or
contributed to one of the many massive restructurings.

The Fractal-specific routines are divided into three categories:

1. Routines that are called once-per-orbit to calculate the orbit
   value. These have names like "XxxxFractal", and their function
   pointers are stored in fractalspecific[fractype].orbitcalc. EVERY
   new fractal type needs one of these. Return 0 to continue iterations,
   1 if we're done. Results for integer fractals are left in 'lnew.x' and
   'lnew.y', for floating point fractals in 'new.x' and 'new.y'.

2. Routines that are called once per pixel to set various variables
   prior to the orbit calculation. These have names like xxx_per_pixel
   and are fairly generic - chances are one is right for your new type.
   They are stored in fractalspecific[fractype].per_pixel.

3. Routines that are called once per screen to set various variables.
   These have names like XxxxSetup, and are stored in
   fractalspecific[fractype].per_image.

4. The main fractal routine. Usually this will be standard_fractal(),
   but if you have written a stand-alone fractal routine independent
   of the standard_fractal mechanisms, your routine name goes here,
   stored in fractalspecific[fractype].calctype.per_image.

Adding a new fractal type should be simply a matter of adding an item
to the 'fractalspecific' structure, writing (or re-using one of the existing)
an appropriate setup, per_image, per_pixel, and orbit routines.

--------------------------------------------------------------------   */
#include "port.h"
#include "prototyp.h"

#include "fractals.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "fpu087.h"
#include "fracsubr.h"
#include "fractype.h"
#include "hcmplx.h"
#include "id_data.h"
#include "mpmath_c.h"
#include "parser.h"
#include "pixel_grid.h"
#ifdef XFRACT
#include "realdos.h"
#endif

#include <cfloat>
#include <cmath>
#include <cstdlib>

LComplex g_l_coefficient;
LComplex g_l_old_z;
LComplex g_l_new_z;
LComplex g_l_param;
LComplex g_l_init;
LComplex g_l_temp;
static LComplex ltmp2;
LComplex g_l_param2;
long g_l_temp_sqr_x;
long g_l_temp_sqr_y;
int g_max_color;
int g_degree;
int g_basin;
double g_newton_r_over_d;
double g_degree_minus_1_over_degree;
double g_threshold;
static DComplex tmp2;
std::vector<DComplex> g_roots;
std::vector<MPC> g_mpc_roots;
long g_fudge_half;
DComplex g_power_z;
int     g_bit_shift_less_1;                  // bit shift less 1
bool g_overflow = false;

#define modulus(z)       (sqr((z).x)+sqr((z).y))
#define conjugate(pz)   ((pz)->y = 0.0 - (pz)->y)
#define distance(z1, z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))
#define pMPsqr(z) (*pMPmul((z), (z)))
#define MPdistance(z1, z2)  (*pMPadd(pMPsqr(*pMPsub((z1).x, (z2).x)), pMPsqr(*pMPsub((z1).y, (z2).y))))

int g_c_exponent;


// These are local but I don't want to pass them as parameters
DComplex g_param_z1;
DComplex g_param_z2;
DComplex *g_float_param;
LComplex *g_long_param; // used here and in jb.c

// --------------------------------------------------------------------
//              These variables are external for speed's sake only
// --------------------------------------------------------------------

double g_sin_x;
double g_cos_x;
static double siny;
static double cosy;
static double tmpexp;
double g_temp_sqr_x;
double g_temp_sqr_y;

static double foldxinitx;
static double foldyinity;
static double foldxinity;
static double foldyinitx;
static long oldxinitx;
static long oldyinity;
static long oldxinity;
static long oldyinitx;
static long longtmp;

double g_quaternion_c;
double g_quaternion_ci;
double g_quaternion_cj;
double g_quaternino_ck;

// temporary variables for trig use
static long lcosx;
static long lsinx;
static long lcosy;
static long lsiny;

/*
**  details of finite attractors (required for Magnet Fractals)
**  (can also be used in "coloring in" the lakes of Julia types)
*/

/*
**  pre-calculated values for fractal types Magnet2M & Magnet2J
*/
static DComplex  T_Cm1;        // 3 * (floatparm - 1)
static DComplex  T_Cm2;        // 3 * (floatparm - 2)
static DComplex  T_Cm1Cm2;     // (floatparm - 1) * (floatparm - 2)

void FloatPreCalcMagnet2() // precalculation for Magnet2 (M & J) for speed
{
    T_Cm1.x = g_float_param->x - 1.0;
    T_Cm1.y = g_float_param->y;
    T_Cm2.x = g_float_param->x - 2.0;
    T_Cm2.y = g_float_param->y;
    T_Cm1Cm2.x = (T_Cm1.x * T_Cm2.x) - (T_Cm1.y * T_Cm2.y);
    T_Cm1Cm2.y = (T_Cm1.x * T_Cm2.y) + (T_Cm1.y * T_Cm2.x);
    T_Cm1.x += T_Cm1.x + T_Cm1.x;
    T_Cm1.y += T_Cm1.y + T_Cm1.y;
    T_Cm2.x += T_Cm2.x + T_Cm2.x;
    T_Cm2.y += T_Cm2.y + T_Cm2.y;
}

// --------------------------------------------------------------------
//              Bailout Routines
// --------------------------------------------------------------------

int  fpMODbailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpREALbailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpIMAGbailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpORbailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit || g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpANDbailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit && g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpMANHbailout()
{
    double manhmag;
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    manhmag = std::fabs(g_new_z.x) + std::fabs(g_new_z.y);
    if ((manhmag * manhmag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fpMANRbailout()
{
    double manrmag;
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    manrmag = g_new_z.x + g_new_z.y; // don't need abs() since we square it next
    if ((manrmag * manrmag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))
static MPC mpcold;
static MPC mpcnew;
static MPC mpctmp;
static MPC mpctmp1;

// --------------------------------------------------------------------
//              Fractal (once per iteration) routines
// --------------------------------------------------------------------
static double xt;
static double yt;
static double t2;

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
void cpower(DComplex *base, int exp, DComplex *result)
{
    if (exp < 0)
    {
        cpower(base, -exp, result);
        CMPLXrecip(*result, *result);
        return;
    }

    xt = base->x;
    yt = base->y;

    if (exp & 1)
    {
        result->x = xt;
        result->y = yt;
    }
    else
    {
        result->x = 1.0;
        result->y = 0.0;
    }

    exp >>= 1;
    while (exp)
    {
        t2 = xt * xt - yt * yt;
        yt = 2 * xt * yt;
        xt = t2;

        if (exp & 1)
        {
            t2 = xt * result->x - yt * result->y;
            result->y = result->y * xt + yt * result->x;
            result->x = t2;
        }
        exp >>= 1;
    }
}

// long version
static long lxt;
static long lyt;
static long lt2;

int lcpower(LComplex *base, int exp, LComplex *result, int bitshift)
{
    if (exp < 0)
    {
        g_overflow = lcpower(base, -exp, result, bitshift) != 0;
        LCMPLXrecip(*result, *result);
        return g_overflow ? 1 : 0;
    }

    g_overflow = false;
    lxt = base->x;
    lyt = base->y;

    if (exp & 1)
    {
        result->x = lxt;
        result->y = lyt;
    }
    else
    {
        result->x = 1L << bitshift;
        result->y = 0L;
    }

    exp >>= 1;
    while (exp)
    {
        lt2 = multiply(lxt, lxt, bitshift) - multiply(lyt, lyt, bitshift);
        lyt = multiply(lxt, lyt, g_bit_shift_less_1);
        if (g_overflow)
        {
            return g_overflow;
        }
        lxt = lt2;

        if (exp & 1)
        {
            lt2 = multiply(lxt, result->x, bitshift) - multiply(lyt, result->y, bitshift);
            result->y = multiply(result->y, lxt, bitshift) + multiply(lyt, result->x, bitshift);
            result->x = lt2;
        }
        exp >>= 1;
    }
    if (result->x == 0 && result->y == 0)
    {
        g_overflow = true;
    }
    return g_overflow;
}

int complex_div(DComplex numerator, DComplex denominator, DComplex *pout);
int complex_mult(DComplex arg1, DComplex arg2, DComplex *pz);

// Distance of complex z from unit circle
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))
#define LDIST1(z) (lsqr((((z).x)-g_fudge_factor)) + lsqr(((z).y)))

int NewtonFractal2()
{
    static char start = 1;
    if (start)
    {
        start = 0;
    }
    cpower(&g_old_z, g_degree-1, &g_tmp_z);
    complex_mult(g_tmp_z, g_old_z, &g_new_z);

    if (DIST1(g_new_z) < g_threshold)
    {
        if (g_fractal_type == fractal_type::NEWTBASIN || g_fractal_type == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            /* this code determines which degree-th root of root the
               Newton formula converges to. The roots of a 1 are
               distributed on a circle of radius 1 about the origin. */
            for (int i = 0; i < g_degree; i++)
            {
                /* color in alternating shades with iteration according to
                   which root of 1 it converged to */
                if (distance(g_roots[i], g_old_z) < g_threshold)
                {
                    if (g_basin == 2)
                    {
                        tmpcolor = 1+(i&7)+((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmpcolor = 1+i;
                    }
                    break;
                }
            }
            if (tmpcolor == -1)
            {
                g_color_iter = g_max_color;
            }
            else
            {
                g_color_iter = tmpcolor;
            }
        }
        return 1;
    }
    g_new_z.x = g_degree_minus_1_over_degree * g_new_z.x + g_newton_r_over_d;
    g_new_z.y *= g_degree_minus_1_over_degree;

    // Watch for divide underflow
    t2 = g_tmp_z.x*g_tmp_z.x + g_tmp_z.y*g_tmp_z.y;
    if (t2 < FLT_MIN)
    {
        return 1;
    }
    else
    {
        t2 = 1.0 / t2;
        g_old_z.x = t2 * (g_new_z.x * g_tmp_z.x + g_new_z.y * g_tmp_z.y);
        g_old_z.y = t2 * (g_new_z.y * g_tmp_z.x - g_new_z.x * g_tmp_z.y);
    }
    return 0;
}

int complex_mult(DComplex arg1, DComplex arg2, DComplex *pz)
{
    pz->x = arg1.x*arg2.x - arg1.y*arg2.y;
    pz->y = arg1.x*arg2.y+arg1.y*arg2.x;
    return 0;
}

int complex_div(DComplex numerator, DComplex denominator, DComplex *pout)
{
    double mod = modulus(denominator);
    if (mod < FLT_MIN)
    {
        return 1;
    }
    conjugate(&denominator);
    complex_mult(numerator, denominator, pout);
    pout->x = pout->x/mod;
    pout->y = pout->y/mod;
    return 0;
}

MP g_newton_mp_r_over_d;
MP g_mp_degree_minus_1_over_degree;
MP g_mp_threshold;
MP g_mp_temp2;
MP g_mp_one;
MP g_mp_temp_param2_x;

int MPCNewtonFractal()
{
    g_mp_overflow = 0;
    mpctmp   = MPCpow(mpcold, g_degree-1);

    mpcnew.x = *pMPsub(*pMPmul(mpctmp.x, mpcold.x), *pMPmul(mpctmp.y, mpcold.y));
    mpcnew.y = *pMPadd(*pMPmul(mpctmp.x, mpcold.y), *pMPmul(mpctmp.y, mpcold.x));
    mpctmp1.x = *pMPsub(mpcnew.x, g_mpc_one.x);
    mpctmp1.y = *pMPsub(mpcnew.y, g_mpc_one.y);
    if (pMPcmp(MPCmod(mpctmp1), g_mp_threshold) < 0)
    {
        if (g_fractal_type == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            for (int i = 0; i < g_degree; i++)
                if (pMPcmp(MPdistance(g_mpc_roots[i], mpcold), g_mp_threshold) < 0)
                {
                    if (g_basin == 2)
                    {
                        tmpcolor = 1+(i&7) + ((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmpcolor = 1+i;
                    }
                    break;
                }
            if (tmpcolor == -1)
            {
                g_color_iter = g_max_color;
            }
            else
            {
                g_color_iter = tmpcolor;
            }
        }
        return 1;
    }

    mpcnew.x = *pMPadd(*pMPmul(g_mp_degree_minus_1_over_degree, mpcnew.x), g_newton_mp_r_over_d);
    mpcnew.y = *pMPmul(mpcnew.y, g_mp_degree_minus_1_over_degree);
    g_mp_temp2 = MPCmod(mpctmp);
    g_mp_temp2 = *pMPdiv(g_mp_one, g_mp_temp2);
    mpcold.x = *pMPmul(g_mp_temp2, (*pMPadd(*pMPmul(mpcnew.x, mpctmp.x), *pMPmul(mpcnew.y, mpctmp.y))));
    mpcold.y = *pMPmul(g_mp_temp2, (*pMPsub(*pMPmul(mpcnew.y, mpctmp.x), *pMPmul(mpcnew.x, mpctmp.y))));
    g_new_z.x = *pMP2d(mpcold.x);
    g_new_z.y = *pMP2d(mpcold.y);
    return g_mp_overflow;
}

int Barnsley1Fractal()
{
    /* Barnsley's Mandelbrot type M1 from "Fractals
    Everywhere" by Michael Barnsley, p. 322 */

    // calculate intermediate products
    oldxinitx   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    oldyinity   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    oldxinity   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    oldyinitx   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);
    // orbit calculation
    if (g_l_old_z.x >= 0)
    {
        g_l_new_z.x = (oldxinitx - g_long_param->x - oldyinity);
        g_l_new_z.y = (oldyinitx - g_long_param->y + oldxinity);
    }
    else
    {
        g_l_new_z.x = (oldxinitx + g_long_param->x - oldyinity);
        g_l_new_z.y = (oldyinitx + g_long_param->y + oldxinity);
    }
    return g_bailout_long();
}

int Barnsley1FPFractal()
{
    /* Barnsley's Mandelbrot type M1 from "Fractals
    Everywhere" by Michael Barnsley, p. 322 */
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    // calculate intermediate products
    foldxinitx = g_old_z.x * g_float_param->x;
    foldyinity = g_old_z.y * g_float_param->y;
    foldxinity = g_old_z.x * g_float_param->y;
    foldyinitx = g_old_z.y * g_float_param->x;
    // orbit calculation
    if (g_old_z.x >= 0)
    {
        g_new_z.x = (foldxinitx - g_float_param->x - foldyinity);
        g_new_z.y = (foldyinitx - g_float_param->y + foldxinity);
    }
    else
    {
        g_new_z.x = (foldxinitx + g_float_param->x - foldyinity);
        g_new_z.y = (foldyinitx + g_float_param->y + foldxinity);
    }
    return g_bailout_float();
}

int Barnsley2Fractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 331, example 4.2 */
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    // calculate intermediate products
    oldxinitx   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    oldyinity   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    oldxinity   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    oldyinitx   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);

    // orbit calculation
    if (oldxinity + oldyinitx >= 0)
    {
        g_l_new_z.x = oldxinitx - g_long_param->x - oldyinity;
        g_l_new_z.y = oldyinitx - g_long_param->y + oldxinity;
    }
    else
    {
        g_l_new_z.x = oldxinitx + g_long_param->x - oldyinity;
        g_l_new_z.y = oldyinitx + g_long_param->y + oldxinity;
    }
    return g_bailout_long();
}

int Barnsley2FPFractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 331, example 4.2 */

    // calculate intermediate products
    foldxinitx = g_old_z.x * g_float_param->x;
    foldyinity = g_old_z.y * g_float_param->y;
    foldxinity = g_old_z.x * g_float_param->y;
    foldyinitx = g_old_z.y * g_float_param->x;

    // orbit calculation
    if (foldxinity + foldyinitx >= 0)
    {
        g_new_z.x = foldxinitx - g_float_param->x - foldyinity;
        g_new_z.y = foldyinitx - g_float_param->y + foldxinity;
    }
    else
    {
        g_new_z.x = foldxinitx + g_float_param->x - foldyinity;
        g_new_z.y = foldyinitx + g_float_param->y + foldxinity;
    }
    return g_bailout_float();
}

int JuliaFractal()
{
    /* used for C prototype of fast integer math routines for classic
       Mandelbrot and Julia */
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return g_bailout_long();
}

int JuliafpFractal()
{
    // floating point version of classical Mandelbrot/Julia
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return g_bailout_float();
}

int LambdaFPFractal()
{
    // variation of classical Mandelbrot/Julia
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    g_temp_sqr_x = g_old_z.x - g_temp_sqr_x + g_temp_sqr_y;
    g_temp_sqr_y = -(g_old_z.y * g_old_z.x);
    g_temp_sqr_y += g_temp_sqr_y + g_old_z.y;

    g_new_z.x = g_float_param->x * g_temp_sqr_x - g_float_param->y * g_temp_sqr_y;
    g_new_z.y = g_float_param->x * g_temp_sqr_y + g_float_param->y * g_temp_sqr_x;
    return g_bailout_float();
}

int LambdaFractal()
{
    // variation of classical Mandelbrot/Julia

    // in complex math) temp = Z * (1-Z)
    g_l_temp_sqr_x = g_l_old_z.x - g_l_temp_sqr_x + g_l_temp_sqr_y;
    g_l_temp_sqr_y = g_l_old_z.y
                - multiply(g_l_old_z.y, g_l_old_z.x, g_bit_shift_less_1);
    // (in complex math) Z = Lambda * Z
    g_l_new_z.x = multiply(g_long_param->x, g_l_temp_sqr_x, g_bit_shift)
             - multiply(g_long_param->y, g_l_temp_sqr_y, g_bit_shift);
    g_l_new_z.y = multiply(g_long_param->x, g_l_temp_sqr_y, g_bit_shift)
             + multiply(g_long_param->y, g_l_temp_sqr_x, g_bit_shift);
    return g_bailout_long();
}

int SierpinskiFractal()
{
    /* following code translated from basic - see "Fractals
    Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
    g_l_new_z.x = (g_l_old_z.x << 1);              // new.x = 2 * old.x
    g_l_new_z.y = (g_l_old_z.y << 1);              // new.y = 2 * old.y
    if (g_l_old_z.y > g_l_temp.y)  // if old.y > .5
    {
        g_l_new_z.y = g_l_new_z.y - g_l_temp.x;    // new.y = 2 * old.y - 1
    }
    else if (g_l_old_z.x > g_l_temp.y)     // if old.x > .5
    {
        g_l_new_z.x = g_l_new_z.x - g_l_temp.x;    // new.x = 2 * old.x - 1
    }
    // end barnsley code
    return g_bailout_long();
}

int SierpinskiFPFractal()
{
    /* following code translated from basic - see "Fractals
    Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */

    g_new_z.x = g_old_z.x + g_old_z.x;
    g_new_z.y = g_old_z.y + g_old_z.y;
    if (g_old_z.y > .5)
    {
        g_new_z.y = g_new_z.y - 1;
    }
    else if (g_old_z.x > .5)
    {
        g_new_z.x = g_new_z.x - 1;
    }

    // end barnsley code
    return g_bailout_float();
}

int LambdaexponentFractal()
{
    // found this in  "Science of Fractal Images"
    if (std::fabs(g_old_z.y) >= 1.0e3)
    {
        return 1;
    }
    if (std::fabs(g_old_z.x) >= 8)
    {
        return 1;
    }
    FPUsincos(&g_old_z.y, &siny, &cosy);

    if (g_old_z.x >= g_magnitude_limit && cosy >= 0.0)
    {
        return 1;
    }
    tmpexp = std::exp(g_old_z.x);
    g_tmp_z.x = tmpexp*cosy;
    g_tmp_z.y = tmpexp*siny;

    //multiply by lamda
    g_new_z.x = g_float_param->x*g_tmp_z.x - g_float_param->y*g_tmp_z.y;
    g_new_z.y = g_float_param->y*g_tmp_z.x + g_float_param->x*g_tmp_z.y;
    g_old_z = g_new_z;
    return 0;
}

int LongLambdaexponentFractal()
{
    // found this in  "Science of Fractal Images"
    if (labs(g_l_old_z.y) >= (1000L << g_bit_shift))
    {
        return 1;
    }
    if (labs(g_l_old_z.x) >= (8L << g_bit_shift))
    {
        return 1;
    }

    SinCos086(g_l_old_z.y, &lsiny,  &lcosy);

    if (g_l_old_z.x >= g_l_magnitude_limit && lcosy >= 0L)
    {
        return 1;
    }
    longtmp = Exp086(g_l_old_z.x);

    g_l_temp.x = multiply(longtmp,      lcosy,   g_bit_shift);
    g_l_temp.y = multiply(longtmp,      lsiny,   g_bit_shift);

    g_l_new_z.x  = multiply(g_long_param->x, g_l_temp.x, g_bit_shift)
              - multiply(g_long_param->y, g_l_temp.y, g_bit_shift);
    g_l_new_z.y  = multiply(g_long_param->x, g_l_temp.y, g_bit_shift)
              + multiply(g_long_param->y, g_l_temp.x, g_bit_shift);
    g_l_old_z = g_l_new_z;
    return 0;
}

int FloatTrigPlusExponentFractal()
{
    // another Scientific American biomorph type
    // z(n+1) = e**z(n) + trig(z(n)) + C

    if (std::fabs(g_old_z.x) >= 6.4e2)
    {
        return 1; // DOMAIN errors
    }
    tmpexp = std::exp(g_old_z.x);
    FPUsincos(&g_old_z.y, &siny, &cosy);
    CMPLXtrig0(g_old_z, g_new_z);

    //new =   trig(old) + e**old + C
    g_new_z.x += tmpexp*cosy + g_float_param->x;
    g_new_z.y += tmpexp*siny + g_float_param->y;
    return g_bailout_float();
}

inline bool trig16_check(long val)
{
    static constexpr long l16triglim = 8L << 16; // domain limit of fast trig functions

    return labs(val) > l16triglim;
}

int LongTrigPlusExponentFractal()
{
    // calculate exp(z)

    // domain check for fast transcendental functions
    if (trig16_check(g_l_old_z.x) || trig16_check(g_l_old_z.y))
    {
        return 1;
    }

    longtmp = Exp086(g_l_old_z.x);
    SinCos086(g_l_old_z.y, &lsiny,  &lcosy);
    LCMPLXtrig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += multiply(longtmp,    lcosy,   g_bit_shift) + g_long_param->x;
    g_l_new_z.y += multiply(longtmp,    lsiny,   g_bit_shift) + g_long_param->y;
    return g_bailout_long();
}

long g_xx_one;
long g_fudge_one;
long g_fudge_two;

int UnityFractal()
{
    g_xx_one = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift) + multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    if ((g_xx_one > g_fudge_two) || (labs(g_xx_one - g_fudge_one) < g_l_delta_min))
    {
        return 1;
    }
    g_l_old_z.y = multiply(g_fudge_two - g_xx_one, g_l_old_z.x, g_bit_shift);
    g_l_old_z.x = multiply(g_fudge_two - g_xx_one, g_l_old_z.y, g_bit_shift);
    g_l_new_z = g_l_old_z;
    return 0;
}

int UnityfpFractal()
{
    double XXOne;
    XXOne = sqr(g_old_z.x) + sqr(g_old_z.y);
    if ((XXOne > 2.0) || (std::fabs(XXOne - 1.0) < g_delta_min))
    {
        return 1;
    }
    g_old_z.y = (2.0 - XXOne)* g_old_z.x;
    g_old_z.x = (2.0 - XXOne)* g_old_z.y;
    g_new_z = g_old_z;
    return 0;
}

int Mandel4Fractal()
{
    /*
       this routine calculates the Mandelbrot/Julia set based on the
       polynomial z**4 + lambda
     */

    // first, compute (x + iy)**2
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    if (g_bailout_long())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return g_bailout_long();
}

int Mandel4fpFractal()
{
    // first, compute (x + iy)**2
    g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y;
    g_new_z.y = g_old_z.x*g_old_z.y*2;
    if (g_bailout_float())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y =  g_old_z.x*g_old_z.y*2 + g_float_param->y;
    return g_bailout_float();
}

int floatZtozPluszpwrFractal()
{
    cpower(&g_old_z, (int)g_params[2], &g_new_z);
    g_old_z = ComplexPower(g_old_z, g_old_z);
    g_new_z.x = g_new_z.x + g_old_z.x +g_float_param->x;
    g_new_z.y = g_new_z.y + g_old_z.y +g_float_param->y;
    return g_bailout_float();
}

int longZpowerFractal()
{
    if (lcpower(&g_l_old_z, g_c_exponent, &g_l_new_z, g_bit_shift))
    {
        g_l_new_z.y = 8L << g_bit_shift;
        g_l_new_z.x = g_l_new_z.y;
    }
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return g_bailout_long();
}

int longCmplxZpowerFractal()
{
    DComplex x, y;

    x.x = (double)g_l_old_z.x / g_fudge_factor;
    x.y = (double)g_l_old_z.y / g_fudge_factor;
    y.x = (double)g_l_param2.x / g_fudge_factor;
    y.y = (double)g_l_param2.y / g_fudge_factor;
    x = ComplexPower(x, y);
    if (std::fabs(x.x) < g_fudge_limit && fabs(x.y) < g_fudge_limit)
    {
        g_l_new_z.x = (long)(x.x * g_fudge_factor);
        g_l_new_z.y = (long)(x.y * g_fudge_factor);
    }
    else
    {
        g_overflow = true;
    }
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return g_bailout_long();
}

int floatZpowerFractal()
{
    cpower(&g_old_z, g_c_exponent, &g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

int floatCmplxZpowerFractal()
{
    g_new_z = ComplexPower(g_old_z, g_param_z2);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

int Barnsley3Fractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 292, example 4.1 */

    // calculate intermediate products
    oldxinitx   = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    oldyinity   = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    oldxinity   = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);

    // orbit calculation
    if (g_l_old_z.x > 0)
    {
        g_l_new_z.x = oldxinitx   - oldyinity - g_fudge_factor;
        g_l_new_z.y = oldxinity << 1;
    }
    else
    {
        g_l_new_z.x = oldxinitx - oldyinity - g_fudge_factor
                 + multiply(g_long_param->x, g_l_old_z.x, g_bit_shift);
        g_l_new_z.y = oldxinity <<1;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_l_new_z.y += multiply(g_long_param->y, g_l_old_z.x, g_bit_shift);
    }
    return g_bailout_long();
}

int Barnsley3FPFractal()
{
    /* An unnamed Mandelbrot/Julia function from "Fractals
    Everywhere" by Michael Barnsley, p. 292, example 4.1 */


    // calculate intermediate products
    foldxinitx  = g_old_z.x * g_old_z.x;
    foldyinity  = g_old_z.y * g_old_z.y;
    foldxinity  = g_old_z.x * g_old_z.y;

    // orbit calculation
    if (g_old_z.x > 0)
    {
        g_new_z.x = foldxinitx - foldyinity - 1.0;
        g_new_z.y = foldxinity * 2;
    }
    else
    {
        g_new_z.x = foldxinitx - foldyinity -1.0 + g_float_param->x * g_old_z.x;
        g_new_z.y = foldxinity * 2;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_new_z.y += g_float_param->y * g_old_z.x;
    }
    return g_bailout_float();
}

int TrigPlusZsquaredFractal()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C
    LCMPLXtrig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y += multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return g_bailout_long();
}

int TrigPlusZsquaredfpFractal()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C

    CMPLXtrig0(g_old_z, g_new_z);
    g_new_z.x += g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y += 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return g_bailout_float();
}

int PopcornFractal_Old()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = siny/cosy + g_old_z.y;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_new_z.x = g_old_z.x - g_param_z1.x*siny;
    g_new_z.y = g_old_z.y - g_param_z1.x*g_sin_x;
    if (g_plot == noplot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }
    else
    {
        g_temp_sqr_x = sqr(g_new_z.x);
    }
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int PopcornFractal()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = siny/cosy + g_old_z.y;
    FPUsincos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    FPUsincos(&g_tmp_z.y, &siny, &cosy);
    g_new_z.x = g_old_z.x - g_param_z1.x*siny;
    g_new_z.y = g_old_z.y - g_param_z1.x*g_sin_x;
    if (g_plot == noplot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit
        || std::fabs(g_new_z.x) > g_magnitude_limit2
        || std::fabs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static void ltrig_arg(long &val)
{
    if (trig16_check(val))
    {
        double tmp;
        tmp = val;
        tmp /= g_fudge_factor;
        tmp = std::fmod(tmp, PI * 2.0);
        tmp *= g_fudge_factor;
        val = (long) tmp;
    }
}

int LPopcornFractal_Old()
{
    g_l_temp = g_l_old_z;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(lsiny, lcosy, g_bit_shift) + g_l_old_z.y;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, lsiny, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, lsinx, g_bit_shift);
    if (g_plot == noplot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    else
    {
        g_l_temp_sqr_x = lsqr(g_l_new_z.x);
        g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    }
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int LPopcornFractal()
{
    g_l_temp = g_l_old_z;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(lsiny, lcosy, g_bit_shift) + g_l_old_z.y;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    SinCos086(g_l_temp.x, &lsinx, &lcosx);
    SinCos086(g_l_temp.y, &lsiny, &lcosy);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, lsiny, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, lsinx, g_bit_shift);
    if (g_plot == noplot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    // else
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

// Popcorn generalization

int PopcornFractalFn()
{
    DComplex tmpx;
    DComplex tmpy;

    // tmpx contains the generalized value of the old real "x" equation
    g_tmp_z = g_param_z2*g_old_z.y;  // tmp = (C * old.y)
    CMPLXtrig1(g_tmp_z, tmpx);             // tmpx = trig1(tmp)
    tmpx.x += g_old_z.y;                  // tmpx = old.y + trig1(tmp)
    CMPLXtrig0(tmpx, g_tmp_z);             // tmp = trig0(tmpx)
    CMPLXmult(g_tmp_z, g_param_z1, tmpx);         // tmpx = tmp * h

    // tmpy contains the generalized value of the old real "y" equation
    g_tmp_z = g_param_z2*g_old_z.x;  // tmp = (C * old.x)
    CMPLXtrig3(g_tmp_z, tmpy);             // tmpy = trig3(tmp)
    tmpy.x += g_old_z.x;                  // tmpy = old.x + trig1(tmp)
    CMPLXtrig2(tmpy, g_tmp_z);             // tmp = trig2(tmpy)

    CMPLXmult(g_tmp_z, g_param_z1, tmpy);         // tmpy = tmp * h

    g_new_z.x = g_old_z.x - tmpx.x - tmpy.y;
    g_new_z.y = g_old_z.y - tmpy.x - tmpx.y;

    if (g_plot == noplot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }

    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit
        || std::fabs(g_new_z.x) > g_magnitude_limit2
        || std::fabs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

#define FIX_OVERFLOW(arg)           \
    if (g_overflow)                   \
    {                               \
        (arg).x = g_fudge_factor;   \
        (arg).y = 0;                \
        g_overflow = false;           \
   }

int LPopcornFractalFn()
{
    LComplex ltmpx, ltmpy;

    g_overflow = false;

    // ltmpx contains the generalized value of the old real "x" equation
    LCMPLXtimesreal(g_l_param2, g_l_old_z.y, g_l_temp); // tmp = (C * old.y)
    LCMPLXtrig1(g_l_temp, ltmpx);             // tmpx = trig1(tmp)
    FIX_OVERFLOW(ltmpx);
    ltmpx.x += g_l_old_z.y;                   // tmpx = old.y + trig1(tmp)
    LCMPLXtrig0(ltmpx, g_l_temp);             // tmp = trig0(tmpx)
    FIX_OVERFLOW(g_l_temp);
    LCMPLXmult(g_l_temp, g_l_param, ltmpx);        // tmpx = tmp * h

    // ltmpy contains the generalized value of the old real "y" equation
    LCMPLXtimesreal(g_l_param2, g_l_old_z.x, g_l_temp); // tmp = (C * old.x)
    LCMPLXtrig3(g_l_temp, ltmpy);             // tmpy = trig3(tmp)
    FIX_OVERFLOW(ltmpy);
    ltmpy.x += g_l_old_z.x;                   // tmpy = old.x + trig1(tmp)
    LCMPLXtrig2(ltmpy, g_l_temp);             // tmp = trig2(tmpy)
    FIX_OVERFLOW(g_l_temp);
    LCMPLXmult(g_l_temp, g_l_param, ltmpy);        // tmpy = tmp * h

    g_l_new_z.x = g_l_old_z.x - ltmpx.x - ltmpy.y;
    g_l_new_z.y = g_l_old_z.y - ltmpy.x - ltmpx.y;

    if (g_plot == noplot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int ScottZXTrigPlusZFractal()
{
    // z = (z*trig(z))+z
    LCMPLXtrig0(g_l_old_z, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_old_z, g_l_temp, g_l_new_z);       // lnew  = old*trig(old)
    LCMPLXadd(g_l_new_z, g_l_old_z, g_l_new_z);        // lnew  = trig(old) + old
    return g_bailout_long();
}

int SkinnerZXTrigSubZFractal()
{
    // z = (z*trig(z))-z
    LCMPLXtrig0(g_l_old_z, g_l_temp);          // ltmp  = trig(old)
    LCMPLXmult(g_l_old_z, g_l_temp, g_l_new_z);       // lnew  = old*trig(old)
    LCMPLXsub(g_l_new_z, g_l_old_z, g_l_new_z);        // lnew  = trig(old) - old
    return g_bailout_long();
}

int ScottZXTrigPlusZfpFractal()
{
    // z = (z*trig(z))+z
    CMPLXtrig0(g_old_z, g_tmp_z);         // tmp  = trig(old)
    CMPLXmult(g_old_z, g_tmp_z, g_new_z);       // new  = old*trig(old)
    g_new_z += g_old_z;        // new  = trig(old) + old
    return g_bailout_float();
}

int SkinnerZXTrigSubZfpFractal()
{
    // z = (z*trig(z))-z
    CMPLXtrig0(g_old_z, g_tmp_z);         // tmp  = trig(old)
    CMPLXmult(g_old_z, g_tmp_z, g_new_z);       // new  = old*trig(old)
    g_new_z -= g_old_z;        // new  = trig(old) - old
    return g_bailout_float();
}

/* The following four fractals are based on the idea of parallel
   or alternate calculations.  The shift is made when the mod
   reaches a given value.  */

int LambdaTrigOrTrigFractal()
{
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if ((LCMPLXmod(g_l_old_z)) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old_z, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);
    }
    else
    {
        LCMPLXtrig1(g_l_old_z, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);
    }
    return g_bailout_long();
}

int LambdaTrigOrTrigfpFractal()
{
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if (CMPLXmod(g_old_z) < g_param_z2.x)
    {
        CMPLXtrig0(g_old_z, g_old_z);
        FPUcplxmul(g_float_param, &g_old_z, &g_new_z);
    }
    else
    {
        CMPLXtrig1(g_old_z, g_old_z);
        FPUcplxmul(g_float_param, &g_old_z, &g_new_z);
    }
    return g_bailout_float();
}

int JuliaTrigOrTrigFractal()
{
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (LCMPLXmod(g_l_old_z) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old_z, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new_z);
    }
    else
    {
        LCMPLXtrig1(g_l_old_z, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new_z);
    }
    return g_bailout_long();
}

int JuliaTrigOrTrigfpFractal()
{
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (CMPLXmod(g_old_z) < g_param_z2.x)
    {
        CMPLXtrig0(g_old_z, g_old_z);
        g_new_z = *g_float_param + g_old_z;
    }
    else
    {
        CMPLXtrig1(g_old_z, g_old_z);
        g_new_z = *g_float_param + g_old_z;
    }
    return g_bailout_float();
}

int ScottTrigPlusTrigFractal()
{
    // z = trig0(z)+trig1(z)
    LCMPLXtrig0(g_l_old_z, g_l_temp);
    LCMPLXtrig1(g_l_old_z, g_l_old_z);
    LCMPLXadd(g_l_temp, g_l_old_z, g_l_new_z);
    return g_bailout_long();
}

int ScottTrigPlusTrigfpFractal()
{
    // z = trig0(z)+trig1(z)
    CMPLXtrig0(g_old_z, g_tmp_z);
    CMPLXtrig1(g_old_z, tmp2);
    g_new_z = g_tmp_z + tmp2;
    return g_bailout_float();
}

int SkinnerTrigSubTrigFractal()
{
    // z = trig(0, z)-trig1(z)
    LCMPLXtrig0(g_l_old_z, g_l_temp);
    LCMPLXtrig1(g_l_old_z, ltmp2);
    LCMPLXsub(g_l_temp, ltmp2, g_l_new_z);
    return g_bailout_long();
}

int SkinnerTrigSubTrigfpFractal()
{
    // z = trig0(z)-trig1(z)
    CMPLXtrig0(g_old_z, g_tmp_z);
    CMPLXtrig1(g_old_z, tmp2);
    g_new_z = g_tmp_z - tmp2;
    return g_bailout_float();
}

//******************************************************************
//  Next six orbit functions are one type - extra functions are
//    special cases written for speed.
//******************************************************************

int TrigPlusSqrFractal() // generalization of Scott and Skinner types
{
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old_z, g_l_temp);     // ltmp = trig(lold)
    LCMPLXmult(g_l_param, g_l_temp, g_l_new_z); // lnew = lparm*trig(lold)
    LCMPLXsqr_old(g_l_temp);         // ltmp = sqr(lold)
    LCMPLXmult(g_l_param2, g_l_temp, g_l_temp);// ltmp = lparm2*sqr(lold)
    LCMPLXadd(g_l_new_z, g_l_temp, g_l_new_z);   // lnew = lparm*trig(lold)+lparm2*sqr(lold)
    return g_bailout_long();
}

int TrigPlusSqrfpFractal() // generalization of Scott and Skinner types
{
    // { z=pixel: z=(p1,p2)*trig(z)+(p3,p4)*sqr(z), |z|<BAILOUT }
    CMPLXtrig0(g_old_z, g_tmp_z);     // tmp = trig(old)
    CMPLXmult(g_param_z1, g_tmp_z, g_new_z); // new = parm*trig(old)
    CMPLXsqr_old(g_tmp_z);        // tmp = sqr(old)
    CMPLXmult(g_param_z2, g_tmp_z, tmp2); // tmp = parm2*sqr(old)
    g_new_z += tmp2;    // new = parm*trig(old)+parm2*sqr(old)
    return g_bailout_float();
}

int ScottTrigPlusSqrFractal()
{
    //  { z=pixel: z=trig(z)+sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old_z, g_l_new_z);    // lnew = trig(lold)
    LCMPLXsqr_old(g_l_temp);        // lold = sqr(lold)
    LCMPLXadd(g_l_temp, g_l_new_z, g_l_new_z);  // lnew = trig(lold)+sqr(lold)
    return g_bailout_long();
}

int ScottTrigPlusSqrfpFractal() // float version
{
    // { z=pixel: z=sin(z)+sqr(z), |z|<BAILOUT }
    CMPLXtrig0(g_old_z, g_new_z);       // new = trig(old)
    CMPLXsqr_old(g_tmp_z);          // tmp = sqr(old)
    g_new_z += g_tmp_z;      // new = trig(old)+sqr(old)
    return g_bailout_float();
}

int SkinnerTrigSubSqrFractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    LCMPLXtrig0(g_l_old_z, g_l_new_z);    // lnew = trig(lold)
    LCMPLXsqr_old(g_l_temp);        // lold = sqr(lold)
    LCMPLXsub(g_l_new_z, g_l_temp, g_l_new_z);  // lnew = trig(lold)-sqr(lold)
    return g_bailout_long();
}

int SkinnerTrigSubSqrfpFractal()
{
    // { z=pixel: z=sin(z)-sqr(z), |z|<BAILOUT }
    CMPLXtrig0(g_old_z, g_new_z);   // new = trig(old)
    CMPLXsqr_old(g_tmp_z);          // old = sqr(old)
    g_new_z -= g_tmp_z;             // new = trig(old)-sqr(old)
    return g_bailout_float();
}

int Magnet1Fractal()    //    Z = ((Z**2 + C - 1)/(2Z + C - 2))**2
{
    //  In "Beauty of Fractals", code by Kev Allen.
    DComplex top, bot, tmp;
    double div;

    top.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x - 1; // top = Z**2+C-1
    top.y = g_old_z.x * g_old_z.y;
    top.y = top.y + top.y + g_float_param->y;

    bot.x = g_old_z.x + g_old_z.x + g_float_param->x - 2;       // bot = 2*Z+C-2
    bot.y = g_old_z.y + g_old_z.y + g_float_param->y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new_z.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new_z.y = tmp.x * tmp.y;
    g_new_z.y += g_new_z.y;

    return g_bailout_float();
}

int Magnet2Fractal()  // Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2)  ) /
//       (3Z**2 + 3(C-2)Z + (C-1)(C-2)+1) )**2
{
    //   In "Beauty of Fractals", code by Kev Allen.
    DComplex top, bot, tmp;
    double div;

    top.x = g_old_z.x * (g_temp_sqr_x-g_temp_sqr_y-g_temp_sqr_y-g_temp_sqr_y + T_Cm1.x)
            - g_old_z.y * T_Cm1.y + T_Cm1Cm2.x;
    top.y = g_old_z.y * (g_temp_sqr_x+g_temp_sqr_x+g_temp_sqr_x-g_temp_sqr_y + T_Cm1.x)
            + g_old_z.x * T_Cm1.y + T_Cm1Cm2.y;

    bot.x = g_temp_sqr_x - g_temp_sqr_y;
    bot.x = bot.x + bot.x + bot.x
            + g_old_z.x * T_Cm2.x - g_old_z.y * T_Cm2.y
            + T_Cm1Cm2.x + 1.0;
    bot.y = g_old_z.x * g_old_z.y;
    bot.y += bot.y;
    bot.y = bot.y + bot.y + bot.y
            + g_old_z.x * T_Cm2.y + g_old_z.y * T_Cm2.x
            + T_Cm1Cm2.y;

    div = bot.x*bot.x + bot.y*bot.y;                // tmp = top/bot
    if (div < FLT_MIN)
    {
        return 1;
    }
    tmp.x = (top.x*bot.x + top.y*bot.y)/div;
    tmp.y = (top.y*bot.x - top.x*bot.y)/div;

    g_new_z.x = (tmp.x + tmp.y) * (tmp.x - tmp.y);      // Z = tmp**2
    g_new_z.y = tmp.x * tmp.y;
    g_new_z.y += g_new_z.y;

    return g_bailout_float();
}

int LambdaTrigFractal()
{
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2 || labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);   // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int LambdaTrigfpFractal()
{
    if (std::fabs(g_old_z.x) >= g_magnitude_limit2 || fabs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

// bailouts are different for different trig functions
int LambdaTrigFractal1()
{
    // sin,cos
    if (labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);               // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z); // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int LambdaTrigfpFractal1()
{
    // sin,cos
    if (std::fabs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    CMPLXtrig0(g_old_z, g_tmp_z);                // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z); // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

int LambdaTrigFractal2()
{
    // sinh,cosh
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);               // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z); // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int LambdaTrigfpFractal2()
{
    // sinh,cosh
    if (std::fabs(g_old_z.x) >= g_magnitude_limit2)
    {
        return 1;
    }
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

int CirclefpFractal()
{
    long i;
    i = (long)(g_params[0]*(g_temp_sqr_x+g_temp_sqr_y));
    g_color_iter = i%g_colors;
    return 1;
}
/*
CirclelongFractal()
{
   long i;
   i = multiply(lparm.x,(g_l_temp_sqr_x+g_l_temp_sqr_y),g_bit_shift);
   i = i >> g_bit_shift;
   g_color_iter = i%colors);
   return 1;
}
*/

// --------------------------------------------------------------------
//              Initialization (once per pixel) routines
// --------------------------------------------------------------------

// this code translated to asm - lives in newton.asm
// transform points with reciprocal function
void invertz2(DComplex *z)
{
    z->x = g_dx_pixel();
    z->y = g_dy_pixel();
    z->x -= g_f_x_center;
    z->y -= g_f_y_center;  // Normalize values to center of circle

    g_temp_sqr_x = sqr(z->x) + sqr(z->y);  // Get old radius
    if (std::fabs(g_temp_sqr_x) > FLT_MIN)
    {
        g_temp_sqr_x = g_f_radius / g_temp_sqr_x;
    }
    else
    {
        g_temp_sqr_x = FLT_MAX;   // a big number, but not TOO big
    }
    z->x *= g_temp_sqr_x;
    z->y *= g_temp_sqr_x;      // Perform inversion
    z->x += g_f_x_center;
    z->y += g_f_y_center; // Renormalize
}

int long_julia_per_pixel()
{
    // integer julia types
    // lambda
    // barnsleyj1
    // barnsleyj2
    // sierpinski
    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
        {
            g_old_z.x = 8;  // value to bail out in one iteration
            g_old_z.y = 8;
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }
    return 0;
}

int long_mandel_per_pixel()
{
    // integer mandel types
    // barnsleym1
    // barnsleym2
    g_l_init.x = g_l_x_pixel();
    g_l_init.y = g_l_y_pixel();

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_init);

        // watch out for overflow
        if (sqr(g_init.x)+sqr(g_init.y) >= 127)
        {
            g_init.x = 8;  // value to bail out in one iteration
            g_init.y = 8;
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else
    {
        g_l_old_z = g_l_init;
    }

    g_l_old_z.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old_z.y += g_l_param.y;
    return 1; // 1st iteration has been done
}

int julia_per_pixel()
{
    // julia

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (g_bit_shift <= 24)
        {
            if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
            {
                g_old_z.x = 8;  // value to bail out in one iteration
                g_old_z.y = 8;
            }
        }
        if (g_bit_shift >  24)
        {
            if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 4.0)
            {
                g_old_z.x = 2;  // value to bail out in one iteration
                g_old_z.y = 2;
            }
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }

    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    g_l_temp = g_l_old_z;
    return 0;
}

int mandel_per_pixel()
{
    // mandel

    if (g_invert != 0)
    {
        invertz2(&g_init);

        // watch out for overflow
        if (g_bit_shift <= 24)
        {
            if (sqr(g_init.x)+sqr(g_init.y) >= 127)
            {
                g_init.x = 8;  // value to bail out in one iteration
                g_init.y = 8;
            }
        }
        if (g_bit_shift >  24)
        {
            if (sqr(g_init.x)+sqr(g_init.y) >= 4)
            {
                g_init.x = 2;  // value to bail out in one iteration
                g_init.y = 2;
            }
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }
    else
    {
        g_l_init.x = g_l_x_pixel();
        g_l_init.y = g_l_y_pixel();
    }
    switch (g_fractal_type)
    {
    case fractal_type::MANDELLAMBDA:              // Critical Value 0.5 + 0.0i
        g_l_old_z.x = g_fudge_half;
        g_l_old_z.y = 0;
        break;
    default:
        g_l_old_z = g_l_init;
        break;
    }

    // alter init value
    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else if (g_use_init_orbit == init_orbit_mode::pixel)
    {
        g_l_old_z = g_l_init;
    }

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_l_old_z.x = g_l_param.x; // initial pertubation of parameters set
        g_l_old_z.y = g_l_param.y;
        g_color_iter = -1;
    }
    else
    {
        g_l_old_z.x += g_l_param.x; // initial pertubation of parameters set
        g_l_old_z.y += g_l_param.y;
    }
    g_l_temp = g_l_init; // for spider
    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    return 1; // 1st iteration has been done
}

int mandelfp_per_pixel()
{
    // floating point mandelbrot
    // mandelfp

    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }
    switch (g_fractal_type)
    {
    case fractal_type::MAGNET2M:
        FloatPreCalcMagnet2();
    case fractal_type::MAGNET1M:
        g_old_z.y = 0.0;        // Critical Val Zero both, but neither
        g_old_z.x = g_old_z.y;      // is of the form f(Z,C) = Z*g(Z)+C
        break;
    case fractal_type::MANDELLAMBDAFP:            // Critical Value 0.5 + 0.0i
        g_old_z.x = 0.5;
        g_old_z.y = 0.0;
        break;
    default:
        g_old_z = g_init;
        break;
    }

    // alter init value
    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_old_z = g_init_orbit;
    }
    else if (g_use_init_orbit == init_orbit_mode::pixel)
    {
        g_old_z = g_init;
    }

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_old_z.x = g_param_z1.x; // initial pertubation of parameters set
        g_old_z.y = g_param_z1.y;
        g_color_iter = -1;
    }
    else
    {
        g_old_z.x += g_param_z1.x;
        g_old_z.y += g_param_z1.y;
    }
    g_tmp_z = g_init; // for spider
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value for regular Mandelbrot
    g_temp_sqr_y = sqr(g_old_z.y);
    return 1; // 1st iteration has been done
}

int juliafp_per_pixel()
{
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value for regular Julia
    g_temp_sqr_y = sqr(g_old_z.y);
    g_tmp_z = g_old_z;
    return 0;
}

int MPCjulia_per_pixel()
{
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    mpcold.x = *pd2MP(g_old_z.x);
    mpcold.y = *pd2MP(g_old_z.y);
    return 0;
}

int othermandelfp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_old_z = g_init_orbit;
    }
    else
    {
        g_old_z = g_init;
    }

    g_old_z.x += g_param_z1.x;      // initial pertubation of parameters set
    g_old_z.y += g_param_z1.y;

    return 1; // 1st iteration has been done
}

int otherjuliafp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    return 0;
}
