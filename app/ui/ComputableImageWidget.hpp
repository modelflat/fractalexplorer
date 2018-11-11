#ifndef FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP
#define FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP

#include <QWidget>
#include <QOpenGLWidget>
#include <QHBoxLayout>

#include "ComputableImage.hpp"
#include "KernelArgWidget.hpp"


class ComputableImageWidget : public QOpenGLWidget {

    Q_OBJECT

public:

    ComputableImageWidget(
        OpenCLBackendPtr backend,
        KernelArgConfigurationStoragePtr<UIProperties> confStorage,
        QWidget* parent = nullptr);

protected:

    OpenCLBackendPtr backend_;
    KernelArgConfigurationStoragePtr<UIProperties> confStorage_;

    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

};

class ComputableImageWidget2D : public ComputableImageWidget, private OpenCLComputableImage<Dim_2D> {

    Q_OBJECT

    const KernelId kernelId_;

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
    ) : QWidget(parent),
        kernel_(backend->compileKernel<UIProperties>({ "newton_fractal", "default" })) {
        image = new ComputableImageWidget2D(backend, confStorage, size, id);
        args = makeParameterWidgetForKernel(id, kernel_.kernel(), confStorage);

        auto* layout = new QHBoxLayout;
        layout->addWidget(image);
        layout->addWidget(args);

        connect(args, &KernelArgWidget::valuesChanged, image, &ComputableImageWidget2D::compute);

        this->setLayout(layout);
    }

};

#endif //FRACTALEXPLORER_COMPUTABLEIMAGEWIDGET_HPP
