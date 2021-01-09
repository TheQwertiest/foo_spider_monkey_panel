#include <memory>

namespace smp::utils
{

template <typename T, typename D>
std::unique_ptr<T, D> make_unique( T* t, D d )
{
    return std::unique_ptr<T, D>( t, d );
}

} // namespace smp::utils
