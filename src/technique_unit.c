#define COBJMACROS

#include "common/registry.c"
#include "common/result.c"
#include "common/rex.c"
#include "common/runtime.c"
#include "common/text.c"

#if defined(REX_WITH_WMI)
#include "common/wmi.c"
#endif

#if defined(REX_WITH_SERVICE)
#include "common/service.c"
#endif

#include REX_TECHNIQUE_SOURCE
