#ifndef FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP
#define FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP

#include <QWidget>
#include <QOpenGLWidget>
#include <QHBoxLayout>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

#include "ComputableImage.hpp"
#include "KernelArgWidget.hpp"

class ComputableImageWidget2D : public QOpenGLWidget, private OpenCLComputableImage<Dim_2D> {

    Q_OBJECT

    OpenCLBackendPtr backend_;
    KernelArgConfigurationStoragePtr<UIProperties> confStorage_;
    const KernelId kernelId_;

    QOpenGLShaderProgram program;
    GLuint vertexBuffer, texture;
    Range<2> size_;

    std::vector<GLuint> pixelStorage_;

public:

    ComputableImageWidget2D(
        OpenCLBackendPtr backend,
        KernelArgConfigurationStoragePtr<UIProperties> confStorage,
        Dim_2D::RangeType dim,
        KernelId kernelId,
        QWidget* parent = nullptr);

public slots:

    /**
     * Compute this image in blocking manner.
     */
    void compute(KernelArgs);

signals:

    void computed();

protected:

    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

};

class ParameterizedComputableImageWidget : public QWidget {

    Q_OBJECT

    ComputableImageWidget2D* image;
    KernelArgWidget* args;

    KernelInstance<UIProperties> kernel_;

public:

    ParameterizedComputableImageWidget(
        OpenCLBackendPtr backend,
        KernelArgConfigurationStoragePtr<UIProperties> confStorage,
        Range<2> size,
        KernelId id,
        QWidget* parent = nullptr
    );

};

#endif //FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP
