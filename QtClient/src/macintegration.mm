#include "macintegration.h"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <QTimer>

namespace {

/// El UTI de los `.nk`. La extension se resuelve a traves de Launch Services en vez de
/// hardcodear `com.foundry.nuke.script`: ese identificador lo exporta el Info.plist de esta app,
/// pero si en la maquina hay otro producto que declara el mismo `.nk` con un UTI propio, el que
/// manda para el doble click es el que resuelva el sistema, no el nuestro. Preguntar por la
/// extension es lo mismo que hace `duti -s <bundle> .nk all`.
UTType *nkContentType()
{
    return [UTType typeWithFilenameExtension:@"nk"];
}

} // namespace

namespace MacIntegration {

bool isDefaultNkHandler()
{
    UTType *type = nkContentType();
    NSURL *bundleUrl = NSBundle.mainBundle.bundleURL;
    if (!type || !bundleUrl)
        return false;

    NSURL *current = [NSWorkspace.sharedWorkspace URLForApplicationToOpenContentType:type];
    if (!current)
        return false;

    // Comparacion por ruta canonica y no por `isEqual:`: las dos URL pueden describir el mismo
    // bundle con distinto trailing slash o pasando por un symlink (`/tmp` -> `/private/tmp`).
    return [current.URLByResolvingSymlinksInPath.path
        isEqualToString:bundleUrl.URLByResolvingSymlinksInPath.path];
}

void setAsDefaultNkHandler(std::function<void(bool ok, const QString &error)> done)
{
    UTType *type = nkContentType();
    NSURL *bundleUrl = NSBundle.mainBundle.bundleURL;
    if (!type || !bundleUrl) {
        if (done)
            done(false, QStringLiteral("no se pudo resolver el UTI de .nk o el bundle de la app"));
        return;
    }

    // Si ya somos el handler no se llama a la API: el sistema muestra su cartel de permiso cada
    // vez que se le pide un CAMBIO, y pedirle que confirme algo que ya esta hecho es ruido.
    if (isDefaultNkHandler()) {
        if (done)
            done(true, QString());
        return;
    }

    [NSWorkspace.sharedWorkspace
        setDefaultApplicationAtURL:bundleUrl
                    toOpenContentType:type
                    completionHandler:^(NSError *_Nullable error) {
        const bool ok = (error == nil);
        const QString detail = ok
            ? QString()
            : QString::fromNSString(error.localizedDescription);
        if (!done)
            return;
        // El completion de NSWorkspace no garantiza el hilo principal, y del otro lado hay
        // widgets: se vuelve al hilo de UI por la cola de eventos de Qt antes de tocar nada.
        QTimer::singleShot(0, [done, ok, detail]() { done(ok, detail); });
    }];
}

} // namespace MacIntegration
