#pragma once

#include <mutex>

namespace hack
{

// https://developercommunity.visualstudio.com/content/problem/842917/error-c3538-when-using-scoped-lock-in-derived-temp.html
using scoped_lock = std::scoped_lock<std::mutex>;

} // namespace hack
