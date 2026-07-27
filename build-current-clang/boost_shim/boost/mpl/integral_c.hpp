#ifndef BOOST_MPL_INTEGRAL_C_HPP_INCLUDED
#define BOOST_MPL_INTEGRAL_C_HPP_INCLUDED

#include <boost/mpl/integral_c_fwd.hpp>
#include <boost/mpl/integral_c_tag.hpp>
#include <boost/mpl/aux_/adl_barrier.hpp>

BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE_OPEN

template<typename T, T N>
struct integral_c
{
    static constexpr T value = N;
    typedef integral_c_tag tag;
    typedef integral_c type;
    typedef T value_type;
    typedef integral_c<T, N> next;
    typedef integral_c<T, N> prior;
    constexpr operator T() const { return value; }
};

template<bool C>
struct integral_c<bool, C>
{
    static constexpr bool value = C;
    typedef integral_c_tag tag;
    typedef integral_c type;
    typedef bool value_type;
    constexpr operator bool() const { return value; }
};

BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE_CLOSE

#endif
