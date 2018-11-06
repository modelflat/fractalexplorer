#ifndef FRACTALEXPLORER_COMPUTABLEIMAGE_HPP
#define FRACTALEXPLORER_COMPUTABLEIMAGE_HPP

#include "OpenCLBackend.hpp"

using Color = cl_float4;

template <size_t Dim>
class Range {
    cl::size_t<Dim> sz_;

public:

    template <
            typename = std::enable_if_t<Dim == 2ull>
            >
    Range(size_t w, size_t h) : sz_() {
        sz_[0] = w;
        sz_[1] = h;
    }

    template <
            typename = std::enable_if_t<Dim == 3ull>
            >
    Range(size_t w, size_t h, size_t d) : Range(w, h) {
        sz_[2] = d;
    }

    inline size_t operator[](size_t i) const noexcept {
        return i < Dim ? sz_[static_cast<int>(i)] : 0ull;
    }

    cl::size_t<3> makeRegion() const noexcept {
        cl::size_t<3> r;
        r[0] = sz_[0];
        r[1] = sz_[1];
        if constexpr (Dim == 2) {
            r[2] = 1;
        } else {
            r[2] = sz_[2];
        }
        return r;
    }
};

struct Dim_2D {
    static constexpr size_t N = 2;

    typedef Range<N> RangeType;

    typedef cl::Image2D ImageType;

    static ImageType createImage(cl::Context ctx, RangeType dim) {
        return ImageType { ctx, CL_MEM_READ_WRITE, { CL_RGBA, CL_UNORM_INT8 }, dim[0], dim[1] };
    }

    static void enqueueKernel(cl::CommandQueue queue, cl::Kernel kernel, cl::NDRange localRange, RangeType dim) {
        queue.enqueueNDRangeKernel(kernel, {0, 0}, {dim[0], dim[1]}, localRange);
    }
};

struct Dim_3D {

};

template <typename DimensionPolicy>
class OpenCLComputableImage {
public:
    using RangeType = typename DimensionPolicy::RangeType;
    using ImageType = typename DimensionPolicy::ImageType;

    OpenCLComputableImage(OpenCLBackendPtr backend, RangeType dimensions) : dimensions_(dimensions) {
        recreateImageIfNeeded(backend, dimensions);
    }

    template <typename KernelInstanceProperties>
    void compute(OpenCLBackendPtr backend, KernelId id, const KernelArgs& args) {
        auto queue = backend->currentQueue();
        auto compiled = backend->compileKernel<KernelInstanceProperties>(id);
//        auto localRange = backend->findKernelBase(id).localRange;
        auto localRange = cl::NDRange {};

        recreateImageIfNeeded(backend, dimensions_);

        compiled.kernel().setArg(compiled.imageArg(), image_);

        if (DimensionPolicy::N >= compiled.dimensionalArgs().size()) {
            throw std::runtime_error("Image is of higher dimensionality than expected");
        }

        for (size_t i = 0; i < DimensionPolicy::N; ++i) {
            compiled.kernel().setArg(compiled.dimensionalArgs()[i], dimensions_[i]);
        }

        applyArgsToKernel(compiled.kernel(), args.begin(), args.end());

        DimensionPolicy::enqueueKernel(backend->currentQueue(), compiled.kernel(), localRange, dimensions_);
    }

    /**
     * Clears this image with specified color.
     */
    void clear(OpenCLBackendPtr backend, Color color={1.0f, 1.0f, 1.0f, 1.0f}) {
        recreateImageIfNeeded(backend, dimensions_);
        cl::size_t<3> origin, region = dimensions_.makeRegion();
        backend->currentQueue().enqueueFillImage(image_, color, origin, region);
    }

private:

    void recreateImageIfNeeded(OpenCLBackendPtr backend, RangeType dimensions) {
        if (image_() == NULL || memoryBelongsToContext(image_, backend->currentContext())) {
            image_ = DimensionPolicy::createImage(backend->currentContext(), dimensions);
        }
    }

    RangeType dimensions_;
    ImageType image_;

};

#endif //FRACTALEXPLORER_COMPUTABLEIMAGE_HPP
