#ifndef FRACTALEXPLORER_OPENCLKERNELUTILS_HPP
#define FRACTALEXPLORER_OPENCLKERNELUTILS_HPP

#include <OpenCL.hpp>

#include <variant>
#include <utility>
#include <unordered_map>

/**
 * Type to actually store arg value in runtime.
 */
using AnyType = std::variant<
    cl_int,
    cl_long,
    cl_float,
    cl_double,
    cl_float2,
    cl_double2,
    cl_float3,
    cl_double3
>;

/**
 * Sets kernel argument at specified index to value of given type.
 */
void setArg(cl::Kernel &kernel, cl_uint idx, const AnyType &value);

inline bool operator<=(const cl_float2 &l, const cl_float2 &r) {
    return l.x <= r.x && l.y <= r.y;
}

inline bool operator<=(const cl_double2 &l, const cl_double2 &r) {
    return l.x <= r.x && l.y <= r.y;
}

inline bool operator<=(const cl_float3 &l, const cl_float3 &r) {
    return l.x <= r.x && l.y <= r.y && l.z <= r.z;
}

inline bool operator<=(const cl_double3 &l, const cl_double3 &r) {
    return l.x <= r.x && l.y <= r.y && l.z <= r.z;
}

/**
 * Kernel parameter which could be passed around by value.
 */
struct KernelArgValue {

    const AnyType value;

    KernelArgValue(bool v)
        : value(std::in_place_type<cl_int>, static_cast<cl_int>(v ? 1 : 0)) {}

    KernelArgValue(int32_t v)
        : value(std::in_place_type<cl_int>, v) {}

    KernelArgValue(int64_t v)
        : value(std::in_place_type<cl_long>, v) {}

    KernelArgValue(float v)
        : value(std::in_place_type<cl_float>, v) {}

    KernelArgValue(double v)
        : value(std::in_place_type<cl_double>, v) {}

    KernelArgValue(float vx, float vy)
        : value(std::in_place_type<cl_float2>, cl_float2{vx, vy}) {}

    KernelArgValue(double vx, double vy)
        : value(std::in_place_type<cl_double2>, cl_double2{vx, vy}) {}

    KernelArgValue(float vx, float vy, float vz)
        : value(std::in_place_type<cl_float3>, cl_float3{vx, vy, vz}) {}

    KernelArgValue(double vx, double vy, double vz)
        : value(std::in_place_type<cl_double3>, cl_double3{vx, vy, vz}) {}

    /**
     * Apply this value to a kernel argument at specified index.
     */
    inline void apply(cl::Kernel &kernel, cl_uint idx) const {
        setArg(kernel, idx, value);
    };

    /**
     * Checks if this value lies between min and max.
     */
    inline bool between(const KernelArgValue &min, const KernelArgValue &max) const {
        return min.value <= value && value <= max.value;
    }

};

/**
 * Map of argument names to their indices.
 */
using ArgNameMap = std::unordered_map<std::string, cl_uint>;

/**
 * Creates map argName => argIndex for given kernel object
 */
ArgNameMap mapNamesToArgIndices(const cl::Kernel &kernel);

/**
 * Vector of arguments.
 */
using KernelArgs = std::unordered_map<std::variant<std::string, size_t>, KernelArgValue>;

/**
 * Apply arguments to kernel.
 */
void applyArgsToKernel(cl::Kernel &kernel, const KernelArgs &args);

/**
 * Kernel settings. Act like kernel meta-info: sources, compileOptions etc.
 */
struct KernelSettings {
    const cl::Program::Sources sourceCode;
    const std::vector<std::string> compileOptions;
    const cl::NDRange localRange = cl::NullRange;
};

#endif //FRACTALEXPLORER_OPENCLKERNELUTILS_HPP
