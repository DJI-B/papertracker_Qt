//
// Created by JellyfishKnight on 25-5-2.
//

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <algorithm>
#include <vector>
// 使用中值滤波过滤异常值
inline std::vector<double> filterOutliers(std::vector<double>& data) {
    if (data.size() <= 3) return data;  // 数据过少时不过滤

    // 复制数据用于排序（不需要再排序，因为computePercentile会排序）
    std::vector<double> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    // 计算四分位数
    size_t size = sorted_data.size();
    size_t q1_pos = size / 4;
    size_t q3_pos = (size * 3) / 4;
    double q1 = sorted_data[q1_pos];
    double q3 = sorted_data[q3_pos];

    // 计算四分位距
    double iqr = q3 - q1;

    // 设定异常值的界限：Q1 - 1.5*IQR 和 Q3 + 1.5*IQR
    double lower_bound = q1 - 1.5 * iqr;
    double upper_bound = q3 + 1.5 * iqr;

    // 过滤异常值
    std::vector<double> filtered_data;
    filtered_data.reserve(data.size());

    for (double value : sorted_data) {
        if (value >= lower_bound && value <= upper_bound) {
            filtered_data.push_back(value);
        }
    }

    return filtered_data;
}
// 计算 data 列表中第 p 百分位的值（线性插值法），p 的范围是 [0,100]
inline double computePercentile(std::vector<double>& data, double p) {
    if (data.empty())
        return 0.0;  // 空时返回 0 （可根据需求调整）
    std::sort(data.begin(), data.end());
    // 位置 pos = p/100 * (N-1)
    double pos = p / 100.0 * (data.size() - 1);
    size_t idx = static_cast<size_t>(std::floor(pos));
    double frac = pos - idx;
    // 如果越界，直接返回最后一个元素
    if (idx + 1 >= data.size())
        return data[idx];
    // 否则线性插值
    return data[idx] + frac * (data[idx + 1] - data[idx]);
}


inline double clamp(double x, double lo, double hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

template <typename T>
inline T average(const std::vector<T>& vec) {
    if (vec.empty()) return T(0);
    T sum = std::accumulate(vec.begin(), vec.end(), T(0));
    return sum / static_cast<T>(vec.size());
}

#endif //TOOLS_HPP
