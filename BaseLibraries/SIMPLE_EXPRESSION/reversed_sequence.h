
#ifndef  _reversed_sequence_
#define  _reversed_sequence_

#include <utility>

template<class list_t, std::size_t new_t>
class push_front_impl;

template<std::size_t... elements_t, std::size_t new_t>
class push_front_impl<std::index_sequence<elements_t...>, new_t>
{
    public:
    using type = std::index_sequence<new_t, elements_t...>;
};

template<class list_t, std::size_t new_t>
using push_front_t = typename push_front_impl<list_t, new_t>::type;


template<std::size_t begin>
struct reversed_sequences_impl
{
    using type=push_front_t<typename reversed_sequences_impl<begin-1>::type,begin>;
};

template<>
struct reversed_sequences_impl<0>
{
    using type=std::index_sequence<0>;
};

template<std::size_t max_t>
using reversed_sequences=typename reversed_sequences_impl<max_t>::type;


#endif

