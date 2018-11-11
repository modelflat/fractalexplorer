#include "ComputableImageWidget.hpp"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QStackedLayout>
#include <QPainter>
#include <QOpenGLVertexArrayObject>

#include <utility>

#include "glsl/GLSL_Sources.hpp"

LOGGER()

#define GLERR     logger->info(fmt::format("LINE {} : {}", __LINE__, gl->glGetError()));

GLuint createVBO(QOpenGLFunctions* gl, size_t size, const float vertexData[]) {
    GLuint vbo;
    gl->glGenBuffers(1, &vbo);
    gl->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    gl->glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertexData, GL_DYNAMIC_DRAW);
    return vbo;
}

ComputableImageWidget2D::ComputableImageWidget2D(
    OpenCLBackendPtr backend,
    KernelArgConfigurationStoragePtr<UIProperties> confStorage,
    Dim_2D::RangeType dim,
    KernelId kernelId,
    QWidget *parent)
    : QOpenGLWidget(parent),
      backend_(backend), confStorage_(std::move(confStorage)),
      OpenCLComputableImage<Dim_2D>(backend, dim),
      kernelId_(std::move(kernelId)),
      size_(dim) {
    logger->info(
        fmt::format("Created new ComputableImageWidget2D for displaying KernelId {},{}",
            kernelId_.src, kernelId_.settings
            ));

    setBaseSize(static_cast<int>(dim[0]), static_cast<int>(dim[1]));
    setMinimumSize(baseSize());

    connect(this, &ComputableImageWidget2D::computed, [this](){
        repaint();
    });

    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
}

void ComputableImageWidget2D::initializeGL() {
    auto* gl = QOpenGLContext::currentContext()->functions();
    logger->info(fmt::format("GL version {}", gl->glGetString(GL_VERSION)));

    gl->glClearColor(0.5, 0.5, 0.5, 1.0);

    static const float vertexData[] = {
        // texture vertex coords: x, y
        1.0f, 1.0f,
        1.0f, -1.0f,
        -1.0f, -1.0f,
        -1.0f, 1.0f,
        // texture UV coords (for texture mapping
        1.f, 1.f,
        1.f, 0.f,
        0.f, 0.f,
        0.f, 1.f,
    };

//    GLuint vao = 0;
//    gl->glGenVertexArrays( 1, &vao );
//    gl->glBindVertexArray( vao );

    pixelStorage_.resize(size_[0] * size_[1]);

    static QOpenGLVertexArrayObject vao; vao.create();
    vao.bind();
    {
//        gl->glEnable(GL_TEXTURE_2D);
        gl->glGenTextures(1, &texture);
        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, texture);
        {
            gl->glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA, size_[0], size_[1], 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV,
                nullptr);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
        }

        program.create();
        program.addShaderFromSourceCode(QOpenGLShader::Vertex, getShaderSource("render.vert").data());
        program.addShaderFromSourceCode(QOpenGLShader::Fragment, getShaderSource("render.frag").data());
        program.link();
        logger->info(fmt::format("LOG: \"{}\"", program.log().toStdString()));
        program.bind();
        {
            int textureLocation = program.uniformLocation("tex");
            gl->glUniform1i(textureLocation, 0);
            program.enableAttributeArray(textureLocation);
        }

        // create VBO containing draw information
        vertexBuffer = createVBO(gl, sizeof(vertexData) / sizeof(float), vertexData);
        // set buffer attribs
        gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        {
            gl->glEnableVertexAttribArray(0);
            // tell opengl that first values govern vertices positions (see vertexShader)
            gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

            gl->glEnableVertexAttribArray(1);
            // tell opengl that remaining values govern fragment positions (see vertexShader)
            gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vertexData + 8);
        }
    }
    GLERR
}

void ComputableImageWidget2D::resizeGL(int w, int h) {
    auto* gl = QOpenGLContext::currentContext()->functions();
    logger->info("ResizeGL called!");
    gl->glViewport(0, 0, w, h);
}

void ComputableImageWidget2D::paintGL() {
    auto* gl = QOpenGLContext::currentContext()->functions();
    logger->info("PaintGL called!");

//    QPainter painter(this);
//    painter.drawRect(QRect( 43, 42, 29, 299));
//
//    QImage img;
//    img.loadFromData((unsigned char*)pixelStorage_.data(), pixelStorage_.size() * sizeof(unsigned int), "bmp");
//    painter.drawImage(QPoint(43, 45), img);

    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, texture);
    {
        gl->glTexSubImage2D(
            GL_TEXTURE_2D, 0,
            0, 0, size_[0], size_[1],
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV,
            pixelStorage_.data()
        );
    }

    program.bind();
    {
        gl->glBindTexture(GL_TEXTURE_2D, texture);
        gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        {
            gl->glDrawArrays(GL_TRIANGLES, 0, 4);
        }
    }
    GLERR
}

void ComputableImageWidget2D::compute(KernelArgs args) {
    logger->info(fmt::format("Computing image [{},{}]", kernelId_.src, kernelId_.settings));
    // TODO add timing
    OpenCLComputableImage<Dim_2D>::clear(backend_);
    OpenCLComputableImage<Dim_2D>::compute<UIProperties>(backend_, kernelId_, args);

    bool hasInterop = false;
    if (!hasInterop) {
        // TODO implement interop case
        cl::size_t<3> origin, region = size_.makeRegion();
        backend_->currentQueue().enqueueReadImage(image(), CL_TRUE, origin, region, 0, 0, pixelStorage_.data());
        logger->info(fmt::format("NON ZERO : {}", std::count_if(pixelStorage_.begin(), pixelStorage_.end(), [](auto el) { return el != 0; })));
    }

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
