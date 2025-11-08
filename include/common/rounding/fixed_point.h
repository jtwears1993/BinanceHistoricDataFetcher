//
// Created by jtwears on 10/25/25.
//

#ifndef BINANCEHISTORICDATAFETCHER_FIXED_POINT_H
#define BINANCEHISTORICDATAFETCHER_FIXED_POINT_H
#include <cmath>
#include <string>

namespace common::rounding {

    class FixedPoint {

    public:

        FixedPoint() = default;

       ~FixedPoint() = default;

        [[nodiscard]] static std::int32_t from_string(const std::string &s, const int &precision) {
            const double value = std::stod(s);
            const double scaled_value = value * calc_scale_factor(precision);
            return static_cast<std::int32_t>(std::round(scaled_value));
        }

        [[nodiscard]] static double to_double(const std::int32_t &fp, const int &precision) {
            return static_cast<double>(fp) / calc_scale_factor(precision);
        }

    private:
        static double calc_scale_factor(const int &precision) {
            return std::pow(10.0, static_cast<double>(precision));
        }
    };
}
#endif //BINANCEHISTORICDATAFETCHER_FIXED_POINT_H