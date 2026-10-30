#ifndef REX_PROVIDER_H
#define REX_PROVIDER_H

#include "rex.h"

HRESULT rex_provider_decode(const rex_arguments *arguments,
                            rex_request *request, rex_provider_role role);
HRESULT rex_provider_preflight(const rex_request *request, rex_state *state);
HRESULT rex_provider_execute(const rex_request *request, rex_state *state,
                             rex_observation *observation);
void rex_provider_release(rex_state *state);

#endif
