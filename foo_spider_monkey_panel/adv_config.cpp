#include <stdafx.h>

#include "adv_config.h"

namespace
{

advconfig_branch_factory branch_smp(
    "Spider Monkey Panel", smp::guid::adv_branch, advconfig_branch::guid_branch_tools, 0 );
advconfig_branch_factory branch_gc(
    "GC: restart is required", smp::guid::adv_branch_gc, smp::guid::adv_branch, 0 );
#ifdef SMP_ENABLE_CXX_STACKTRACE
advconfig_branch_factory branch_stacktrace(
    "C++ exception stack trace", smp::guid::adv_branch_stacktrace, smp::guid::adv_branch, 1 );
#endif
#ifdef _DEBUG
advconfig_branch_factory branch_zeal(
    "Zeal", smp::guid::adv_branch_zeal, smp::guid::adv_branch_gc, 4 );
#endif

} // namespace

namespace smp::config::advanced
{

advconfig_integer_factory gc_max_heap(
    "Maximum heap size (in bytes) (0 - auto configuration)",
    smp::guid::adv_var_gc_max_heap, smp::guid::adv_branch_gc, 0,
    0, 0, std::numeric_limits<uint32_t>::max() );
advconfig_integer_factory gc_max_heap_growth(
    "Allowed heap growth before GC trigger (in bytes) (0 - auto configuration)",
    smp::guid::adv_var_gc_max_heap_growth, smp::guid::adv_branch_gc, 1,
    0, 0, 256UL * 1024 * 1024 );
advconfig_integer_factory gc_budget(
    "GC cycle time budget (in ms)",
    smp::guid::adv_var_gc_budget, smp::guid::adv_branch_gc, 2,
    5, 1, 100 );
advconfig_integer_factory gc_delay(
    "Delay before next GC trigger (in ms)",
    smp::guid::adv_var_gc_delay, smp::guid::adv_branch_gc, 3,
    50, 1, 500 );
advconfig_integer_factory gc_max_alloc_increase(
    "Allowed number of allocations before next GC trigger",
    smp::guid::adv_var_gc_max_alloc_increase, smp::guid::adv_branch_gc, 4,
    1000, 1, 100000 );

#ifdef SMP_ENABLE_CXX_STACKTRACE
advconfig_checkbox_factory stacktrace(
    "Enable",
    smp::guid::adv_var_stacktrace, smp::guid::adv_branch_stacktrace, 0,
    false );
advconfig_integer_factory stacktrace_max_depth(
    "Maximum stack trace depth",
    smp::guid::adv_var_stacktrace_depth, smp::guid::adv_branch_stacktrace, 1,
    10, 5, 100 );
advconfig_integer_factory stacktrace_max_recursion(
    "Maximum number of recursions",
    smp::guid::adv_var_stacktrace_recursion, smp::guid::adv_branch_stacktrace, 2,
    5, 5, 100 );
#endif

#ifdef _DEBUG
advconfig_checkbox_factory zeal(
    "Enable",
    smp::guid::adv_var_zeal, smp::guid::adv_branch_zeal, 0,
    false );
advconfig_integer_factory zeal_level(
    "Level",
    smp::guid::adv_var_zeal_level, smp::guid::adv_branch_zeal, 1,
    2, 0, 14 );
advconfig_integer_factory zeal_freq(
    "Frequency (in number of allocations)",
    smp::guid::adv_var_zeal_freq, smp::guid::adv_branch_zeal, 2,
    400, 1, 5000 );

#endif

} // namespace smp::config::advanced
