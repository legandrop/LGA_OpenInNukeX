#ifndef MACINTEGRATION_H
#define MACINTEGRATION_H

#include <QString>
#include <functional>

/// Cosas que solo se pueden pedir con las APIs nativas de macOS y que por lo tanto viven en un
/// `.mm`. Mismo patron que `MacIntegration` de LinkRedirector.
namespace MacIntegration {

/// Deja a ESTA app como aplicacion por defecto para los `.nk`, que es exactamente lo que hacia
/// `duti -s com.lga.openinnukex .nk all` — solo que llamando a la misma API que duti llama por
/// dentro, sin depender de un binario de Homebrew que el usuario tiene que instalarse aparte.
///
/// Es ASINCRONA a proposito: `NSWorkspace` puede tener que pedirle permiso al usuario antes de
/// cambiar una asociacion (lo hace el sistema, con su propio cartel), asi que el resultado no se
/// sabe cuando la funcion vuelve. `done` corre en el hilo de UI y recibe el error en crudo de
/// Launch Services cuando falla, para el log.
void setAsDefaultNkHandler(std::function<void(bool ok, const QString &error)> done);

/// Si la app YA es el handler por defecto de los `.nk`. Sirve para no molestar al usuario con el
/// cartel de permiso cuando no hay nada que cambiar.
bool isDefaultNkHandler();

} // namespace MacIntegration

#endif // MACINTEGRATION_H
