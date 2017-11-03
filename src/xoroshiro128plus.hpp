#pragma once

#include <cstdint>
#include <initializer_list>
#include <array>
#include <limits>
#include <cstdio>
#include <type_traits>

namespace Xoroshiro {

template<int k, class T>
constexpr T rotl(T t)
{
    using U = std::make_unsigned_t<T>;
    return T((U(t)<<k)|(U(t)>>(std::numeric_limits<U>::digits-k)));
}
constexpr decltype(auto) splitmix_64(uint64_t seed)
{
    return [x=seed]()mutable {
        auto z = (x += UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        return z ^ (z >> 31);
    };
}
constexpr decltype(auto) splitmix_64_single(uint64_t x)
{
    auto z = (x += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}
struct xoroshiro128plus {
    static constexpr const size_t digits = std::numeric_limits<uint64_t>::digits;
    uint64_t    s0{1};
    uint64_t    s1{2};
    constexpr xoroshiro128plus() = default;
    constexpr xoroshiro128plus(const xoroshiro128plus&) = default;
    constexpr xoroshiro128plus(xoroshiro128plus&&) noexcept = default;
    xoroshiro128plus&operator=(const xoroshiro128plus&) = default;
    xoroshiro128plus&operator=(xoroshiro128plus&&) noexcept = default;

    constexpr xoroshiro128plus(uint64_t _s0, uint64_t _s1)
    : s0(_s0),s1(_s1){}
    constexpr xoroshiro128plus(uint64_t _s0)
    : s0(_s0),s1(splitmix_64_single(_s0)){}

    static constexpr xoroshiro128plus sub_next(uint64_t _s0, uint64_t _s1)
    {
        return xoroshiro128plus(rotl<55>(_s0) ^ _s1 ^(_s1<<14),rotl<36>(_s1));
    }
    constexpr xoroshiro128plus next() const
    {
        return sub_next(s0,s0^s1);
    }
    constexpr xoroshiro128plus operator ()() const
    {
        return next();
    }
    constexpr operator uint64_t() const
    {
        return s0 + s1;
    }
    constexpr friend bool operator==(const xoroshiro128plus &rhs , const xoroshiro128plus &lhs)
    {
        return rhs.s1 == lhs.s1 && rhs.s0 == rhs.s1;
    }
    constexpr friend bool operator!=(const xoroshiro128plus &rhs , const xoroshiro128plus &lhs)
    {
        return !(lhs==rhs);
    }
    constexpr xoroshiro128plus jump() const
    {
//        return sub_jump(xoroshiro128plus(0,0), 0);
        auto res = xoroshiro128plus(0,0);
        for(auto && word : { 0xbeac0467eba5facbul, 0xd86b048b86aa9922ul} ){
            for(auto bit = 1ul; bit; bit <<= 1) {
                if(word & bit) {
                    res.s0 ^= s0;
                    res.s1 ^= s1;
                }
                res = res();
            }
        }
        return res;
    }
};
class xoroshiro128plus_engine {
    protected:
        xoroshiro128plus state_{};
    public:
        using result_type = uint64_t;
        constexpr xoroshiro128plus_engine() = default;
        constexpr xoroshiro128plus_engine(const xoroshiro128plus_engine &) = default;
        constexpr xoroshiro128plus_engine(xoroshiro128plus_engine &&) noexcept = default;
        xoroshiro128plus_engine&operator=(const xoroshiro128plus_engine &) = default;
        xoroshiro128plus_engine&operator=(xoroshiro128plus_engine &&) noexcept = default;

        constexpr xoroshiro128plus_engine(result_type val) { seed(val); }
        constexpr result_type max() const { return std::numeric_limits<result_type>::max();}
        constexpr result_type min() const { return std::numeric_limits<result_type>::min();}
        template<class SSeq>
        explicit xoroshiro128plus_engine(SSeq &q)
        {
            seed(q);
        }
        void seed(result_type val)
        {
            state_ = xoroshiro128plus{val};
        }
        template<class SSeq>
        void seed(SSeq &q)
        {
            auto w = std::array<uint32_t,4>{};
            q.generate(std::begin(w),std::end(w));
            state_ = xoroshiro128plus{w[0]|(uint64_t(w[1])<<32),w[2]|(uint64_t(w[3])<<32)};
        }
        constexpr result_type operator ()()
        {
            return result_type(state_ = state_());
        }
};
}
