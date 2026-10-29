#ifndef REX_TECHNIQUE_H
#define REX_TECHNIQUE_H

#include "rex.h"

#define REX_DECLARE_TECHNIQUE(prefix)                                      \
    static HRESULT prefix##_decode(const rex_arguments *, rex_request *);   \
    static HRESULT prefix##_preflight(const rex_request *, rex_state *);    \
    static HRESULT prefix##_execute(const rex_request *, rex_state *,       \
                                    rex_observation *);                      \
    static void prefix##_release(rex_state *)

#endif
