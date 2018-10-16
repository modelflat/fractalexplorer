#ifndef FRACTALEXPLORER_CLC_RANDOM_HPP
#define FRACTALEXPLORER_CLC_RANDOM_HPP

#include <string_view>

static constexpr std::string_view RANDOM_SOURCE { R"CL(

#ifndef real
    #error "real" type is not defined. You probably have your includes in wrong order.
#endif

#ifndef __RANDOM_CLH
#define __RANDOM_CLH

// source code:
// http://cas.ee.ic.ac.uk/people/dt10/research/rngs-gpu-mwc64x.html
uint MWC64X(uint2 *state) {
    enum { A = 4294883355U };
    uint x = (*state).x, c = (*state).y;    // Unpack the state
    uint res = x ^ c;                       // Calculate the result
    uint hi = mul_hi(x, A);                 // Step the RNG
    x = x * A + c;
    c = hi + (x < c);
    *state = (uint2)(x, c);                 // Pack the state back up
    return res;                             // Return the next result
}

// ! this rng does not take into account local id, but it can
// be mixed in one of seed values if needed
void init_state(ulong seed, uint2 *state) {
    int id = get_global_id(0) + 1;
    uint2 s = as_uint2(seed);
    (*state) = (uint2)(
        // create a mixture of id and two seeds
        (id + s.x & 0xFFFF) * s.y,
        (id ^ (s.y & 0xFFFF0000)) ^ s.x
    );
}

// retrieve random REAL in range [0.0; 1.0] (both inclusive)
real random(uint2 *state) {
    return ((real)MWC64X(state)) / (real)0xffffffff;
}

#endif
)CL" };

#endif //FRACTALEXPLORER_CLC_RANDOM_HPP
