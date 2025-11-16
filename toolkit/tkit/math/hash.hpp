#include "tkit/utils/hash.hpp"
#include "tkit/math/quaternion.hpp"

template <typename T, TKit::usize N0, TKit::usize... N> struct std::hash<TKit::Math::Tensor<T, N0, N...>>
{
    std::size_t operator()(const TKit::Math::Tensor<T, N0, N...> &p_Tensor) const
    {
        return TKit::HashRange(p_Tensor.GetData(), p_Tensor.GetData() + (N0 * ... * N));
    }
};
template <typename T> struct std::hash<TKit::Math::Quaternion<T>>
{
    std::size_t operator()(const TKit::Math::Quaternion<T> &p_Quaternion) const
    {
        return TKit::Hash(p_Quaternion.w, p_Quaternion.x, p_Quaternion.y, p_Quaternion.z);
    }
};
