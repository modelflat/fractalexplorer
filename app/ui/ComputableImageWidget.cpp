#include "ComputableImageWidget.hpp"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <utility>

LOGGER()

ComputableImageWidget::ComputableImageWidget(
    OpenCLBackendPtr backend,
    KernelArgConfigurationStoragePtr<UIProperties> confStorage,
    QWidget *parent)
: QOpenGLWidget(parent), backend_(std::move(backend)), confStorage_(std::move(confStorage))
{}

void ComputableImageWidget::initializeGL() {
    logger->info("InitializeGL called!");
    auto* gl = QOpenGLContext::currentContext()->functions();

    logger->info(fmt::format("GL version {}", gl->glGetString(GL_VERSION)));
}

void ComputableImageWidget::resizeGL(int w, int h) {
    auto* gl = QOpenGLContext::currentContext()->functions();
    logger->info("ResizeGL called!");
}

void ComputableImageWidget::paintGL() {
    auto* gl = QOpenGLContext::currentContext()->functions();
    logger->info("PaintGL called!");
}

ComputableImageWidget2D::ComputableImageWidget2D(
    OpenCLBackendPtr backend,
    KernelArgConfigurationStoragePtr<UIProperties> confStorage,
    Dim_2D::RangeType dim,
    KernelId kernelId,
    QWidget *parent)
: ComputableImageWidget(backend, std::move(confStorage), parent),
  OpenCLComputableImage<Dim_2D>(backend, dim),
  kernelId_(std::move(kernelId)) {

    logger->info(fmt::format("Created new ComputableImageWidget2D for displaying KernelId {},{}",
        kernelId_.src, kernelId_.settings)
    );
    
}

void ComputableImageWidget2D::compute(KernelArgs args) {
    logger->info(fmt::format("Computing image [{},{}]", kernelId_.src, kernelId_.settings));
    // TODO add timing
    OpenCLComputableImage<Dim_2D>::compute<UIProperties>(backend_, kernelId_, args);

    logger->info(fmt::format("Image [{},{}] computed", kernelId_.src, kernelId_.settings));
//    emit computed();
}


