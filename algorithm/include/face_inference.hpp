//
// Created by JellyfishKnight on 2025/2/26.
//

#ifndef FaceInference_HPP
#define FaceInference_HPP

#include "base_inference.hpp"
#include "kalman_filter.hpp"
#include <memory>
#include <vector>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>
#include "logger.hpp"
#include <chrono>

class FaceInference final : public BaseInference {
public:
    FaceInference();

    virtual ~FaceInference() = default;
    void set_offset_map(const std::unordered_map<std::string, float>& offset_map) {
        blendShapeOffsetMap = offset_map;
    }
    // 加载模型
    void load_model(const std::string &model_path) override;

    // 运行推理
    void inference(cv::Mat image) override;

    std::vector<float> get_output() override;

private:
    void init_kalman_filter() override;

    // 预处理图像
    void preprocess(const cv::Mat& input) override;

    // 运行模型
    void run_model() override;

    // 处理结果
    void process_results() override;
    std::unordered_map<std::string, float> blendShapeOffsetMap;
    // 初始化ARKit模型输出的映射表
    void initBlendShapeIndexMap() override;
};

#endif //FaceInference_HPP
