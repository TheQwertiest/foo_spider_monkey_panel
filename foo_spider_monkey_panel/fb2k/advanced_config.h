#pragma once

#include <qwr/fb2k_adv_config.h>

namespace smp::config::advanced
{

extern qwr::fb2k::AdvConfigUint32_MT gc_budget;
extern qwr::fb2k::AdvConfigUint32_MT gc_delay;
extern qwr::fb2k::AdvConfigUint32_MT gc_max_alloc_increase;
extern qwr::fb2k::AdvConfigUint32_MT gc_max_heap;
extern qwr::fb2k::AdvConfigUint32_MT gc_max_heap_growth;

extern qwr::fb2k::AdvConfigUint32_MT performance_max_runtime;

extern qwr::fb2k::AdvConfigBool_MT debug_log_extended_include_error;
extern qwr::fb2k::AdvConfigBool_MT debug_use_custom_timer_engine;

#ifdef SMP_ENABLE_CXX_STACKTRACE
extern qwr::fb2k::AdvConfigBool_MT stacktrace;
extern qwr::fb2k::AdvConfigUint32_MT stacktrace_max_depth;
extern qwr::fb2k::AdvConfigUint32_MT stacktrace_max_recursion;
#endif

#ifdef _DEBUG
extern qwr::fb2k::AdvConfigBool_MT zeal;
extern qwr::fb2k::AdvConfigUint32_MT zeal_freq;
extern qwr::fb2k::AdvConfigUint8_MT zeal_level;
#endif

} // namespace smp::config::advanced
