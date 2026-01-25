#include "tkit/utils/hash.hpp"
#include "tkit/math/quaternion.hpp"

template <typename T, TKit::usize N0, TKit::usize... N> struct std::hash<TKit::Math::Tensor<T, N0, N...>>
{
    std::size_t operator()(const TKit::Math::Tensor<T, N0, N...> &tensor) const
    {
        return TKit::HashRange(tensor.GetData(), tensor.GetData() + (N0 * ... * N));
    }
};
template <typename T> struct std::hash<TKit::Math::Quaternion<T>>
{
    std::size_t operator()(const TKit::Math::Quaternion<T> &quaternion) const
    {
        return TKit::Hash(quaternion.w, quaternion.x, quaternion.y, quaternion.z);
    }
};
