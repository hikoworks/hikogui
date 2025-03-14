// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <tuple>
#include <limits>
#include <algorithm>
#include <vector>
#include <cmath>
#include <span>

export module hikogui_graphic_path_bezier_curve;
import hikogui_container;
import hikogui_geometry;
import hikogui_graphic_path_bezier;
import hikogui_graphic_path_bezier_point;
import hikogui_image;
import hikogui_numeric;
import hikogui_utility;

export namespace hi { inline namespace v1 {

/*! Bezier Curve
 * A linear, quadratic or cubic bezier curve.
 */
export struct bezier_curve {
    enum class Type : uint8_t { None, Linear, Quadratic, Cubic };

    Type type;
    point2 P1; //!< First point
    point2 C1; //!< Control point
    point2 C2; //!< Control point
    point2 P2; //!< Last point

    bezier_curve() noexcept = delete;
    bezier_curve(bezier_curve const& other) noexcept = default;
    bezier_curve(bezier_curve&& other) noexcept = default;
    bezier_curve& operator=(bezier_curve const& other) noexcept = default;
    bezier_curve& operator=(bezier_curve&& other) noexcept = default;

    /*! Construct a linear bezier-curve.
     */
    bezier_curve(point2 const P1, point2 const P2) noexcept : type(Type::Linear), P1(P1), C1(), C2(), P2(P2) {}

    /*! Construct a quadratic bezier-curve.
     */
    bezier_curve(point2 const P1, point2 const C1, point2 const P2) noexcept : type(Type::Quadratic), P1(P1), C1(C1), C2(), P2(P2)
    {
    }

    /*! Construct a cubic bezier-curve.
     */
    bezier_curve(point2 const P1, point2 const C1, point2 const C2, point2 const P2) noexcept :
        type(Type::Cubic), P1(P1), C1(C1), C2(C2), P2(P2)
    {
    }

    /*! Construct a bezier-curve of any type.
     */
    bezier_curve(Type const type, point2 const P1, point2 const C1, point2 const C2, point2 const P2) noexcept :
        type(type), P1(P1), C1(C1), C2(C2), P2(P2)
    {
    }

    /*! Return a point on the bezier-curve.
     * Values of `t` beyond 0.0 and 1.0 will find a point extrapolated beyond the
     * bezier segment.
     *
     * \param t a relative distance between 0.0 (point P1) and 1.0 (point P2).
     * \return the coordinates of the point on the curve.
     */
    [[nodiscard]] point2 pointAt(float const t) const noexcept
    {
        switch (type) {
        case Type::Linear:
            return bezierPointAt(P1, P2, t);
        case Type::Quadratic:
            return bezierPointAt(P1, C1, P2, t);
        case Type::Cubic:
            return bezierPointAt(P1, C1, C2, P2, t);
        default:
            hi_no_default();
        }
    }

    /*! Return a tangent on the bezier-curve.
     * Values of `t` beyond 0.0 and 1.0 will find a point extrapolated beyond the
     * bezier segment.
     *
     * \param t a relative distance between 0.0 (point P1) and 1.0 (point P2).
     * \return the tangent-vector at point t on the curve
     */
    [[nodiscard]] constexpr vector2 tangentAt(float const t) const noexcept
    {
        switch (type) {
        case Type::Linear:
            return bezierTangentAt(P1, P2, t);
        case Type::Quadratic:
            return bezierTangentAt(P1, C1, P2, t);
        case Type::Cubic:
            return bezierTangentAt(P1, C1, C2, P2, t);
        default:
            hi_no_default();
        }
    }

    /*! Return the x values where the curve crosses the y-axis.
     * \param y y-axis.
     * \return 0 to 3, or infinite number of x values.
     */
    [[nodiscard]] lean_vector<float> solveXByY(float const y) const noexcept
    {
        switch (type) {
        case Type::Linear:
            return bezierFindX(P1, P2, y);
        case Type::Quadratic:
            return bezierFindX(P1, C1, P2, y);
        case Type::Cubic:
            return bezierFindX(P1, C1, C2, P2, y);
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] hi_force_inline lean_vector<float> solveTForNormalsIntersectingPoint(point2 P) const noexcept
    {
        switch (type) {
        case Type::Linear:
            return bezierFindTForNormalsIntersectingPoint(P1, P2, P);
        case Type::Quadratic:
            return bezierFindTForNormalsIntersectingPoint(P1, C1, P2, P);
        case Type::Cubic:
            hi_no_default();
        default:
            hi_no_default();
        }
    }

    struct sdf_distance_result {
        /** The vector between P and N.
         */
        vector2 PN;

        bezier_curve const *curve = nullptr;

        /** Linear position on the curve-segment, 0.0 and 1.0 are end-points.
         */
        float t = 0.0f;

        /** The square distance between P and N.
         */
        float sq_distance = std::numeric_limits<float>::max();

        constexpr sdf_distance_result() noexcept = default;
        constexpr sdf_distance_result(sdf_distance_result const&) noexcept = default;
        constexpr sdf_distance_result(sdf_distance_result&&) noexcept = default;
        constexpr sdf_distance_result& operator=(sdf_distance_result const&) noexcept = default;
        constexpr sdf_distance_result& operator=(sdf_distance_result&&) noexcept = default;
        constexpr sdf_distance_result(bezier_curve const *curve) noexcept : curve(curve) {}

        /** The orthogonality of the line PN and the tangent of the curve at N.
         */
        [[nodiscard]] hi_force_inline constexpr float orthogonality() const noexcept
        {
            hilet tangent = curve->tangentAt(t);
            return cross(normalize(tangent), normalize(PN));
        };

        [[nodiscard]] hi_force_inline float distance() const noexcept
        {
            return std::sqrt(sq_distance);
        }

        [[nodiscard]] hi_force_inline float signed_distance() const noexcept
        {
            hilet d = distance();
            return orthogonality() < 0.0 ? d : -d;
        }

        [[nodiscard]] hi_force_inline constexpr bool operator<(sdf_distance_result const& rhs) const noexcept
        {
            if (abs(sq_distance - rhs.sq_distance) < 0.01f) {
                return abs(orthogonality()) > abs(rhs.orthogonality());
            } else {
                return sq_distance < rhs.sq_distance;
            }
        }
    };

    /** Find the distance from the point to the curve.
     *
     * If the distances are equal between two curves, take the one with a maximum orthogonality.
     * If the orthogonality >= then the point is inside that edge.
     *
     * @param P The point from which to calculate the distance to this curve.
     * @return squared distance from curve, orthogonality.
     */
    [[nodiscard]] sdf_distance_result sdf_distance(point2 P) const noexcept
    {
        auto nearest = sdf_distance_result{this};

        hilet ts = solveTForNormalsIntersectingPoint(P);
        for (auto t : ts) {
            t = std::clamp(t, 0.0f, 1.0f);

            hilet PN = P - pointAt(t);
            hilet sq_distance = squared_hypot(PN);
            if (sq_distance < nearest.sq_distance) {
                nearest.t = t;
                nearest.PN = PN;
                nearest.sq_distance = sq_distance;
            }
        }

        return nearest;
    }

    /*! Split a cubic bezier-curve into two cubic bezier-curve.
     * \param t a relative distance between 0.0 (point P1) and 1.0 (point P2)
     *        where to split the curve.
     * \return two cubic bezier-curves.
     */
    [[nodiscard]] std::pair<bezier_curve, bezier_curve> cubicSplit(float const t) const noexcept
    {
        hilet outerA = bezier_curve{P1, C1};
        hilet outerBridge = bezier_curve{C1, C2};
        hilet outerB = bezier_curve{C2, P2};

        hilet innerA = bezier_curve{outerA.pointAt(t), outerBridge.pointAt(t)};
        hilet innerB = bezier_curve{outerBridge.pointAt(t), outerB.pointAt(t)};

        hilet newPoint = bezier_curve{innerA.pointAt(t), innerB.pointAt(t)}.pointAt(t);

        return {{P1, outerA.pointAt(t), innerA.pointAt(t), newPoint}, {newPoint, innerB.pointAt(t), outerB.pointAt(t), P2}};
    }

    /*! Split a quadratic bezier-curve into two quadratic bezier-curve.
     * \param t a relative distance between 0.0 (point P1) and 1.0 (point P2)
     *        where to split the curve.
     * \return two quadratic bezier-curves.
     */
    [[nodiscard]] std::pair<bezier_curve, bezier_curve> quadraticSplit(float const t) const noexcept
    {
        hilet outerA = bezier_curve{P1, C1};
        hilet outerB = bezier_curve{C1, P2};

        hilet newPoint = bezier_curve{outerA.pointAt(t), outerB.pointAt(t)}.pointAt(t);

        return {{P1, outerA.pointAt(t), newPoint}, {newPoint, outerB.pointAt(t), P2}};
    }

    /*! Split a linear bezier-curve into two linear bezier-curve.
     * \param t a relative distance between 0.0 (point P1) and 1.0 (point P2)
     *        where to split the curve.
     * \return two linear bezier-curves.
     */
    [[nodiscard]] std::pair<bezier_curve, bezier_curve> linearSplit(float const t) const noexcept
    {
        hilet newPoint = pointAt(t);

        return {{P1, newPoint}, {newPoint, P2}};
    }

    /*! Split a bezier-curve into two bezier-curve of the same type.
     * \param t a relative distance between 0.0 (point P1) and 1.0 (point P2)
     *        where to split the curve.
     * \return two bezier-curves.
     */
    [[nodiscard]] std::pair<bezier_curve, bezier_curve> split(float const t) const noexcept
    {
        switch (type) {
        case Type::Linear:
            return linearSplit(t);
        case Type::Quadratic:
            return quadraticSplit(t);
        case Type::Cubic:
            return cubicSplit(t);
        default:
            hi_no_default();
        }
    }

    /*! Subdivide a bezier-curve until each are flat enough.
     * \param r resulting list of linear segments.
     * \param minimumFlatness minimum amount of flatness of the resulting curve segments.
     */
    void subdivideUntilFlat_impl(std::vector<bezier_curve>& r, float const minimumFlatness) const noexcept
    {
        if (flatness() >= minimumFlatness) {
            r.push_back(*this);
        } else {
            hilet[a, b] = split(0.5f);
            a.subdivideUntilFlat_impl(r, minimumFlatness);
            b.subdivideUntilFlat_impl(r, minimumFlatness);
        }
    }

    /*! Subdivide a bezier-curve until each are flat enough.
     * \param tolerance maximum amount of curviness.
     * \return resulting list of curve segments.
     */
    [[nodiscard]] std::vector<bezier_curve> subdivideUntilFlat(float const tolerance) const noexcept
    {
        std::vector<bezier_curve> r;
        subdivideUntilFlat_impl(r, 1.0f - tolerance);
        return r;
    }

    /*! Return the flatness of a curve.
     * \return 1.0 when completely flat, < 1.0 when curved.
     */
    [[nodiscard]] float flatness() const noexcept
    {
        switch (type) {
        case Type::Linear:
            return bezierFlatness(P1, P2);
        case Type::Quadratic:
            return bezierFlatness(P1, C1, P2);
        case Type::Cubic:
            return bezierFlatness(P1, C1, C2, P2);
        default:
            hi_no_default();
        }
    }

    /*! Return a line-segment from a curve at a certain distance.
     * \param offset positive means the parallel line will be on the starboard of the curve.
     * \return line segment offset from the curve.
     */
    [[nodiscard]] bezier_curve toParallelLine(float const offset) const noexcept
    {
        hilet[newP1, newP2] = parallelLine(P1, P2, offset);
        return {newP1, newP2};
    }

    [[nodiscard]] friend bool operator==(bezier_curve const& lhs, bezier_curve const& rhs) noexcept
    {
        if (lhs.type != rhs.type) {
            return false;
        }
        switch (lhs.type) {
        case bezier_curve::Type::Linear:
            return (lhs.P1 == rhs.P1) && (lhs.P2 == rhs.P2);
        case bezier_curve::Type::Quadratic:
            return (lhs.P1 == rhs.P1) && (lhs.C1 == rhs.C1) && (lhs.P2 == rhs.P2);
        case bezier_curve::Type::Cubic:
            return (lhs.P1 == rhs.P1) && (lhs.C1 == rhs.C1) && (lhs.C2 == rhs.C2) && (lhs.P2 == rhs.P2);
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] friend bezier_curve operator*(transformer2 auto const& lhs, bezier_curve const& rhs) noexcept
    {
        return {rhs.type, lhs * rhs.P1, lhs * rhs.C1, lhs * rhs.C2, lhs * rhs.P2};
    }

    /*! Reverse direction of a curve.
     */
    [[nodiscard]] friend bezier_curve operator~(bezier_curve const& rhs) noexcept
    {
        return {rhs.type, rhs.P2, rhs.C2, rhs.C1, rhs.P1};
    }
};

namespace detail {

[[nodiscard]] constexpr std::vector<float> solveCurvesXByY(std::vector<bezier_curve> const& v, float y) noexcept
{
    std::vector<float> r;
    r.reserve(v.size());

    for (hilet& curve : v) {
        hilet xValues = curve.solveXByY(y);
        for (hilet x : xValues) {
            r.push_back(x);
        }
    }
    return r;
}

[[nodiscard]] constexpr std::optional<std::vector<std::pair<float, float>>>
getFillSpansAtY(std::vector<bezier_curve> const& v, float y) noexcept
{
    auto xValues = solveCurvesXByY(v, y);

    // Sort x values, each pair is a span.
    std::sort(xValues.begin(), xValues.end());

    // End-to-end connected curves will yield duplicate values.
    hilet uniqueEnd = std::unique(xValues.begin(), xValues.end());

    // After removing duplicates, we should end up with pairs of x values.
    std::size_t const uniqueValueCount = (uniqueEnd - xValues.begin());

    if (uniqueValueCount % 2 != 0) {
        // Something is wrong in solving the curves. Probably numeric instability.
        // In any case, just ignore this sample.
        return {};
    }

    // Create pairs of values.
    auto r = std::vector<std::pair<float, float>>{};
    r.reserve(uniqueValueCount / 2);
    for (std::size_t i = 0; i < uniqueValueCount; i += 2) {
        r.emplace_back(xValues[i], xValues[i + 1]);
    }
    return r;
}

constexpr void fillPartialPixels(std::span<uint8_t> row, ssize_t const i, float const startX, float const endX) noexcept
{
    hilet pixelCoverage = std::clamp(endX, i + 0.0f, i + 1.0f) - std::clamp(startX, i + 0.0f, i + 1.0f);

    auto& pixel = row[i];
    pixel = static_cast<uint8_t>(std::min(pixelCoverage * 51.0f + pixel, 255.0f));
}

constexpr void fillFullPixels(std::span<uint8_t> row, ssize_t const start, ssize_t const size) noexcept
{
    if (size < 16) {
        hilet end = start + size;
        for (ssize_t i = start; i < end; i++) {
            row[i] += 0x33;
        }
    } else {
        auto u8p = &row[start];
        hilet u8end = u8p + size;

        // First add 51 to all pixels up to the alignment.
        hilet alignedStart = hi::ceil(u8p, sizeof(uint64_t));
        while (u8p < alignedStart) {
            *(u8p++) += 0x33;
        }

        // add 51 for each pixel, 8 pixels at a time.
        auto u64p = reinterpret_cast<uint64_t *>(u8p);
        auto const *const u64end = reinterpret_cast<uint64_t const *>(hi::floor(u8end, sizeof(uint64_t)));
        while (u64p < u64end) {
            *(u64p++) += 0x3333333333333333ULL;
        }

        // Add 51 to the last pixels.
        u8p = reinterpret_cast<uint8_t *>(u64p);
        while (u8p < u8end) {
            *(u8p++) += 0x33;
        }
    }
}

/*! Render pixels in a row between two x values.
 * Fully covered sub-pixel will have the value 51.
 */
constexpr void fillRowSpan(std::span<uint8_t> row, float const startX, float const endX) noexcept
{
    if (startX >= row.size() || endX < 0.0f) {
        return;
    }

    hilet startX_int = floor_cast<std::size_t>(startX);
    hilet endXplusOne = endX + 1.0f;
    hilet endX_int = floor_cast<std::size_t>(endXplusOne);
    hilet startColumn = std::max(startX_int, std::size_t{0});
    hilet endColumn = std::min(endX_int, row.size());
    hilet nrColumns = endColumn - startColumn;

    if (nrColumns == 1) {
        fillPartialPixels(row, startColumn, startX, endX);
    } else {
        fillPartialPixels(row, startColumn, startX, endX);
        fillFullPixels(row, startColumn + 1, nrColumns - 2);
        fillPartialPixels(row, endColumn - 1, startX, endX);
    }
}

constexpr void fillRow(std::span<uint8_t> row, std::size_t rowY, std::vector<bezier_curve> const& curves) noexcept
{
    // 5 times super sampling.
    for (float y = rowY + 0.1f; y < (rowY + 1); y += 0.2f) {
        auto optionalSpans = getFillSpansAtY(curves, y);
        if (!optionalSpans) {
            // try again, with a slight offset.
            optionalSpans = getFillSpansAtY(curves, y + 0.01f);
        }

        if (optionalSpans) {
            hilet& spans = optionalSpans.value();

            for (hilet& span : spans) {
                fillRowSpan(row, span.first, span.second);
            }
        }
    }
}

[[nodiscard]] constexpr float generate_sdf_r8_pixel(point2 point, std::vector<bezier_curve> const& curves) noexcept
{
    if (curves.empty()) {
        return -std::numeric_limits<float>::max();
    }

    auto it = curves.cbegin();
    auto nearest = (it++)->sdf_distance(point);

    for (; it != curves.cend(); ++it) {
        hilet distance = it->sdf_distance(point);

        if (distance < nearest) {
            nearest = distance;
        }
    }

    return nearest.signed_distance();
}

} // namespace detail

/** Make a contour of Bezier curves from a list of points.
 * The contour is also colorized to be used for creating multichannel-signed-distance-fields.
 *
 * @param first Iterator to the first point in a list
 * @param last Iterator one beyond the last point in a list
 */
[[nodiscard]] constexpr std::vector<bezier_curve>
makeContourFromPoints(std::vector<bezier_point>::const_iterator begin, std::vector<bezier_point>::const_iterator end) noexcept
{
    hilet points = bezier_point::normalizePoints(begin, end);

    std::vector<bezier_curve> r;

    auto type = bezier_curve::Type::None;
    auto P1 = point2{};
    auto C1 = point2{};
    auto C2 = point2{};

    for (hilet& point : points) {
        switch (point.type) {
        case bezier_point::Type::Anchor:
            switch (type) {
            case bezier_curve::Type::None:
                P1 = point.p;
                type = bezier_curve::Type::Linear;
                break;
            case bezier_curve::Type::Linear:
                r.emplace_back(P1, point.p);
                P1 = point.p;
                type = bezier_curve::Type::Linear;
                break;
            case bezier_curve::Type::Quadratic:
                r.emplace_back(P1, C1, point.p);
                P1 = point.p;
                type = bezier_curve::Type::Linear;
                break;
            case bezier_curve::Type::Cubic:
                r.emplace_back(P1, C1, C2, point.p);
                P1 = point.p;
                type = bezier_curve::Type::Linear;
                break;
            default:
                hi_no_default();
            }
            break;
        case bezier_point::Type::QuadraticControl:
            C1 = point.p;
            type = bezier_curve::Type::Quadratic;
            break;
        case bezier_point::Type::CubicControl1:
            C1 = point.p;
            type = bezier_curve::Type::Cubic;
            break;
        case bezier_point::Type::CubicControl2:
            C2 = point.p;
            hi_assert(type == bezier_curve::Type::Cubic);
            break;
        default:
            hi_no_default();
        }
    }

    return r;
}

/** Inverse a contour.
 * Reverse the direction of the whole contour, turning it inside out.
 * This is useful for creating a stroke, by inverting the inner offset contour.
 * @param contour contour to reverse.
 * @return the reversed contour.
 */
[[nodiscard]] constexpr std::vector<bezier_curve> makeInverseContour(std::vector<bezier_curve> const& contour) noexcept
{
    auto r = std::vector<bezier_curve>{};
    r.reserve(contour.size());

    for (auto i = contour.rbegin(); i != contour.rend(); i++) {
        r.push_back(~(*i));
    }

    return r;
}

/*! Make a contour of Bezier curves from another contour of Bezier curves at a offset.
 * Make a new contour made out of line-segments offset from the original curve. After
 * offsetting the line segment the line segments are properly cut or extended to
 * cover all intersections and gaps.
 *
 * \param contour a list of bezier curve segments forming a closed contour.
 * \param offset positive means the parallel contour will be on the starboard side of the given contour.
 * \param line_join_style how the gaps between line segments are joined together.
 * \param tolerance to how curved the new contour should look.
 */
[[nodiscard]] constexpr std::vector<bezier_curve> makeParallelContour(
    std::vector<bezier_curve> const& contour,
    float offset,
    hi::line_join_style line_join_style,
    float tolerance) noexcept
{
    auto contourAtOffset = std::vector<bezier_curve>{};
    for (hilet& curve : contour) {
        for (hilet& flatCurve : curve.subdivideUntilFlat(tolerance)) {
            contourAtOffset.push_back(flatCurve.toParallelLine(offset));
        }
    }

    // The resulting path now consists purely of line-segments that may have gaps and overlaps.
    // This needs to be repaired.
    std::optional<point2> intersectPoint;
    auto r = std::vector<bezier_curve>{};
    for (hilet& curve : contourAtOffset) {
        if (r.size() == 0) {
            r.push_back(curve);

        } else if (r.back().P2 == curve.P1) {
            r.push_back(curve);

        } else if ((intersectPoint = getIntersectionPoint(r.back().P1, r.back().P2, curve.P1, curve.P2))) {
            r.back().P2 = intersectPoint.value();
            r.push_back(curve);
            r.back().P1 = intersectPoint.value();

        } else if (
            line_join_style == line_join_style::miter and
            to_bool(intersectPoint = getExtrapolatedIntersectionPoint(r.back().P1, r.back().P2, curve.P1, curve.P2))) {
            r.back().P2 = intersectPoint.value();
            r.push_back(curve);
            r.back().P1 = intersectPoint.value();

        } else {
            r.emplace_back(r.back().P2, curve.P1);
            r.push_back(curve);
        }
    }

    // Repair the endpoints of the contour as well.
    if (r.size() > 0 && r.back().P2 != r.front().P1) {
        if ((intersectPoint = getIntersectionPoint(r.back().P1, r.back().P2, r.front().P1, r.front().P2))) {
            r.back().P2 = r.front().P1 = intersectPoint.value();
        } else {
            r.emplace_back(r.back().P2, r.front().P1);
        }
    }

    return r;
}

/** Fill a linear gray scale image by filling a curve with anti-aliasing.
 * @param image An alpha-channel image to make opaque where pixel is inside the contours
 * @param curves All curves of path, in no particular order.
 */
constexpr void fill(pixmap_span<uint8_t> image, std::vector<bezier_curve> const& curves) noexcept
{
    for (auto y = 0_uz; y < image.height(); y++) {
        detail::fillRow(image[y], y, curves);
    }
}

/** Fill a signed distance field image from the given contour.
 * @param image An signed-distance-field which show distance toward the closest curve
 * @param curves All curves of path, in no particular order.
 */
constexpr void fill(pixmap_span<sdf_r8> image, std::vector<bezier_curve> const& curves) noexcept
{
    for (auto row_nr = 0_uz; row_nr != image.height(); ++row_nr) {
        hilet row = image[row_nr];
        hilet y = static_cast<float>(row_nr);
        for (auto column_nr = 0_uz; column_nr != image.width(); ++column_nr) {
            hilet x = static_cast<float>(column_nr);
            row[column_nr] = detail::generate_sdf_r8_pixel(point2(x, y), curves);
        }
    }
}

}} // namespace hi::v1
