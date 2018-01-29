#include <QGuiApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>

#include "Paths.h"
#include "Context.h"
#include "EffectNode.h"
#include "Model.h"
#include "Registry.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Radiance");
    QCoreApplication::setOrganizationDomain("radiance.lighting");
    QCoreApplication::setApplicationName("Radiance");
    {
        auto format = QSurfaceFormat::defaultFormat();
        format.setVersion(3, 2);
        format.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);
    }
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);

#ifdef DEBUG_RESOURCES
    Paths::initialize(true);
#else
    Paths::initialize();
#endif

    Context c;
    Registry r;
    auto e1 = r.deserialize(&c, QString("{\"type\": \"EffectNode\", \"name\": \"wwave\", \"intensity\": 0.5}"));
    qDebug() << static_cast<EffectNode*>(e1)->intensity();
    auto e2 = new EffectNode(&c, "resat");
    e2->setIntensity(1.);

    Model m;
    m.addVideoNode(e1);
    m.addVideoNode(e2);
}
