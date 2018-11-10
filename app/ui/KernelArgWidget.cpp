#include "KernelArgWidget.hpp"
#include "app/core/Utility.hpp"

#include <QVBoxLayout>

LOGGER()
//
//template <typename T>
//T getFromStream(std::istream& ss, bool& err)  {
//    T val; ss >> val;
//    err |= ss.fail();
//    return val;
//}
//
//template <typename ...Ts>
//std::optional<KernelArgValue> parseValue(const std::string& string) {
//    std::istringstream iss { string };
//    bool error = false;
//    KernelArgValue value { getFromStream<Ts>(iss, error)... };
//    return value;
//}
//
//std::optional<KernelArgValue> parseNumericValue(KernelArgType valueType, const std::string& string) {
//    switch (valueType) {
//        case KernelArgType::Float32:
//            return parseValue<float>(string);
//        case KernelArgType::Float64:
//            return parseValue<double>(string);
//        case KernelArgType::Int32:
//            return parseValue<int32_t>(string);
//        case KernelArgType::Int64:
//            return parseValue<int64_t>(string);
//        case KernelArgType::Vector2Float32:
//            return parseValue<float, float>(string);
//        case KernelArgType::Vector2Float64:
//            return parseValue<double, double>(string);
//        case KernelArgType::Vector3Float32:
//            return parseValue<float, float, float>(string);
//        case KernelArgType::Vector3Float64:
//            return parseValue<double, double, double>(string);
//        default:
//            logger->warn(fmt::format("KernelArgType::<{}> is not a numeric value, can't parse", (int)valueType));
//            return {};
//    }
//}
//
//NumberWidget::NumberWidget(const QString& name, KernelArgType valueType, ArgProperties<UIProperties> conf, QWidget *parent)
//: ArgProviderWidget(parent), valueType_(valueType) {
//    groupBox = new QGroupBox(name, this);
//    auto* lines = new QVBoxLayout;
//    textBox = new QLineEdit;
//    lines->addWidget(textBox);
//    lines->addStretch(1);
//
//    groupBox->setLayout(lines);
//
//    new Slider(valueType, conf.min(), conf.max(), conf.defaultValue(), this);
//
//    connect(textBox, & QLineEdit::textEdited, [this](const QString& text){
//        auto value = this->value();
//        if (value) { emit valueChanged(*value); }
//    });
//}
//
//std::optional<KernelArgValue> NumberWidget::value() {
//    auto text = this->textBox->text().trimmed();
//    if (text.isEmpty()) { return {}; }
//    return parseNumericValue(valueType_, text.toStdString());
//}

KernelArgWidget::KernelArgWidget(
    ArgsTypesWithNames argTypes, KernelArgProperties<UIProperties> conf,
    QWidget *parent) : QWidget(parent) {

    if (argTypes.size() == conf.size()) {
        auto err = fmt::format("Inconsistent sizes argTypes ({}) vs conf ({})", argTypes.size(), conf.size());
        logger->error(err);
        throw std::runtime_error(err);
    }

    std::transform(conf.begin(), conf.end(), argTypes.begin(), std::back_inserter(argProviders),
        [&](auto argConf, auto argTypeAndName) {
        auto [type, name] = argTypeAndName;
        return new Slider(type, argConf.min(), argConf.max(), argConf.defaultValue(), this);
    });

    auto* layout = new QVBoxLayout;
    std::for_each(argProviders.begin(), argProviders.end(), [layout](auto* pr) {
        layout->addWidget(pr);
    });

    this->setLayout(layout);
}

KernelArgWidget* makeParameterWidgetForKernel(
    KernelId id, cl::Kernel kernel, KernelArgConfigurationStoragePtr<UIProperties> confStorage
) {
    auto honey = detectArgumentTypesAndNames(kernel);
    auto conf = confStorage->findOrParseConfiguration(id, honey);
    return new KernelArgWidget(honey, conf);
}

template <typename Flt>
std::tuple<int, int, int> interpolate(Flt val, Flt min, Flt max, double granularity) {
    auto span = std::abs(max - min);
    auto rawSize = span / granularity;
    int intSize = static_cast<int>(std::floor(rawSize));
    int intVal = static_cast<int>(std::floor(val / granularity));
    return { 0, intSize, intVal };
}

static constexpr double granularity = 1e-4f;

Primitive getAndUnpackVectorComponent(size_t idx, AnyType val) {
    return std::visit([](auto val){
        return static_cast<Primitive>(val);
    }, getVectorComponent(idx, val));
}

Slider::Slider(KernelArgType type, KernelArgValue min, KernelArgValue max, KernelArgValue def, QWidget *parent)
: ArgProviderWidget(parent), type_(type) {
    auto traits = findTypeTraits(type);
    sliders_ = std::vector<QSlider *>(traits.numComponents);

    auto* lay = new QVBoxLayout;

    for (size_t i = 0; i < sliders_.size(); ++i) {
        auto *sl = new QSlider(Qt::Orientation::Horizontal);
        auto[min_, max_, def_] = interpolate(
            getAndUnpackVectorComponent(i, def.value),
            getAndUnpackVectorComponent(i, min.value),
            getAndUnpackVectorComponent(i, max.value), granularity);
        sl->setMinimum(min_);
        sl->setMaximum(max_);
        sl->setValue(def_);

        connect(sl, &QSlider::valueChanged, [this](int) {
            emit this->valueChanged(*value());
        });

        sliders_[i] = sl;

        auto* labelPack = new QHBoxLayout;
        labelPack->addWidget(new QLabel(QString::fromStdString(fmt::format("s{}", i))));
        labelPack->addWidget(sl);

        lay->addLayout(labelPack);
    }

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
