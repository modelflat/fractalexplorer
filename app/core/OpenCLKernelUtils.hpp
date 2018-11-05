#ifndef FRACTALEXPLORER_OPENCLKERNELUTILS_HPP
#define FRACTALEXPLORER_OPENCLKERNELUTILS_HPP

#include <OpenCL.hpp>
#include <Utility.hpp>

#include <variant>
#include <utility>
#include <unordered_map>
#include <optional>
#include <functional>

std::string to_string(const cl::Kernel& kernel);

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
 * Most general type among possible primitives in AnyType.
 */
using Primitive = double;

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

    AnyType value;

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

    KernelArgValue(cl_float2 v)
        : value(std::in_place_type<cl_float2>, v) {}

    KernelArgValue(double vx, double vy)
        : value(std::in_place_type<cl_double2>, cl_double2{vx, vy}) {}

    KernelArgValue(cl_double2 v)
        : value(std::in_place_type<cl_double2>, v) {}

    KernelArgValue(float vx, float vy, float vz)
        : value(std::in_place_type<cl_float3>, cl_float3{vx, vy, vz}) {}

    KernelArgValue(cl_float3 v)
        : value(std::in_place_type<cl_float3>, v) {}

    KernelArgValue(double vx, double vy, double vz)
        : value(std::in_place_type<cl_double3>, cl_double3{vx, vy, vz}) {}

    KernelArgValue(cl_double3 v)
        : value(std::in_place_type<cl_double3>, v) {}

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

template <typename Iter>
void applyArgsToKernel(cl::Kernel &kernel, Iter begin, Iter end) {
    auto nameMap = mapNamesToArgIndices(kernel);
    std::for_each(
        begin, end, [&](const auto &value) {
            std::visit(
                [&](auto &&arg) {
                    using Type = decltype(arg);
                    if constexpr (std::is_same_v<Type, std::string>) {
                        auto val = nameMap.find(arg);
                        if (val == nameMap.end()) {
                            throw std::invalid_argument(fmt::format("Kernel has no argument named \"{}\"", arg));
                        }
                        value.second.apply(kernel, val->second);
                    } else if constexpr (std::is_same_v<Type, size_t>) {
                        value.second.apply(kernel, static_cast<cl_uint>(arg));
                    }
                }, value.first);
        });
}

/**
 * Type enum for runtime type detection in kernels.
 */
enum class KernelArgType {
    Unknown         = 0,
    Int32           = 1,
    Int64           = 2,
    Float32         = 3,
    Float64         = 4,
    Vector2Float32  = 5,
    Vector2Float64  = 6,
    Vector3Float32  = 7,
    Vector3Float64  = 8,
    Image           = 9,
    Buffer          = 10,
};

struct KernelArgTypeTraits {
    using VectorConverter = std::function<KernelArgValue(const std::vector<Primitive>&)>;

    const size_t numComponents;
    const VectorConverter fromVector;
};

using ArgsTypesWithNames = std::vector<std::pair<KernelArgType, std::string>>;

/**
 * Detects argument types and names for given kernel.
 */
ArgsTypesWithNames detectArgumentTypesAndNames(const cl::Kernel &kernel);

/**
 * A set of properties from which a kernel instance can be built.
 */
class KernelBase {
    cl::Program::Sources sourceCode_;
    std::vector<std::string> compileOptions_;

public:

    KernelBase(
        cl::Program::Sources sourceCode,
        std::vector<std::string> compileOptions
        );

    inline const auto& sources() const { return sourceCode_; }

    inline void sources(cl::Program::Sources src) { sourceCode_ = std::move(src); }

    inline const auto& options() const { return compileOptions_; }

    inline void options(std::vector<std::string> opt) { compileOptions_ = std::move(opt); }

    cl::Program build(const cl::Context&) const;
};

/**
 * Semantic argument properties. Consist of basic properties (default value, min, max) and UserProperties,
 * which user may supply to provide additional logic to argument value handling.
 */
template <typename UserProperties>
class ArgProperties {
    KernelArgValue defaultVal_, min_, max_;
    UserProperties props_;

public:
    ArgProperties(KernelArgValue defaultVal_, KernelArgValue min_, KernelArgValue max_, const UserProperties& props_)
    : defaultVal_(defaultVal_), min_(min_), max_(max_), props_(props_)
    {}

    ArgProperties(KernelArgValue defaultVal_, KernelArgValue min_, KernelArgValue max_, UserProperties &&props_)
        : defaultVal_(defaultVal_), min_(min_), max_(max_), props_(std::forward<UserProperties>(props_))
    {}

    inline KernelArgValue defaultValue() const { return defaultVal_; }

    inline void defaultValue(KernelArgValue defaultVal_) { this->defaultVal_ = defaultVal_; }

    inline KernelArgValue min() const { return min_; }

    inline void min(KernelArgValue min_) { this->min_ = min_; }

    inline KernelArgValue max() const { return max_; }

    inline void max(KernelArgValue max_) { this->max_ = max_; }

    inline const UserProperties& userProps() const { return props_; }

    inline UserProperties& userProps() { return props_; }

    inline void userProps(const UserProperties& props) { this->props_ = props; }

    inline void userProps(UserProperties&& props) { this->props_ = std::forward<UserProperties>(props); }
};

/**
 * Among argument names, try to detect an argument which may correspond to image
 */
cl_uint detectImageArgIdx(const ArgNameMap& map);

/**
 * Among argument names, try to detect arguments which may correspond to image
 * dimensions (e.g. args named "width" and "height".
 */
std::vector<cl_uint> detectImageDimensionalArgIdxs(const ArgNameMap& map);

class PropertyParsingError : public std::runtime_error {
public:
    PropertyParsingError(const std::string &_Message) : runtime_error(_Message) {}
};

/**
 * Parse basic argument properties from input stream.
 * Format: <name|index> <default> <min> <max>
 */
std::tuple<
    cl_uint, std::tuple<KernelArgValue, KernelArgValue, KernelArgValue>
> parseBasicProperties(const ArgsTypesWithNames&, std::istream&);

/**
 * Parse basic and optional user properties from input stream using user-provided logic.
 * Format: <basic-properties> [user-supplied properties]
 */
template <typename UserArgProperties>
ArgProperties<UserArgProperties> propertiesFromStream(const ArgsTypesWithNames& argDict, std::istream& str) {
    auto [index, defMinMax] = parseBasicProperties(argDict, str);
    auto [def, min, max] = defMinMax;
    return { def, min, max, UserArgProperties::fromStream(str) };
}

/**
 * Empty user properties for argument property discovery
 */
struct NoUserProperties {
    static auto fromStream(std::istream& str) { return NoUserProperties {}; }
};

size_t findArgIndex(const ArgNameMap&);

template <typename UserT = NoUserProperties>
using KernelArgProperties = std::vector<ArgProperties<UserT>>;

template <typename UserArgProperties = NoUserProperties>
KernelArgProperties<UserArgProperties> propertiesFromConfig(const ArgsTypesWithNames& argDict, std::string_view conf) {
    LOGGER();

    KernelArgProperties<UserArgProperties> result;
    result.reserve(8);

    std::istringstream ss { conf.data() };
    size_t line = 0;
    while (!ss.eof()) {
        ++line;
        try {
            result.push_back( propertiesFromStream<NoUserProperties>(argDict, ss) );
        } catch (const std::exception& e) {
            auto err = fmt::format("L{}: error during parsing: {}", line, e.what());
            logger->error(err);
            throw e;
        }
    }

    return result;
}

/**
 * Low-level cl::Kernel instance enriched by argument metainfo.
 *
 * @tparam UserArgProperties user-supplied argument properties
 */
template <typename UserArgProperties = NoUserProperties>
class KernelInstance {
    cl::Kernel kernel_;
    KernelArgProperties<UserArgProperties> argProps_;
    ArgNameMap nameMap_;

    cl_uint imageArgIdx_;
    std::vector<cl_uint> dimensionalArgs_;

public:

    KernelInstance(cl::Kernel kernel, KernelArgProperties<UserArgProperties> props)
    : kernel_(std::move(kernel)), argProps_(std::move(props)), nameMap_(mapNamesToArgIndices(kernel))
    {
        imageArgIdx_ = detectImageArgIdx(nameMap_);
        dimensionalArgs_ = detectImageDimensionalArgIdxs(nameMap_);
    }

    KernelInstance(cl::Kernel kernel) : KernelInstance(kernel, {}) {}

    inline cl::Kernel kernel() const { return kernel_; }

    inline const KernelArgProperties<UserArgProperties> &argProps() const { return argProps_; }

    inline KernelArgProperties<UserArgProperties> &argProps() { return argProps_; }

    inline void argProps(const KernelArgProperties<UserArgProperties> &argProps_) { this->argProps_ = argProps_; }

    inline void argProps(KernelArgProperties<UserArgProperties> &&argProps_) { this->argProps_ = std::move(argProps_); }

    inline const ArgNameMap& nameMap() const { return nameMap_; }

    inline cl_uint imageArg() const { return imageArgIdx_; }

    inline void imageArg(cl_uint imageArgIdx_) { this->imageArgIdx_ = imageArgIdx_; }

    inline const std::vector<cl_uint>& dimensionalArgs() const { return dimensionalArgs_; }

    inline void dimensionalArgs(std::vector<cl_uint> dimensionalArgs_) { this->dimensionalArgs_ = std::move(dimensionalArgs_); }

    inline void updateArgProperties(size_t idx, ArgProperties<UserArgProperties> newProp) {
        argProps_[idx] = newProp;
    }

    inline void updateArgProperties(std::string_view argName, ArgProperties<UserArgProperties> newProp) {
        argProps_[findArgIndex(argName, nameMap_)] = newProp;
    }

    inline ArgsTypesWithNames detectArgTypesAndNames() const {
        return detectArgumentTypesAndNames(kernel_);
    }

};

#endif //FRACTALEXPLORER_OPENCLKERNELUTILS_HPP
