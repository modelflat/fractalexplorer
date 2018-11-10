#include "KernelArgWidget.hpp"
#include "app/core/Utility.hpp"

#include <QVBoxLayout>

LOGGER()

template <typename Flt>
std::tuple<int, int, int> interpolate(Flt val, Flt min, Flt max, double granularity) {
    int intMin = static_cast<int>(std::floor(min / granularity));
    int intSize = static_cast<int>(std::floor(max / granularity));
    int intVal = static_cast<int>(std::floor(val / granularity));
    return { intMin, intSize, intVal };
}

static constexpr double granularity = 1e-2f;

template <typename T = Primitive>
T getAndUnpackVectorComponent(size_t idx, AnyType val) {
    return std::visit([](auto val){
        return static_cast<T>(val);
    }, getVectorComponent(idx, val));
}

Slider::Slider(KernelArgType type, KernelArgValue min, KernelArgValue max, KernelArgValue def, QWidget *parent)
: ArgProviderWidget(parent), type_(type) {
    auto traits = findTypeTraits(type);
    sliders_ = std::vector<QSlider *>(traits.numComponents);

    auto* lay = new QVBoxLayout;

    for (size_t i = 0; i < sliders_.size(); ++i) {
        auto *sl = new QSlider(Qt::Orientation::Horizontal);

        if (traits.klass == KernelArgTypeClass::Integer) {
            // put an argument as is
            sl->setMinimum(getAndUnpackVectorComponent<int>(i, min.value));
            sl->setMaximum(getAndUnpackVectorComponent<int>(i, max.value));
            sl->setValue(  getAndUnpackVectorComponent<int>(i, def.value));
        } else {
            auto[min_, max_, def_] = interpolate(
                getAndUnpackVectorComponent(i, def.value),
                getAndUnpackVectorComponent(i, min.value),
                getAndUnpackVectorComponent(i, max.value), granularity);
            sl->setMinimum(min_);
            sl->setMaximum(max_);
            sl->setValue(def_);
        }

        connect(sl, &QSlider::valueChanged, [this](int) {
            emit this->valueChanged(*value());
        });

        sliders_[i] = sl;

        auto* labelPack = new QHBoxLayout;
        labelPack->addWidget(new QLabel(QString::fromStdString(fmt::format("s{}", i))));
        labelPack->addWidget(sl);

        lay->addLayout(labelPack);
    }

    this->setLayout(lay);
}

std::optional<KernelArgValue> Slider::value() {
    auto traits = findTypeTraits(type_);
    std::vector<Primitive> vec(traits.numComponents);

    for (size_t i = 0; i < vec.size(); ++i) {
        int val = sliders_[i]->value();
        vec[i] = val * granularity;
    }

    return traits.fromVector(vec);
}

KernelArgWidget::KernelArgWidget(
    ArgsTypesWithNames argTypes, KernelArgProperties<UIProperties> conf,
    QWidget *parent) : QWidget(parent) {

    if (argTypes.size() != conf.size()) {
        auto err = fmt::format("Inconsistent sizes argTypes ({}) vs conf ({})", argTypes.size(), conf.size());
        logger->error(err);
        throw std::runtime_error(err);
    }

    auto* layout = new QVBoxLayout;
    for (size_t i = 0; i < conf.size(); ++i) {
        auto [type, name] = argTypes[i];
        if (findTypeTraits(type).klass != KernelArgTypeClass::Memory && !conf[i].userProps().hidden) {
            argProviders.push_back(
                new Slider(type, conf[i].min(), conf[i].max(), conf[i].defaultValue(), this)
            );
            auto* gb = new QGroupBox(QString::fromStdString(name));
            auto* insideGB = new QHBoxLayout;
            insideGB->addWidget(argProviders.back());
            gb->setLayout(insideGB);
            layout->addWidget(gb);
        }
    }

    this->setLayout(layout);
}

KernelArgWidget* makeParameterWidgetForKernel(
    KernelId id, cl::Kernel kernel, KernelArgConfigurationStoragePtr<UIProperties> confStorage
) {
    auto honey = detectArgumentTypesAndNames(kernel);
    auto conf = confStorage->findOrParseConfiguration(id, honey);
    return new KernelArgWidget(honey, conf);
}