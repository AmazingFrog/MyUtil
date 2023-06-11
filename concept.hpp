#ifndef _CONCEPT_H_
#define _CONCEPT_H_

#include <string>
#include <concepts>
#include <iterator>

namespace shochu {

namespace util {
template<typename T>
std::string to_string(const T&);
}

namespace concepts {
using std::to_string;
using shochu::util::to_string;

template<typename T>
concept hasToString = requires(T a) {
    { to_string(a) } -> std::same_as<std::string>;
};

template<typename T>
concept hasIter = requires(T a) {
    std::begin(a);
    std::end(a);
};

} // concepts
} // shochu

#endif // _CONCEPT_H_
