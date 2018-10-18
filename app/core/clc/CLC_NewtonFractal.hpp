#ifndef FRACTALEXPLORER_CLC_NEWTONFRACTAL_HPP
#define FRACTALEXPLORER_CLC_NEWTONFRACTAL_HPP

#include <string_view>

static constexpr std::string_view NEWTON_FRACTAL_3EQ_SOURCE { R"CL(

// Here:
// a = a * (-3) / (3 - t);
// b = 0, 0
// c = C * (-t) / (3 - t);
// z**3 + a*z**2 + c = 0
// NOTE: value -1 for root is not supported!!!
int solve_cubic_newton_fractal_optimized(real2 a, real2 c, real precision, int root, real2* roots) {
    real ax = a.x;
    real ax2 = ax * ax;
    real ay = a.y;
    real ay2 = ay * ay;
    real cx = c.x;
    real cy = c.y;

    real2 D = {
        (cx*cx - cy*cy) / 4.0f + (cx*ax*(ax2 - 3*ay2) - cy*ay*(3*ax2 - ay2)) / 27.0f,
        cx*cy / 2.0f + (cx*ay*(3*ax2 - ay2) + cy*ax*(ax2 - 3*ay2)) / 27.0f
    };
    real modulus = length(D);
    real2 first_root_of_D = {
        sqrt((modulus + D.x) / 2.0f),
        sign(D.y) * sqrt((modulus - D.x) / 2.0f)
    };

    real2 base_alpha = {
        ax * (3*ay2 - ax2),
        ay * (ay2 - 3*ax2)
    };

    base_alpha /= 27.0f;
    base_alpha -= c / 2.0f;

    real2 base_beta = base_alpha;
    base_alpha -= first_root_of_D;
    base_beta += first_root_of_D;

    // ====================
    real2 alpha[3];
    complex_cbrt(base_alpha, alpha);
    real2 beta[3];
    complex_cbrt(base_beta, beta);
    // now we have alpha & beta values, lets combine them such as:
    // alpha[0]*beta[i] = -p / 3;
    int idx = -1;
    real2 r;
    for (int i = 0; i < 3; ++i) {
        r.x = alpha[0].x * beta[i].x - alpha[0].y * beta[i].y + (ay2 - ax2) / 9.0f;
        r.y = alpha[0].y * beta[i].x + alpha[0].x * beta[i].y - 2*ax*ay / 9.0f;
        if (fabs(r.x) < precision && fabs(r.y) < precision) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        // cannot find b, such that alpha*beta = p/3 (although it should always exists);
        // maybe floating point error is greater than precision, but better not to risk and report error
        return 1;
    }
    // now corresponding beta is found for alpha[0]. other betas can be inferred from it
    if (root == 0) {
        // alpha_1 + beta_1 - a/3
        roots[0] = alpha[0] + beta[idx] - a / 3;
    }
    if (root == 1) {
        // alpha_2 + beta_3 - a/3
        roots[1].x = alpha[1].x - ax / 3 +
            COMPL_ONE_3rd_ROOT_REAL*beta[idx].x - COMPL_ONE_3rd_ROOT_IMAG*beta[idx].y;
        roots[1].y = alpha[1].y - ay / 3 +
            COMPL_ONE_3rd_ROOT_REAL*beta[idx].y + COMPL_ONE_3rd_ROOT_IMAG*beta[idx].x;
    }
    if (root == 2) {
        // alpha_3 + beta_2 - a/3
        roots[2].x = alpha[2].x - ax / 3 +
            COMPL_ONE_2nd_ROOT_REAL*beta[idx].x - COMPL_ONE_2nd_ROOT_IMAG*beta[idx].y;
        roots[2].y = alpha[2].y - ay / 3 +
            COMPL_ONE_2nd_ROOT_REAL*beta[idx].y + COMPL_ONE_2nd_ROOT_IMAG*beta[idx].x;
    }
    return 0;
}
)CL" };

static constexpr std::string_view NEWTON_FRACTAL_SOURCE { R"CL(

#if (defined(cl_khr_fp64) && defined(USE_DOUBLE_PRECISION))
    #define PRECISION 1e-9
#else
    #define PRECISION 1e-4
#endif

#define DYNAMIC_COLOR 0

float3 hsv2rgb(float3 hsv) {
    const float c = hsv.y * hsv.z;
    const float x = c * (1 - fabs(fmod( hsv.x / 60, 2 ) - 1));
    float3 rgb;
    if      (0 <= hsv.x && hsv.x < 60) {
        rgb = (float3)(c, x, 0);
    } else if (60 <= hsv.x && hsv.x < 120) {
        rgb = (float3)(x, c, 0);
    } else if (120 <= hsv.x && hsv.x < 180) {
        rgb = (float3)(0, c, x);
    } else if (180 <= hsv.x && hsv.x < 240) {
        rgb = (float3)(0, x, c);
    } else if (240 <= hsv.x && hsv.x < 300) {
        rgb = (float3)(x, 0, c);
    } else {
        rgb = (float3)(c, 0, x);
    }
    return (rgb + (hsv.z - c)); //* 255;
}

// Draws newton fractal
kernel void newton_fractal(
    // plane bounds
    real min_x, real max_x, real min_y, real max_y,
    // fractal parameters
    global real* C_const, int backward, int t, real h,
    // how many initial points select
    uint runs_count,
    // how many times solve equation for certain initial point
    uint points_count,
    // how many initial steps will be skipped
    uint iter_skip,
    // seed - seed value for pRNG; see "random.clh"
    ulong seed,
    // color. this color will only be used as static!
    global float4* color_in,
    // image buffer for output
    write_only image2d_t out_image)
{
    // const C
    const real2 C = {C_const[0], C_const[1]};
    // color
    #if (DYNAMIC_COLOR)
        float4 color = {fabs(sin(PI * h / 3.)), fabs(cos(PI * h / 3.)), 0.0, 0.0};
        float3 color_hsv = { 0.0, 1.0, 1.0 };
    #else
        float4 color = {0.0, 0.0, 0.0, 1.0};
    #endif
    // initialize pRNG
    uint2 rng_state;
    init_state(seed, &rng_state);
    // spans, scales, sizes
    real span_x = max_x - min_x;
    real span_y = max_y - min_y;
    int image_width = get_image_width(out_image);
    int image_height = get_image_height(out_image);
    real scale_x = span_x / image_width;
    real scale_y = span_y / image_height;
    int2 coord;
    // for each run
    real2 roots[3];
    real2 a;
    const real2 c = -C * h * t / (3 - t * h); // t sign switches between Explicit and Implicit Euler method
    const real a_modifier = -3 / (3 - t * h);
    const real max_distance_from_prev = length((real2)(max_x - min_x, max_y - min_y));
    real total_distance = 0.0;
    // TODO run count was proved to be inefficient. remove?
    for (int run = 0; run < runs_count; ++run) {
        // choose starting point
        real2 starting_point = {
            ((random(&rng_state)) * span_x + min_x) / 2,
            ((random(&rng_state)) * span_y + min_y) / 2
        };
        uint is = iter_skip;
        int frozen = 0;

        // iterate through solutions of cubic equation
        for (int i = 0; i < points_count; ++i) {
            // compute next point:
            uint root_number = (as_uint(random(&rng_state)) >> 7) % 3;
            if (backward) {
                a = starting_point * a_modifier;
                solve_cubic_newton_fractal_optimized(a, c, 1e-8, root_number, roots);

                real distance_from_prev = length(starting_point - roots[root_number]);
                total_distance += distance_from_prev;

                starting_point = roots[root_number];
            } else {
                a = starting_point;
                real2 last = { (a.x*a.x - a.y*a.y), -2*a.x*a.y };
                last /= (a.x*a.x*a.x*a.x + a.y*a.y*a.y*a.y + 2*a.x*a.x*a.y*a.y);
                real2 last_mul_C = { last.x*C.x - last.y*C.y, (last.x*C.y + last.y*C.x) };

                real distance_from_prev = length(starting_point - a);
                total_distance += distance_from_prev;

                starting_point = a - h / 3.0 * a - h*last_mul_C / 3.0;
            }
            // the first iter_skip points will  be skipped
            if (is == 0) {
                // transform coords:
                coord.x = (starting_point.x - min_x) / scale_x;
                coord.y = image_height - 1 - (int)((starting_point.y - min_y) / scale_y);

                // draw next point:
                #if (DYNAMIC_COLOR)
                    //(1 - distance_from_prev / max_distance_from_prev)
                    if (backward) {
                        color_hsv.x = convert_float(360.0 * (root_number / 3.0));
                    } else {
                        color_hsv.x = convert_float(360.0 * sin(total_distance));
                    }
                    color_hsv.y = convert_float(1.0 * ((float)(i) / (points_count - iter_skip)));
                    color_hsv.z = convert_float(1.0 * (total_distance / (max_distance_from_prev * (points_count - iter_skip))));
                #endif
                if (coord.x < image_width && coord.y < image_height && coord.x >= 0 && coord.y >= 0) {
                    #if DYNAMIC_COLOR
                        write_imagef(out_image, coord, (float4)(hsv2rgb( color_hsv ), 1.0));
                    #else
                        write_imagef(out_image, coord, color);
                    #endif
                    frozen = 0;
                } else {
                    if (++frozen > 15) {
                        // this generally means that solution is going to approach infinity
                        //printf("[OCL] error at slave %d: frozen!\n", get_global_id(0));
                        break;
                    }
                }
            } else {
                --is;
            }
        }
    }
}

)CL" };

#endif //FRACTALEXPLORER_CLC_NEWTONFRACTAL_HPP
