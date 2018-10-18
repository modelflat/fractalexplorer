#ifndef FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP
#define FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP

#include <QtWidgets/QWidget>
#include <QtWidgets/QOpenGLWidget>

#include "ComputableImage.hpp"

struct ArgSpecification {
    std::string name;
    std::string description;
    KernelArgType argType;
};

class ComputableImageWidget : public QOpenGLWidget {

public:

    ComputableImageWidget(QWidget* parent = nullptr) : QOpenGLWidget(parent) {}

public slots:

    /**
     * Set parameters for underlying OpenCL kernel.
     */
    void setParameters(const std::vector<ArgSpecification>& params);

    /**
     * Run an OpenCL job to (re)compute this image.
     */
    void compute();

protected:

    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

};

class ComputableImageWidget2D : public ComputableImageWidget, private OpenCLComputableImage<Dim_2D> {

    const KernelId kernelId_;

public:

    ComputableImageWidget2D(OpenCLBackendPtr backend, Dim_2D::RangeType dim, KernelId kernelId, QWidget* parent = nullptr)
    : ComputableImageWidget(parent), OpenCLComputableImage(backend, dim), kernelId_(kernelId)
    {}

};

class ComputableImageWidget3D : public ComputableImageWidget, private OpenCLComputableImage<Dim_3D> {

};

#endif //FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP
