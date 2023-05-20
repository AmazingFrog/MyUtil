#ifndef _CONCEPT_H_
#define _CONCEPT_H_

#include <concepts>
#include <string>
#include <iterator>

namespace shochu {
namespace concepts {

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
