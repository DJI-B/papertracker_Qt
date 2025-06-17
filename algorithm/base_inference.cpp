//
// Created by JellyfishKnight on 25-4-18.
//
#include "base_inference.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

void BaseInference::set_dt(float dt)
{
    this->dt = dt;
}

void BaseInference::set_use_filter(bool use)
{
    use_filter = use;
}

void BaseInference::set_q_factor(float factor)
{
    q_factor = factor;
}

void BaseInference::set_r_factor(float factor)
{
    r_factor = factor;
}

bool BaseInference::use_filter_status() const
{
    return use_filter;
}

void BaseInference::set_amp_map(const std::unordered_map<std::string, int>& amp_map)
{
    blendShapeAmpMap = amp_map;
}

const std::unordered_map<std::string, size_t>& BaseInference::getBlendShapeIndexMap()
{
    return blendShapeIndexMap;
}

void BaseInference::plot_curve(float raw, float filtered)
{
    // 限制数据点数量
    if (raw_data.size() >= max_points) {
        raw_data.erase(raw_data.begin());       // 删除最旧数据
        filtered_data.erase(filtered_data.begin());
    }

    // 追加新数据
    raw_data.push_back(raw);
    filtered_data.push_back(filtered);

    // 创建画布
    int width = 800, height = 400;
    cv::Mat plot = cv::Mat::zeros(height, width, CV_8UC3);
    plot.setTo(cv::Scalar(30, 30, 30));  // 设置深灰色背景

    // 计算最大最小值，用于归一化到画布
    float min_val = *std::min_element(raw_data.begin(), raw_data.end());
    float max_val = *std::max_element(raw_data.begin(), raw_data.end());
    min_val = std::min(min_val, *std::min_element(filtered_data.begin(), filtered_data.end()));
    max_val = std::max(max_val, *std::max_element(filtered_data.begin(), filtered_data.end()));

    float range = max_val - min_val;
    if (range < 1e-6) range = 1.0f;  // 避免除零问题

    int step = width / max_points;  // x 轴步长
    int y_offset = height - 50;     // y 轴偏移量

    cv::Scalar raw_color(0, 0, 255);       // 原始数据：红色
    cv::Scalar filtered_color(0, 255, 0);  // 滤波数据：绿色

    // 绘制曲线
    for (size_t i = 1; i < raw_data.size(); i++) {
        int x1 = (i - 1) * step;
        int x2 = i * step;

        int y1_raw = y_offset - ((raw_data[i - 1] - min_val) / range) * (height - 100);
        int y2_raw = y_offset - ((raw_data[i] - min_val) / range) * (height - 100);

        int y1_filtered = y_offset - ((filtered_data[i - 1] - min_val) / range) * (height - 100);
        int y2_filtered = y_offset - ((filtered_data[i] - min_val) / range) * (height - 100);

        cv::line(plot, cv::Point(x1, y1_raw), cv::Point(x2, y2_raw), raw_color, 2);        // 画原始数据线
        cv::line(plot, cv::Point(x1, y1_filtered), cv::Point(x2, y2_filtered), filtered_color, 2);  // 画滤波后数据线
    }

    // 显示图像
    cv::imshow("Filtered Comparison", plot);
    cv::waitKey(1);
}

// 将原始数据放大增益
void BaseInference::AmpMapToOutput(std::vector<float>& output)
{
    for (int i = 0; i < blendShapes.size(); i++)
    {
        if (blendShapeAmpMap.contains(blendShapes[i]))
        {
            if (blendShapeAmpMap[blendShapes[i]] != 0)
            {
                output[i] = output[i] * (blendShapeAmpMap[blendShapes[i]] * 0.02 + 1);
                output[i] = std::min(1.0f, output[i]);
            }
        }
        // 对tongueRight进行额外的2.5倍放大处理
        if (blendShapes[i] == "tongueRight") {
            output[i] = output[i] * 2.5f;
            // 确保值不超过1.0（如果需要限制在[0,1]范围内）
            output[i] = std::min(1.0f, output[i]);
        }
        // // 应用偏置值
        // if (blendShapeOffsetMap.contains(blendShapes[i]))
        // {
        //     // 应用偏置值，确保结果在0-1之间
        //     output[i] = std::max(0.0f, std::min(1.0f, output[i] + blendShapeOffsetMap[blendShapes[i]]));
        // }
    }
}

void BaseInference::init_io_names() {
    input_names_.clear();
    output_names_.clear();
    input_name_ptrs_.clear();
    output_name_ptrs_.clear();

    Ort::AllocatorWithDefaultOptions allocator;

    // 获取输入信息
    size_t num_input_nodes = session_->GetInputCount();

    for (size_t i = 0; i < num_input_nodes; i++) {
        auto name = session_->GetInputNameAllocated(i, allocator);
        input_names_.push_back(name.get());

        // 获取输入形状
        auto type_info = session_->GetInputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
        auto shape = tensor_info.GetShape();

        // 动态维度处理
        for (size_t j = 0; j < shape.size(); j++) {
            if (shape[j] < 0) {
                if (j == 0) shape[j] = 1;        // 批次大小
                else if (j == 1) shape[j] = 1;   // 通道数(灰度)
                else if (j == 2) shape[j] = 256; // 高度
                else if (j == 3) shape[j] = 256; // 宽度
            }
        }

        input_shapes_.push_back(shape);
    }

    // 更新指针数组 - 只需要执行一次
    for (const auto& name : input_names_) {
        input_name_ptrs_.push_back(name.c_str());
    }

    // 获取输出信息
    size_t num_output_nodes = session_->GetOutputCount();

    for (size_t i = 0; i < num_output_nodes; i++) {
        auto name = session_->GetOutputNameAllocated(i, allocator);
        output_names_.push_back(name.get());
    }

    // 更新指针数组 - 只需要执行一次
    for (const auto& name : output_names_) {
        output_name_ptrs_.push_back(name.c_str());
    }

    // 设置输入维度信息
    if (!input_shapes_.empty()) {
        auto& shape = input_shapes_[0];
        if (shape.size() >= 4) {
            input_c_ = shape[1];
            input_h_ = shape[2];
            input_w_ = shape[3];
        }
    }
}

void BaseInference::allocate_buffers()
{
    // 预分配处理图像的内存
    processed_image_ = cv::Mat(input_h_, input_w_, CV_32FC1);

    // 预分配输入数据内存
    if (!input_shapes_.empty()) {
        size_t input_size = 1;
        for (auto dim : input_shapes_[0]) {
            input_size *= dim;
        }
        input_data_.resize(input_size);
    }

    // 创建内存信息 - 只需创建一次
    memory_info_ = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator,
        OrtMemType::OrtMemTypeDefault
    );
}

