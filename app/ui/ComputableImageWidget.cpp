#include "ComputableImageWidget.hpp"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QStackedLayout>
#include <utility>

LOGGER()

ComputableImageWidget::ComputableImageWidget(
    OpenCLBackendPtr backend,
    KernelArgConfigurationStoragePtr<UIProperties> confStorage,
    QWidget *parent)
: QOpenGLWidget(parent), backend_(std::move(backend)), confStorage_(std::move(confStorage))
{
    logger->info("creating CIW");
}

void ComputableImageWidget::initializeGL() {
    logger->info("InitializeGL called!");
    auto* gl = QOpenGLContext::currentContext()->functions();
    gl->glClearColor(1.0, 0, 0, 0);
    logger->info(fmt::format("GL version {}", gl->glGetString(GL_VERSION)));
}

void ComputableImageWidget::resizeGL(int w, int h) {
    auto* gl = QOpenGLContext::currentContext()->functions();
    logger->info("ResizeGL called!");
    gl->glViewport(0, 0, w, h);
}

void ComputableImageWidget::paintGL() {
    auto* gl = QOpenGLContext::currentContext()->functions();
    logger->info("PaintGL called!");
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

    setBaseSize(static_cast<int>(dim[0]), static_cast<int>(dim[1]));
    setMinimumSize(baseSize());

    connect(this, &ComputableImageWidget2D::computed, [this](){
        ComputableImageWidget::repaint();
    });
}

void ComputableImageWidget2D::compute(KernelArgs args) {
    logger->info(fmt::format("Computing image [{},{}]", kernelId_.src, kernelId_.settings));
    // TODO add timing
    OpenCLComputableImage<Dim_2D>::compute<UIProperties>(backend_, kernelId_, args);

    logger->info(fmt::format("Image [{},{}] computed", kernelId_.src, kernelId_.settings));
    emit computed();
}

ParameterizedComputableImageWidget::ParameterizedComputableImageWidget(OpenCLBackendPtr backend,
    KernelArgConfigurationStoragePtr<UIProperties> confStorage, Range<2> size, KernelId id, QWidget *parent)
: QWidget(parent),
  kernel_(backend->compileKernel<UIProperties>(id)),
  image(new ComputableImageWidget2D(backend, confStorage, size, id)) {
    args = makeParameterWidgetForKernel(id, kernel_.kernel(), confStorage);

    auto* layout = new QVBoxLayout;

    auto* settingsLayout = new QHBoxLayout;
    auto* settings = new QPushButton("Parameters");
    settingsLayout->addWidget(settings);
    auto* sizeParameters = new QPushButton("Size");
    settingsLayout->addWidget(sizeParameters);
    auto* resetParameters = new QPushButton("Reset");
    settingsLayout->addWidget(resetParameters);

    layout->addLayout(settingsLayout);
    args->setWindowFlags(Qt::WindowStaysOnTopHint);
    args->setMinimumWidth(250); // TODO depend on slider constratints / be configurable

    connect(settings, &QPushButton::clicked, [this](auto checked) {
        args->setVisible(!args->isVisible());
    });

    connect(args, &KernelArgWidget::valuesChanged, image, &ComputableImageWidget2D::compute);

    layout->addWidget(image);

    layout->setMargin(1);
    layout->setSpacing(1);

    this->setLayout(layout);
}
