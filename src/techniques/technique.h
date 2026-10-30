#ifndef REX_TECHNIQUE_H
#define REX_TECHNIQUE_H

#include "rex.h"

#if defined(_MSC_VER)
#define REX_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#define REX_NOINLINE __attribute__((noinline))
#else
#define REX_NOINLINE
#endif

#define REX_DECLARE_TECHNIQUE(prefix)                                          \
    static HRESULT prefix##_decode(const rex_arguments *, rex_request *);      \
    static HRESULT prefix##_preflight(const rex_request *, rex_state *);       \
    static HRESULT prefix##_execute(const rex_request *, rex_state *,          \
                                    rex_observation *);                        \
    static void prefix##_release(rex_state *)

#if defined(REX_STANDALONE)
#define REX_TECHNIQUE_LOADER(prefix) prefix##_load_technique
#else
#define REX_TECHNIQUE_LOADER(prefix) rex_load_technique
#endif

#define REX_DEFINE_TECHNIQUE(prefix, selector_value, preflight_value,          \
                             execution_value, disposition_value)               \
    REX_DECLARE_TECHNIQUE(prefix);                                             \
    REX_NOINLINE void REX_TECHNIQUE_LOADER(prefix)(rex_technique *             \
                                                   technique) {                \
        technique->selector = selector_value;                                  \
        technique->preflight_evidence_source = preflight_value;                \
        technique->execution_evidence_source = execution_value;                \
        technique->success_disposition = disposition_value;                    \
        technique->decode = prefix##_decode;                                   \
        technique->preflight = prefix##_preflight;                             \
        technique->execute = prefix##_execute;                                 \
        technique->release = prefix##_release;                                 \
    }                                                                          \
    typedef int rex_technique_definition_requires_semicolon

#endif
