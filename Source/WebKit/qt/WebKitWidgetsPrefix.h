#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H && defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#endif

#include "config.h"

#ifdef QT_DEBUG
#define QT_NOT_REACHED() qt_assert("Not reached!",__FILE__,__LINE__)
#else
#define QT_NOT_REACHED() qt_noop()
#endif

#if OS(WINDOWS)

#ifndef WEBCORE_EXPORT
#define WEBCORE_EXPORT WTF_IMPORT_DECLARATION
#endif // !WEBCORE_EXPORT

#endif
