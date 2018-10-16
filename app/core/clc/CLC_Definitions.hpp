#ifndef FRACTALEXPLORER_CLC_DEFINITIONS_HPP
#define FRACTALEXPLORER_CLC_DEFINITIONS_HPP

#include <string_view>

static constexpr std::string_view DEFINITIONS_SOURCE { R"CL(
#ifndef __COMMON_CLH
#define __COMMON_CLH

//
// Implementation of some complex numbers operations
// #define USE_DOUBLE_PRECISION

#if (defined(cl_khr_fp64) && defined(USE_DOUBLE_PRECISION))
    #define real  double
    #define real2 double2
    #define PI M_PI
    #define COMPL_ONE_2nd_ROOT_REAL -0.5
    #define COMPL_ONE_2nd_ROOT_IMAG 0.866025403784438596
#else
    #define real  float
    #define real2 float2
    #define PI M_PI_F
    #define COMPL_ONE_2nd_ROOT_REAL -0.5f
    #define COMPL_ONE_2nd_ROOT_IMAG 0.86603f
#endif

#define COMPL_ONE_3rd_ROOT_REAL COMPL_ONE_2nd_ROOT_REAL
#define COMPL_ONE_3rd_ROOT_IMAG -COMPL_ONE_2nd_ROOT_IMAG

// division
real2 complex_div(real2 a, real2 b) {
    real sq = b.x*b.x + b.y*b.y;
    real2 result = {(a.x*b.x + a.y*b.y) / sq, (a.y*b.x - a.x*b.y) / sq};
    return result;
}

// square root
void complex_sqrt(real2 a, real2 roots[]) {
    roots[0].x = sqrt((  a.x + length(a)) / 2);
    roots[0].y = sign(a.y) * sqrt(( -a.x + length(a)) / 2);
    roots[1].x = -roots[0].x;
    roots[1].y = -roots[0].y;
}

// cube root
void complex_cbrt(real2 a, real2 roots[]) {
    real cbrt_abs_a = cbrt(length(a));
    real phi = atan2(a.y, a.x) / 3.0;
    real temp = 0;
    roots[0].y = sincos(phi, &temp) * cbrt_abs_a;
    roots[0].x = temp * cbrt_abs_a;
    roots[1].y = sincos(phi + 2*PI/3.0, &temp) * cbrt_abs_a;
    roots[1].x = temp * cbrt_abs_a;
    roots[2].y = sincos(phi + 4*PI/3.0, &temp) * cbrt_abs_a;
    roots[2].x = temp * cbrt_abs_a;
}

#endif
)CL"};

#endif //FRACTALEXPLORER_CLC_DEFINITIONS_HPP
