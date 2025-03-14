// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file geometry/matrix.hpp Defines geo::matrix, matrix2 and matrix3.
 * @ingroup geometry
 */

#pragma once

#include "translate2.hpp"
#include "scale2.hpp"
#include "rotate2.hpp"
#include "aarectangle.hpp"
#include "../macros.hpp"
#include <array>
#include <exception>
#include <compare>
#include <stdexcept>

hi_export_module(hikogui.geometry : matrix2);

hi_export namespace hi { inline namespace v1 {
class aarectangle;
class scale2;
class translate2;
class matrix2;

[[nodiscard]] constexpr matrix2 operator*(matrix2 const& lhs, matrix2 const& rhs) noexcept;
[[nodiscard]] constexpr aarectangle operator*(scale2 const& lhs, aarectangle const& rhs) noexcept;
[[nodiscard]] constexpr matrix2 operator*(translate2 const& lhs, scale2 const& rhs) noexcept;

/** A 2D or 3D homogeneous matrix for transforming homogeneous vectors and points.
 *
 * This matrix is in column major order. It is implemented as 4 columns made
 * from a `f32x4` numeric-array.
 *
 */
class matrix2 {
public:
    constexpr matrix2(matrix2 const&) noexcept = default;
    constexpr matrix2(matrix2&&) noexcept = default;
    constexpr matrix2& operator=(matrix2 const&) noexcept = default;
    constexpr matrix2& operator=(matrix2&&) noexcept = default;

    /** Constructs an identity matrix.
     */
    constexpr matrix2() noexcept
    {
        auto const a = f32x4::broadcast(1.0f);
        _col0 = a.x000();
        _col1 = a._0y00();
        _col2 = a._00z0();
        _col3 = a._000w();
    };

    /** Construct a matrix from four columns.
     *
     * @param col0 The 1st `f32x4` column.
     * @param col1 The 2nd `f32x4` column.
     * @param col2 The 3rd `f32x4` column.
     * @param col3 The 4th `f32x4` column.
     */
    constexpr matrix2(f32x4 col0, f32x4 col1, f32x4 col2, f32x4 col3 = f32x4{0.0f, 0.0f, 0.0f, 1.0f}) noexcept :
        _col0(col0), _col1(col1), _col2(col2), _col3(col3)
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a matrix from four vectors.
     *
     * @param col0 The 1st `vector3` column.
     * @param col1 The 2nd `vector3` column.
     * @param col2 The 3rd `vector3` column.
     * @param col3 The 4th `vector3` column.
     */
    constexpr matrix2(vector3 col0, vector3 col1, vector3 col2, vector3 col3 = vector3{}) noexcept :
        _col0(static_cast<f32x4>(col0)),
        _col1(static_cast<f32x4>(col1)),
        _col2(static_cast<f32x4>(col2)),
        _col3(static_cast<f32x4>(col3).xyz1())
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a matrix from two vectors.
     *
     * @param col0 The 1st `vector2` column.
     * @param col1 The 2nd `vector2` column.
     */
    constexpr matrix2(vector2 col0, vector2 col1) noexcept :
        _col0(static_cast<f32x4>(col0)),
        _col1(static_cast<f32x4>(col1)),
        _col2(f32x4{0.0f, 0.0f, 1.0f, 0.0f}),
        _col3(f32x4{0.0f, 0.0f, 0.0f, 1.0f})
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a 3x3 matrix from scalar values.
     *
     * The function parameters are in row-major order for pretty formatting in source code.
     * The matrix itself remains in column-major order.
     *
     * @param c0r0 Value for column 0, row 0
     * @param c1r0 Value for column 1, row 0
     * @param c2r0 Value for column 2, row 0
     * @param c0r1 Value for column 0, row 1
     * @param c1r1 Value for column 1, row 1
     * @param c2r1 Value for column 2, row 1
     * @param c0r2 Value for column 0, row 2
     * @param c1r2 Value for column 1, row 2
     * @param c2r2 Value for column 2, row 2
     */
    constexpr matrix2(
        float c0r0,
        float c1r0,
        float c2r0,
        float c0r1,
        float c1r1,
        float c2r1,
        float c0r2,
        float c1r2,
        float c2r2) noexcept :
        _col0(c0r0, c0r1, c0r2, 0.0f), _col1(c1r0, c1r1, c1r2, 0.0f), _col2(c2r0, c2r1, c2r2, 0.0f), _col3(0.0f, 0.0f, 0.0f, 1.0f)
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a 4x4 matrix from scalar values.
     *
     * The function parameters are in row-major order for pretty formatting in source code.
     * The matrix itself remains in column-major order.
     *
     * @param c0r0 Value for column 0, row 0
     * @param c1r0 Value for column 1, row 0
     * @param c2r0 Value for column 2, row 0
     * @param c3r0 Value for column 3, row 0
     * @param c0r1 Value for column 0, row 1
     * @param c1r1 Value for column 1, row 1
     * @param c2r1 Value for column 2, row 1
     * @param c3r1 Value for column 3, row 1
     * @param c0r2 Value for column 0, row 2
     * @param c1r2 Value for column 1, row 2
     * @param c2r2 Value for column 2, row 2
     * @param c3r2 Value for column 3, row 2
     * @param c0r3 Value for column 0, row 3
     * @param c1r3 Value for column 1, row 3
     * @param c2r3 Value for column 2, row 3
     * @param c3r3 Value for column 3, row 3
     */
    constexpr matrix2(
        float c0r0,
        float c1r0,
        float c2r0,
        float c3r0,
        float c0r1,
        float c1r1,
        float c2r1,
        float c3r1,
        float c0r2,
        float c1r2,
        float c2r2,
        float c3r2,
        float c0r3,
        float c1r3,
        float c2r3,
        float c3r3) noexcept :
        _col0(c0r0, c0r1, c0r2, c0r3), _col1(c1r0, c1r1, c1r2, c1r3), _col2(c2r0, c2r1, c2r2, c2r3), _col3(c3r0, c3r1, c3r2, c3r3)
    {
        hi_axiom(holds_invariant());
    }

    [[nodiscard]] constexpr matrix2(translate2 const& rhs) noexcept
    {
        auto const ones = f32x4::broadcast(1.0f);
        _col0 = ones.x000();
        _col1 = ones._0y00();
        _col2 = ones._00z0();
        _col3 = ones._000w() + f32x4{rhs};
    }

    [[nodiscard]] constexpr matrix2(scale2 const& rhs) noexcept
    {
        _col0 = f32x4{rhs}.x000();
        _col1 = f32x4{rhs}._0y00();
        _col2 = f32x4{rhs}._00z0();
        _col3 = f32x4{rhs}._000w();
    }

    /** Convert quaternion to matrix.
     *
     */
    [[nodiscard]] constexpr matrix2(rotate2 const& rhs) noexcept
    {
        // Original from https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
        //   1 - 2(yy + zz) |     2(xy - zw) |     2(xz + yw)
        //       2(xy + zw) | 1 - 2(xx + zz) |     2(yz - xw)
        //       2(xz - yw) |     2(yz + xw) | 1 - 2(xx + yy)

        // Flipping adds and multiplies:
        //   1 - 2(zz + yy) |     2(xy - zw) |     2(yw + xz)
        //       2(zw + yx) | 1 - 2(xx + zz) |     2(yz - xw)
        //       2(zx - yw) |     2(xw + zy) | 1 - 2(yy + xx)

        // All multiplies.
        auto const x_mul = f32x4{rhs}.xxxx() * f32x4{rhs};
        auto const y_mul = f32x4{rhs}.yyyy() * f32x4{rhs};
        auto const z_mul = f32x4{rhs}.zzzz() * f32x4{rhs};

        auto twos = f32x4{-2.0f, 2.0f, 2.0f, 0.0f};
        auto one = f32x4{1.0f, 0.0f, 0.0f, 0.0f};
        _col0 = one + addsub_mask<0b0011>(z_mul.zwxy(), y_mul.yxwz()) * twos;
        one = one.yxzw();
        twos = twos.yxzw();
        _col1 = one + addsub_mask<0b0110>(x_mul.yxwz(), z_mul.wzyx()) * twos;
        one = one.xzyw();
        twos = twos.xzyw();
        _col2 = one + addsub_mask<0b0101>(y_mul.wzyx(), x_mul.zwxy()) * twos;
        _col3 = one.xywz();
    }

    /** Convert a point to its f32x4-numeric_array.
     */
    [[nodiscard]] constexpr explicit operator std::array<f32x4, 4>() const noexcept
    {
        hi_axiom(holds_invariant());
        return {_col0, _col1, _col2, _col3};
    }

    /** Create a transformation matrix to translate and uniformly-scale a src_rectangle to a dst_rectangle.
     *
     * The implementation is located in scale.hpp since the definition requires both scale and translate.
     *
     * @param src_rectangle The rectangle to be transformed.
     * @param dst_rectangle The rectangle after transformation.
     * @param alignment How the src_rectangle should be aligned inside the dst_rectangle after scaling and moving.
     * @return A transformation matrix to move and scale the src_rectangle to the dst_rectangle.
     */
    [[nodiscard]] constexpr static matrix2
    uniform(aarectangle src_rectangle, aarectangle dst_rectangle, alignment alignment) noexcept
    {
        auto const scale = scale2::uniform(src_rectangle.size(), dst_rectangle.size());
        auto const scaled_rectangle = scale * src_rectangle;
        auto const translation = translate2::align(scaled_rectangle, dst_rectangle, alignment);
        return translation * scale;
    }

    /** Get a column.
     *
     * @tparam I The index of the column.
     * @return A const reference to the selected column as a `f32x4`.
     */
    template<int I>
    [[nodiscard]] friend constexpr f32x4 const& get(matrix2 const& rhs) noexcept
    {
        if constexpr (I == 0) {
            return rhs._col0;
        } else if constexpr (I == 1) {
            return rhs._col1;
        } else if constexpr (I == 2) {
            return rhs._col2;
        } else if constexpr (I == 3) {
            return rhs._col3;
        } else {
            hi_static_no_default();
        }
    }

    /** Get a column.
     *
     * @tparam I The index of the column.
     * @return A reference to the selected column as a `f32x4`.
     */
    template<int I>
    [[nodiscard]] friend constexpr f32x4& get(matrix2& rhs) noexcept
    {
        if constexpr (I == 0) {
            return rhs._col0;
        } else if constexpr (I == 1) {
            return rhs._col1;
        } else if constexpr (I == 2) {
            return rhs._col2;
        } else if constexpr (I == 3) {
            return rhs._col3;
        } else {
            hi_static_no_default();
        }
    }

    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return _col0.z() == 0.0f and _col0.w() == 0.0f and _col1.z() == 0.0f and _col1.w() == 0.0f and _col2.x() == 0.0f and
            _col2.y() == 0.0f and _col2.z() == 1.0f and _col2.w() == 0.0f and _col3.z() == 0.0f and _col3.w() == 1.0f;
    }

    /** Transform a `f32x4` numeric array by the matrix.
     *
     * @param rhs The `f32x4` numeric array to transform.
     * @return The transformed numeric array.
     */
    [[nodiscard]] constexpr f32x4 operator*(f32x4 const& rhs) const noexcept
    {
        return {_col0 * rhs.xxxx() + _col1 * rhs.yyyy() + _col2 * rhs.zzzz() + _col3 * rhs.wwww()};
    }

    /** Matrix transpose.
     */
    [[nodiscard]] friend constexpr matrix2 transpose(matrix2 const& rhs) noexcept
    {
        auto tmp = transpose(rhs._col0, rhs._col1, rhs._col2, rhs._col3);
        return matrix2{std::get<0>(tmp), std::get<1>(tmp), std::get<2>(tmp), std::get<3>(tmp)};
    }

    /** Reflect axis of a matrix.
     *
     * The default axis of HikoGUI's geometry system are:
     * ```
     *        +y
     *        |   -z (away from camera)
     *        |  /
     *        | /
     *        |/
     * -x ----+---- +x
     *       /|
     *      / |
     *     /  |
     *   +z   |
     *        -y
     * ```
     *
     * In Vulkan the Y axis is downward; so to translate a matrix from HikoGUI to Vulkan you can use:
     *
     * ```
     * auto vulkan_M = reflect<'x', 'Y', 'z'>(hikogui_M);
     * ```
     *
     * The original axis are defined as the lower-case characters 'x', 'y', 'z' and 'w';
     * or for the negated axis as the upper-case characters 'X', 'Y', 'Z' and 'W'.
     *
     * @tparam DstX Which of the original axis to use for the new matrix's x-axis.
     * @tparam DstY Which of the original axis to use for the new matrix's y-axis.
     * @tparam DstZ Which of the original axis to use for the new matrix's z-axis.
     * @tparam DstW Which of the original axis to use for the new matrix's w-axis.
     * @param rhs The matrix to reflect
     * @return The reflected matrix.
     */
    template<char DstX, char DstY, char DstZ, char DstW = 'w'>
    [[nodiscard]] friend constexpr matrix2 reflect(matrix2 const& rhs) noexcept
    {
        return matrix2{reflect_column<DstX>(), reflect_column<DstY>(), reflect_column<DstZ>(), reflect_column<DstW>()} * rhs;
    }

    /** Compare two matrices potentially of different dimensions.
     *
     * @param rhs The right-hand-side matrix.
     * @retval true When the two matrices compare equal.
     */
    [[nodiscard]] constexpr friend bool operator==(matrix2 const& lhs, matrix2 const& rhs) noexcept
    {
        return equal(lhs._col0, rhs._col0) and equal(lhs._col1, rhs._col1) and equal(lhs._col3, rhs._col3);
    }

    /** Invert matrix.
     */
    [[nodiscard]] constexpr matrix2 operator~() const
    {
        //                   rc
        // var s0 : Number = i00 * i11 -
        //                  i10 * i01;
        // var c0 : Number = i20 * i31 -
        //                  i30 * i21;
        auto const s0c0 = _col0 * _col1.yxwz();

        // var s1 : Number = i00 * i12 -
        //                  i10 * i02;
        // var c1 : Number = i20 * i32 -
        //                  i30 * i22;
        auto const s1c1 = _col0 * _col2.yxwz();
        auto const s0c0s1c1 = hsub(s0c0, s1c1);

        // var s2 : Number = i00 * i13 -
        //                  i10 * i03;
        // var c2 : Number = i20 * i33 -
        //                  i30 * i23;
        auto const s2c2 = _col0 * _col3.yxwz();

        // var s3 : Number = i01 * i12 -
        //                  i11 * i02;
        // var c3 : Number = i21 * i32 -
        //                  i31 * i22;
        auto const s3c3 = _col1 * _col2.yxwz();
        auto const s2c2s3c3 = hsub(s2c2, s3c3);

        // var s4 : Number = i01 * i13 -
        //                  i11 * i03;
        // var c4 : Number = i21 * i33 -
        //                  i31 * i23;
        auto const s4c4 = _col1 * _col3.yxwz();

        // var s5 : Number = i02 * i13 -
        //                  i12 * i03;
        // var c5 : Number = i22 * i33 -
        //                  i32 * i23;
        auto const s5c5 = _col2 * _col3.yxwz();
        auto const s4c4s5c5 = hsub(s4c4, s5c5);

        // det = (s0 * c5 +
        //       -s1 * c4 +
        //        s2 * c3 +
        //        s3 * c2 +
        //       -s4 * c1 +
        //        s5 * c0)
        auto const s0123 = s0c0s1c1.xz00() + s2c2s3c3._00xz();
        auto const s45__ = s4c4s5c5.xz00();

        auto const c5432 = s4c4s5c5.wy00() + s2c2s3c3._00wy();
        auto const c10__ = s0c0s1c1.wy00();

        auto const det_prod_half0 = neg_mask<0b0010>(s0123 * c5432);
        auto const det_prod_half1 = neg_mask<0b0001>(s45__ * c10__);

        auto const det_sum0 = hadd(det_prod_half0, det_prod_half1);
        auto const det_sum1 = hadd(det_sum0, det_sum0);
        auto const det = hadd(det_sum1, det_sum1).xxxx();

        if (det.x() == 0.0f) {
            throw std::domain_error("Divide by zero");
        }

        auto const invdet = rcp(det);

        auto const t = transpose(*this);

        //   rc     rc          rc          rc
        // m.i00 := (i11 *  c5 + i12 * -c4 + i13 *  c3) * invdet;
        // m.i10 := (i10 * -c5 + i12 *  c2 + i13 * -c1) * invdet;
        // m.i20 := (i10 *  c4 + i11 * -c2 + i13 *  c0) * invdet;
        // m.i30 := (i10 * -c3 + i11 *  c1 + i12 * -c0) * invdet;
        auto tmp_c5543 = neg_mask<0b1010>(c5432.xxyz());
        auto tmp_c4221 = neg_mask<0b0101>(c5432.yww0() + c10__._000x());
        auto tmp_c3100 = neg_mask<0b1010>(c5432.z000() + c10__._0xyy());
        auto const inv_col0 = ((t._col1.yxxx() * tmp_c5543) + (t._col1.zzyy() * tmp_c4221) + (t._col1.wwwz() * tmp_c3100)) * invdet;

        // m.i01 := (i01 * -c5 + i02 *  c4 + i03 * -c3) * invdet;
        // m.i11 := (i00 *  c5 + i02 * -c2 + i03 *  c1) * invdet;
        // m.i21 := (i00 * -c4 + i01 *  c2 + i03 * -c0) * invdet;
        // m.i31 := (i00 *  c3 + i01 * -c1 + i02 *  c0) * invdet;
        tmp_c5543 = -tmp_c5543;
        tmp_c4221 = -tmp_c4221;
        tmp_c3100 = -tmp_c3100;
        auto const inv_col1 = ((t._col0.yxxx() * tmp_c5543) + (t._col0.zzyy() * tmp_c4221) + (t._col0.wwwz() * tmp_c3100)) * invdet;

        // m.i02 := (i31 *  s5 + i32 * -s4 + i33 *  s3) * invdet;
        // m.i12 := (i30 * -s5 + i32 *  s2 + i33 * -s1) * invdet;
        // m.i22 := (i30 *  s4 + i31 * -s2 + i33 *  s0) * invdet;
        // m.i32 := (i30 * -s3 + i31 *  s1 + i32 * -s0) * invdet;
        auto tmp_s5543 = neg_mask<0b1010>(s45__.yyx0() + s0123._000w());
        auto tmp_s4221 = neg_mask<0b0101>(s45__.x000() + s0123._0zzy());
        auto tmp_s3100 = neg_mask<0b1010>(s0123.wyxx());
        auto const inv_col2 = ((t._col3.yxxx() * tmp_s5543) + (t._col3.zzyy() * tmp_s4221) + (t._col3.wwwz() * tmp_s3100)) * invdet;

        // m.i03 := (i21 * -s5 + i22 *  s4 + i23 * -s3) * invdet;
        // m.i13 := (i20 *  s5 + i22 * -s2 + i23 *  s1) * invdet;
        // m.i23 := (i20 * -s4 + i21 *  s2 + i23 * -s0) * invdet;
        // m.i33 := (i20 *  s3 + i21 * -s1 + i22 *  s0) * invdet;
        tmp_s5543 = -tmp_s5543;
        tmp_s4221 = -tmp_s4221;
        tmp_s3100 = -tmp_s3100;
        auto const inv_col3 = ((t._col2.yxxx() * tmp_s5543) + (t._col2.zzyy() * tmp_s4221) + (t._col2.wwwz() * tmp_s3100)) * invdet;

        return matrix2{inv_col0, inv_col1, inv_col2, inv_col3};
    }

private:
    f32x4 _col0;
    f32x4 _col1;
    f32x4 _col2;
    f32x4 _col3;

    template<char Axis>
    [[nodiscard]] constexpr static f32x4 reflect_column() noexcept
    {
        if constexpr (Axis == 'x') {
            return f32x4{1.0f, 0.0f, 0.0f, 0.0f};
        } else if constexpr (Axis == 'X') {
            return f32x4{-1.0f, 0.0f, 0.0f, 0.0f};
        } else if constexpr (Axis == 'y') {
            return f32x4{0.0f, 1.0f, 0.0f, 0.0f};
        } else if constexpr (Axis == 'Y') {
            return f32x4{0.0f, -1.0f, 0.0f, 0.0f};
        } else if constexpr (Axis == 'z') {
            return f32x4{0.0f, 0.0f, 1.0f, 0.0f};
        } else if constexpr (Axis == 'Z') {
            return f32x4{0.0f, 0.0f, -1.0f, 0.0f};
        } else if constexpr (Axis == 'w') {
            return f32x4{0.0f, 0.0f, 0.0f, 1.0f};
        } else if constexpr (Axis == 'W') {
            return f32x4{0.0f, 0.0f, 0.0f, -1.0f};
        } else {
            hi_static_no_default();
        }
    }
};

}} // namespace hi::v1
