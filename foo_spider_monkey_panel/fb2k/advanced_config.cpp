#include <stdafx.h>

#include "advanced_config.h"

namespace
{

advconfig_branch_factory branch_smp(
    "Spider Monkey Panel", smp::guid::adv_branch, advconfig_branch::guid_branch_tools, 0 );
advconfig_branch_factory branch_performance(
    "Performance: restart is required", smp::guid::adv_branch_performance, smp::guid::adv_branch, 0 );
advconfig_branch_factory branch_gc(
    "GC", smp::guid::adv_branch_gc, smp::guid::adv_branch_performance, 0 );

#ifdef _DEBUG
advconfig_branch_factory branch_zeal(
    "Zeal", smp::guid::adv_branch_zeal, smp::guid::adv_branch_gc, 4 );
#endif

advconfig_branch_factory branch_debug(
    "Debugging", smp::guid::adv_branch_debug, smp::guid::adv_branch, 99 );

advconfig_branch_factory branch_log(
    "Logging", smp::guid::adv_branch_log, smp::guid::adv_branch_debug, 0 );

#ifdef SMP_ENABLE_CXX_STACKTRACE
advconfig_branch_factory branch_stacktrace(
    "C++ exception stack trace", smp::guid::adv_branch_stacktrace, smp::guid::adv_branch_log, 0 );
#endif

} // namespace

namespace smp::config::advanced
{

qwr::fb2k::AdvConfigUint32_MT gc_max_heap(
    "Maximum heap size (in bytes) (0 - auto configuration)",
    smp::guid::adv_var_gc_max_heap, smp::guid::adv_branch_gc, 0,
    0, 0, std::numeric_limits<uint32_t>::max() );
qwr::fb2k::AdvConfigUint32_MT gc_max_heap_growth(
    "Allowed heap growth before GC trigger (in bytes) (0 - auto configuration)",
    smp::guid::adv_var_gc_max_heap_growth, smp::guid::adv_branch_gc, 1,
    0, 0, 256UL * 1024 * 1024 );
qwr::fb2k::AdvConfigUint32_MT gc_budget(
    "GC cycle time budget (in ms)",
    smp::guid::adv_var_gc_budget, smp::guid::adv_branch_gc, 2,
    5, 1, 100 );
qwr::fb2k::AdvConfigUint32_MT gc_delay(
    "Delay before next GC trigger (in ms)",
    smp::guid::adv_var_gc_delay, smp::guid::adv_branch_gc, 3,
    50, 1, 500 );
qwr::fb2k::AdvConfigUint32_MT gc_max_alloc_increase(
    "Allowed number of allocations before next GC trigger",
    smp::guid::adv_var_gc_max_alloc_increase, smp::guid::adv_branch_gc, 4,
    1000, 1, 100000 );

qwr::fb2k::AdvConfigUint32_MT performance_max_runtime(
    "Script execution time limit before triggering a `slow script` warning (in seconds)",
    smp::guid::adv_var_performance_max_runtime, smp::guid::adv_branch_performance, 4,
    5, 0, 60 );

qwr::fb2k::AdvConfigBool_MT debug_log_extended_include_error(
    "Log additional information when `include()` fails",
    smp::guid::adv_var_log_include_search_paths, smp::guid::adv_branch_log, 50,
    false );

#ifdef SMP_ENABLE_CXX_STACKTRACE
qwr::fb2k::AdvConfigBool_MT stacktrace(
    "Enable",
    smp::guid::adv_var_stacktrace, smp::guid::adv_branch_stacktrace, 0,
    false );
qwr::fb2k::AdvConfigUint32_MT stacktrace_max_depth(
    "Maximum stack trace depth",
    smp::guid::adv_var_stacktrace_depth, smp::guid::adv_branch_stacktrace, 1,
    10, 5, 100 );
qwr::fb2k::AdvConfigUint32_MT stacktrace_max_recursion(
    "Maximum number of recursions",
    smp::guid::adv_var_stacktrace_recursion, smp::guid::adv_branch_stacktrace, 2,
    5, 5, 100 );
#endif

qwr::fb2k::AdvConfigBool_MT debug_use_custom_timer_engine(
    "Use custom timer engine",
    smp::guid::adv_var_debug_timer_engine, smp::guid::adv_branch_debug, 50,
    false );

#ifdef _DEBUG
qwr::fb2k::AdvConfigBool_MT zeal(
    "Enable",
    smp::guid::adv_var_zeal, smp::guid::adv_branch_zeal, 0,
    false );
qwr::fb2k::AdvConfigUint8_MT zeal_level(
    "Level",
    smp::guid::adv_var_zeal_level, smp::guid::adv_branch_zeal, 1,
    2, 0, 14 );
qwr::fb2k::AdvConfigUint32_MT zeal_freq(
    "Frequency (in number of allocations)",
    smp::guid::adv_var_zeal_freq, smp::guid::adv_branch_zeal, 2,
    400, 1, 5000 );

#endif

} // namespace smp::config::advanced
