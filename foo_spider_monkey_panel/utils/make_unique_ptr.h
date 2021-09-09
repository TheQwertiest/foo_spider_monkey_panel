#include <memory>

namespace smp::utils
{

template <typename T, typename D>
[[nodiscard]] std::unique_ptr<T, D> make_unique_with_dtor( T* t, D d )
{
    return std::unique_ptr<T, D>( t, d );
}

} // namespace smp::utils
