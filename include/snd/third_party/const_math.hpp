#pragma once

/* 
 C++11 constexpr versions of cmath functions needed for the FFT.
 Copyright Paul Keir 2012-2016
 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file license.txt or copy at http://boost.org/LICENSE_1_0.txt)
*/

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <limits> // nan

namespace const_math {

constexpr double tol = 0.001;

constexpr double abs(const double x)    { return x < 0.0 ? -x : x; }

constexpr double square(const double x) { return x*x; }

constexpr double sqrt_helper(const double x, const double g) {
  return abs(g-x/g) < tol ? g : sqrt_helper(x,(g+x/g)/2.0);
}

constexpr double sqrt(const double x) { return sqrt_helper(x,1.0); }

constexpr double cube(const double x) { return x*x*x; }

// Based on the triple-angle formula: sin 3x = 3 sin x - 4 sin ^3 x
constexpr
double sin_helper(const double x) {
  return x < tol ? x : 3*(sin_helper(x/3.0)) - 4*cube(sin_helper(x/3.0));
}

constexpr
double sin(const double x) {
  return sin_helper(x < 0 ? -x+M_PI : x);
}

//sinh 3x = 3 sinh x + 4 sinh ^3 x
constexpr
double sinh_helper(const double x) {
  return x < tol ? x : 3*(sinh_helper(x/3.0)) + 4*cube(sinh_helper(x/3.0));
}

//sinh 3x = 3 sinh x + 4 sinh ^3 x
constexpr
double sinh(const double x) {
  return x < 0 ? -sinh_helper(-x) : sinh_helper(x);
}

constexpr double cos (const double x) { return sin(M_PI_2 - x); }

constexpr double cosh(const double x) { return sqrt(1.0 + square(sinh(x))); }

constexpr
double pow(double base, int exponent) {
  return exponent <  0 ? 1.0 / pow(base,-exponent) :
         exponent == 0 ? 1.                        :
         exponent == 1 ? base                      :
         base * pow(base,exponent-1);
}

// atan formulae from http://mathonweb.com/algebra_e-book.htm
// x - x^3/3 + x^5/5 - x^7/7+x^9/9  etc.
constexpr
double atan_poly_helper(const double res,  const double num1,
                        const double den1, const double delta) {
  return res < tol ? res :
         res + atan_poly_helper((num1*delta)/(den1+2.)-num1/den1,
                                 num1*delta*delta,den1+4.,delta);
}

constexpr
double atan_poly(const double x) {
  return x + atan_poly_helper(pow(x,5)/5.-pow(x,3)/3., pow(x,7), 7., x*x);
}

// Define an M_PI_6? Define a root 3?
constexpr
double atan_identity(const double x) {
  return x <= (2. - sqrt(3.)) ? atan_poly(x) :
         (M_PI_2 / 3.) + atan_poly((sqrt(3.)*x-1)/(sqrt(3.)+x));
}

constexpr
double atan_cmplmntry(const double x) {
  return (x < 1) ? atan_identity(x) : M_PI_2 - atan_identity(1/x);
}

constexpr
double atan(const double x) {
  return (x >= 0) ? atan_cmplmntry(x) : -atan_cmplmntry(-x);
}

constexpr
double atan2(const double y, const double x) {
  return           x >  0 ? atan(y/x)        : 
         y >= 0 && x <  0 ? atan(y/x) + M_PI :
         y <  0 && x <  0 ? atan(y/x) - M_PI :
         y >  0 && x == 0 ?  M_PI_2          :
         y <  0 && x == 0 ? -M_PI_2          : 0;   // 0 == undefined
}

constexpr
double nearest(double x) {
  return (x-0.5) > (int)x ? (int)(x+0.5) : (int)x;
}

constexpr
double fraction(double x) {
  return (x-0.5) > (int)x ? -(((double)(int)(x+0.5))-x) : x-((double)(int)(x));
}

constexpr
double exp_helper(const double r) {
  return 1.0 + r + pow(r,2)/2.0   + pow(r,3)/6.0   +
                   pow(r,4)/24.0  + pow(r,5)/120.0 +
                   pow(r,6)/720.0 + pow(r,7)/5040.0;
}

// exp(x) = e^n . e^r (where n is an integer, and -0.5 > r < 0.5
// exp(r) = e^r = 1 + r + r^2/2 + r^3/6 + r^4/24 + r^5/120
constexpr
double exp(const double x) {
  return pow(M_E,static_cast<int>(nearest(x))) * exp_helper(fraction(x));
}

constexpr
double mantissa(const double x) {
  return x >= 10.0 ? mantissa(x *  0.1) :
         x <  1.0  ? mantissa(x * 10.0) :
         x;
}

// log(m) = log(sqrt(m)^2) = 2 x log( sqrt(m) )
// log(x) = log(m x 10^p) = 0.86858896 ln( sqrt(m) ) + p
constexpr
int exponent_helper(const double x, const int e) {
  return x >= 10.0 ? exponent_helper(x *  0.1, e+1) :
         x <  1.0  ? exponent_helper(x * 10.0, e-1) :
         e;
}

constexpr
int exponent(const double x) { return exponent_helper(x,0); }

constexpr
double log_helper2(const double y) {
  return 2.0 * (y + pow(y,3)/3.0 + pow(y,5)/5.0 +
                    pow(y,7)/7.0 + pow(y,9)/9.0 + pow(y,11)/11.0);
}

// log in the range 1-sqrt(10)
constexpr
double log_helper(const double x) { return log_helper2((x-1.0)/(x+1.0)); }

// n.b. log 10 is 2.3025851
// n.b. log m = log (sqrt(m)^2) = 2 * log sqrt(m)
constexpr
double log(const double x) {
  return x == 0 ? -std::numeric_limits<double>::infinity() :
         x <  0 ?  std::numeric_limits<double>::quiet_NaN() :
         2.0 * log_helper(sqrt(mantissa(x))) + 2.3025851 * exponent(x);
}

} // const_math