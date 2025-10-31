#ifndef ANALYSIS_CONTEXT_H
#define ANALYSIS_CONTEXT_H

#include "RadioContext.h"
#include "DisplayContext.h"

struct AggregateContext {
    RadioContext* radioCtx;
    DisplayContext* displayCtx;
};

#endif // ANALYSIS_CONTEXT_H