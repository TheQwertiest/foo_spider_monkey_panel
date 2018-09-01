
namespace smp::config::advanced
{

extern advconfig_integer_factory g_var_max_heap;
extern advconfig_integer_factory g_var_max_heap_growth;
extern advconfig_integer_factory g_var_gc_budget;
extern advconfig_integer_factory g_var_gc_delay;
#ifdef _DEBUG
extern advconfig_checkbox_factory g_var_gc_zeal;
extern advconfig_integer_factory g_var_gc_zeal_level;
extern advconfig_integer_factory g_var_gc_zeal_freq;
#endif

}
