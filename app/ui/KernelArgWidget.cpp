#include "KernelArgWidget.hpp"
#include "app/core/Utility.hpp"
#include <iostream>
#include <QVBoxLayout>

LOGGER()

static constexpr double granularity = 1e-2f;

template <typename Flt>
std::tuple<int, int, int> interpolate(Flt val, Flt min, Flt max, double granularity) {
    int intMin = static_cast<int>(std::floor(min / granularity));
    int intSize = static_cast<int>(std::floor(max / granularity));
    int intVal = static_cast<int>(std::floor(val / granularity));
    return { intMin, intSize, intVal };
}

template <typename T = Primitive>
T getAndUnpackVectorComponent(size_t idx, AnyType val) {
    return std::visit([](auto val){
        return static_cast<T>(val);
    }, getVectorComponent(idx, val));
}

void applyValuesToSlider(QSlider* slider, size_t componentIdx, KernelArgTypeTraits traits,
    KernelArgValue min, KernelArgValue max, KernelArgValue def
) {
    if (traits.klass == KernelArgTypeClass::Integer) {
        // put an argument as is
        slider->setMinimum(getAndUnpackVectorComponent<int>(componentIdx, min.value));
        slider->setMaximum(getAndUnpackVectorComponent<int>(componentIdx, max.value));
        slider->setValue(  getAndUnpackVectorComponent<int>(componentIdx, def.value));
    } else {
        auto[min_, max_, def_] = interpolate(
            getAndUnpackVectorComponent(componentIdx, def.value),
            getAndUnpackVectorComponent(componentIdx, min.value),
            getAndUnpackVectorComponent(componentIdx, max.value), granularity);
        slider->setMinimum(min_);
        slider->setMaximum(max_);
        slider->setValue(def_);
    }
}

Slider::Slider(KernelArgType type, KernelArgValue min, KernelArgValue max, KernelArgValue def, QWidget *parent)
: ArgProviderWidget(parent), type_(type) {
    auto traits = findTypeTraits(type);

    sliders_ = QVector<QSlider *>(traits.numComponents);

    auto* lay = new QVBoxLayout;

    for (size_t i = 0; i < sliders_.size(); ++i) {
        auto *slider = new QSlider(Qt::Orientation::Horizontal);

        applyValuesToSlider(slider, i, traits, min, max, def);

        connect(slider, &QSlider::valueChanged, [this](int) {
            emit this->valueChanged(*value());
        });

        sliders_[i] = slider;

        auto* labelPack = new QHBoxLayout;
        labelPack->addWidget(new QLabel(QString::fromStdString(fmt::format("s{}", i))));
        labelPack->addWidget(slider);

        lay->addLayout(labelPack);
    }

    this->setLayout(lay);
}

std::optional<KernelArgValue> Slider::value() {
    auto traits = findTypeTraits(type_);
    std::vector<Primitive> vec(traits.numComponents);

    for (size_t i = 0; i < vec.size(); ++i) {
        int val = sliders_[i]->value();
        vec[i] = val * (traits.klass == KernelArgTypeClass::Integer ? 1.0 : granularity);
    }

    return traits.fromVector(vec);
}

KernelArgWidget::KernelArgWidget(
    ArgsTypesWithNames argTypes, KernelArgProperties<UIProperties> conf,
    QWidget *parent) : QWidget(parent), cachedArgValues() {

    if (argTypes.size() != conf.size()) {
        auto err = fmt::format("Inconsistent sizes argTypes ({}) vs conf ({})", argTypes.size(), conf.size());
        logger->error(err);
        throw std::runtime_error(err);
    }

    auto* layout = new QVBoxLayout;
    for (size_t i = 0; i < conf.size(); ++i) {
        auto [type, name] = argTypes[i];
        if (findTypeTraits(type).klass != KernelArgTypeClass::Memory) {
            if (!conf[i].userProps().hidden) {
                auto* slider = new Slider(type, conf[i].min(), conf[i].max(), conf[i].defaultValue());
                argProviders.push_back(slider);

                auto *gb = new QGroupBox(QString::fromStdString(name));
                auto *insideGB = new QVBoxLayout;
                insideGB->addWidget(slider);
                insideGB->setMargin(0);
                gb->setLayout(insideGB);

                layout->addWidget(gb);

                connect(slider, &Slider::valueChanged, [this, i](auto val) {
                        cachedArgValues[i] = val;
                        emit this->valuesChanged(cachedArgValues);
                    }
               );
            }
            cachedArgValues[i] = conf[i].defaultValue();
        }
    }

    this->setLayout(layout);
}

KernelArgWidget* makeParameterWidgetForKernel(
    KernelId id, cl::Kernel kernel, KernelArgConfigurationStoragePtr<UIProperties> confStorage,
    QWidget* parent
) {
    auto honey = detectArgumentTypesAndNames(kernel);
    auto conf = confStorage->findOrParseConfiguration(id, honey);
    return new KernelArgWidget(honey, conf, parent);
}