#pragma once

namespace smp::config::advanced
{

extern advconfig_integer_factory gc_budget;
extern advconfig_integer_factory gc_delay;
extern advconfig_integer_factory gc_max_alloc_increase;
extern advconfig_integer_factory gc_max_heap;
extern advconfig_integer_factory gc_max_heap_growth;

#ifdef _DEBUG
extern advconfig_checkbox_factory zeal;
extern advconfig_integer_factory zeal_freq;
extern advconfig_integer_factory zeal_level;
#endif

} // namespace smp::config::advanced
