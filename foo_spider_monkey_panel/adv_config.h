#pragma once

namespace smp::config::advanced
{

extern advconfig_integer_factory gc_budget;
extern advconfig_integer_factory gc_delay;
extern advconfig_integer_factory gc_max_alloc_increase;
extern advconfig_integer_factory gc_max_heap;
extern advconfig_integer_factory gc_max_heap_growth;

extern advconfig_integer_factory performance_max_runtime;

#ifdef SMP_ENABLE_CXX_STACKTRACE
extern advconfig_checkbox_factory stacktrace;
extern advconfig_integer_factory stacktrace_max_depth;
extern advconfig_integer_factory stacktrace_max_recursion;
#endif

#ifdef _DEBUG
extern advconfig_checkbox_factory zeal;
extern advconfig_integer_factory zeal_freq;
extern advconfig_integer_factory zeal_level;
#endif

} // namespace smp::config::advanced
