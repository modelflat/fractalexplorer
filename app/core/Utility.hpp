#ifndef FRACTALEXPLORER_UTILITY_HPP
#define FRACTALEXPLORER_UTILITY_HPP

#include <string>
#include <sstream>

template <typename Iter>
std::string join(Iter begin_, Iter end_, const char* sep = " ") {
    std::ostringstream ss;
    ss << *begin_;
    begin_++;
    for (; begin_ != end_; begin_++) {
        ss << sep << *begin_;
    }
    return ss.str();
}

#endif //FRACTALEXPLORER_UTILITY_HPP
