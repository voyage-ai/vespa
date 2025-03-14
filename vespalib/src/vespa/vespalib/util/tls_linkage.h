// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#ifdef __PIC__
#define TLS_LINKAGE __attribute__((visibility("hidden"), tls_model("initial-exec")))
#else
#define TLS_LINKAGE __attribute__((visibility("hidden"), tls_model("local-exec")))
#endif
