#pragma once
#if !defined(FRACTALS_H)
#define FRACTALS_H

extern void FloatPreCalcMagnet2();
extern void cpower(DComplex *, int, DComplex *);
extern int lcpower(LComplex *, int, LComplex *, int);
extern int lcomplex_mult(LComplex, LComplex, LComplex *, int);
extern int MPCNewtonFractal();
extern int Barnsley1Fractal();
extern int Barnsley1FPFractal();
extern int Barnsley2Fractal();
extern int Barnsley2FPFractal();
extern int JuliaFractal();
extern int JuliafpFractal();
extern int LambdaFPFractal();
extern int LambdaFractal();
extern int SierpinskiFractal();
extern int SierpinskiFPFractal();
extern int LambdaexponentFractal();
extern int LongLambdaexponentFractal();
extern int FloatTrigPlusExponentFractal();
extern int LongTrigPlusExponentFractal();
extern int MarksLambdaFractal();
extern int MarksLambdafpFractal();
extern int UnityFractal();
extern int UnityfpFractal();
extern int Mandel4Fractal();
extern int Mandel4fpFractal();
extern int floatZtozPluszpwrFractal();
extern int longZpowerFractal();
extern int longCmplxZpowerFractal();
extern int floatZpowerFractal();
extern int floatCmplxZpowerFractal();
extern int Barnsley3Fractal();
extern int Barnsley3FPFractal();
extern int TrigPlusZsquaredFractal();
extern int TrigPlusZsquaredfpFractal();
extern int Richard8fpFractal();
extern int Richard8Fractal();
extern int PopcornFractal();
extern int LPopcornFractal();
extern int PopcornFractal_Old();
extern int LPopcornFractal_Old();
extern int PopcornFractalFn();
extern int LPopcornFractalFn();
extern int MarksCplxMand();
extern int SpiderfpFractal();
extern int SpiderFractal();
extern int TetratefpFractal();
extern int ZXTrigPlusZFractal();
extern int ScottZXTrigPlusZFractal();
extern int SkinnerZXTrigSubZFractal();
extern int ZXTrigPlusZfpFractal();
extern int ScottZXTrigPlusZfpFractal();
extern int SkinnerZXTrigSubZfpFractal();
extern int Sqr1overTrigFractal();
extern int Sqr1overTrigfpFractal();
extern int TrigPlusTrigFractal();
extern int TrigPlusTrigfpFractal();
extern int ScottTrigPlusTrigFractal();
extern int ScottTrigPlusTrigfpFractal();
extern int SkinnerTrigSubTrigFractal();
extern int SkinnerTrigSubTrigfpFractal();
extern int TrigXTrigfpFractal();
extern int TrigXTrigFractal();
extern int TrigPlusSqrFractal();
extern int TrigPlusSqrfpFractal();
extern int ScottTrigPlusSqrFractal();
extern int ScottTrigPlusSqrfpFractal();
extern int SkinnerTrigSubSqrFractal();
extern int SkinnerTrigSubSqrfpFractal();
extern int TrigZsqrdfpFractal();
extern int TrigZsqrdFractal();
extern int SqrTrigFractal();
extern int SqrTrigfpFractal();
extern int Magnet1Fractal();
extern int Magnet2Fractal();
extern int LambdaTrigFractal();
extern int LambdaTrigfpFractal();
extern int LambdaTrigFractal1();
extern int LambdaTrigfpFractal1();
extern int LambdaTrigFractal2();
extern int LambdaTrigfpFractal2();
extern int ManOWarFractal();
extern int ManOWarfpFractal();
extern int MarksMandelPwrfpFractal();
extern int MarksMandelPwrFractal();
extern int TimsErrorfpFractal();
extern int TimsErrorFractal();
extern int CirclefpFractal();
extern int VLfpFractal();
extern int EscherfpFractal();
extern int long_julia_per_pixel();
extern int long_richard8_per_pixel();
extern int long_mandel_per_pixel();
extern int julia_per_pixel();
extern int marks_mandelpwr_per_pixel();
extern int mandel_per_pixel();
extern int marksmandel_per_pixel();
extern int marksmandelfp_per_pixel();
extern int marks_mandelpwrfp_per_pixel();
extern int mandelfp_per_pixel();
extern int juliafp_per_pixel();
extern int MPCjulia_per_pixel();
extern int otherrichard8fp_per_pixel();
extern int othermandelfp_per_pixel();
extern int otherjuliafp_per_pixel();
extern int MarksCplxMandperp();
extern int LambdaTrigOrTrigFractal();
extern int LambdaTrigOrTrigfpFractal();
extern int JuliaTrigOrTrigFractal();
extern int JuliaTrigOrTrigfpFractal();
extern int HalleyFractal();
extern int Halley_per_pixel();
extern int MPCHalleyFractal();
extern int MPCHalley_per_pixel();
extern int dynamfloat(double *, double *, double*);
extern int mandelcloudfloat(double *, double *, double*);
extern int dynam2dfloat();
extern int QuaternionFPFractal();
extern int quaternionfp_per_pixel();
extern int quaternionjulfp_per_pixel();
extern int LongPhoenixFractal();
extern int PhoenixFractal();
extern int long_phoenix_per_pixel();
extern int phoenix_per_pixel();
extern int long_mandphoenix_per_pixel();
extern int mandphoenix_per_pixel();
extern int HyperComplexFPFractal();
extern int LongPhoenixFractalcplx();
extern int PhoenixFractalcplx();
extern int (*floatbailout)();
extern int (*longbailout)();
extern int (*bignumbailout)();
extern int (*bigfltbailout)();
extern int fpMODbailout();
extern int fpREALbailout();
extern int fpIMAGbailout();
extern int fpORbailout();
extern int fpANDbailout();
extern int fpMANHbailout();
extern int fpMANRbailout();
extern int bnMODbailout();
extern int bnREALbailout();
extern int bnIMAGbailout();
extern int bnORbailout();
extern int bnANDbailout();
extern int bnMANHbailout();
extern int bnMANRbailout();
extern int bfMODbailout();
extern int bfREALbailout();
extern int bfIMAGbailout();
extern int bfORbailout();
extern int bfANDbailout();
extern int bfMANHbailout();
extern int bfMANRbailout();
extern int LongPhoenixFractal();
extern int PhoenixFractal();
extern int LongPhoenixFractalcplx();
extern int PhoenixFractalcplx();
extern int LongPhoenixPlusFractal();
extern int PhoenixPlusFractal();
extern int LongPhoenixMinusFractal();
extern int PhoenixMinusFractal();
extern int LongPhoenixCplxPlusFractal();
extern int PhoenixCplxPlusFractal();
extern int LongPhoenixCplxMinusFractal();
extern int PhoenixCplxMinusFractal();
extern int long_phoenix_per_pixel();
extern int phoenix_per_pixel();
extern int long_mandphoenix_per_pixel();
extern int mandphoenix_per_pixel();
extern void set_pixel_calc_functions();
extern int MandelbrotMix4fp_per_pixel();
extern int MandelbrotMix4fpFractal();
extern bool MandelbrotMix4Setup();

#endif