//
// Created by JellyfishKnight on 25-4-18.
//

#ifndef BASE_INFERENCE_HPP
#define BASE_INFERENCE_HPP
#include <fstream>
#include <kalman_filter.hpp>
#include <opencv2/core.hpp>
#include <onnxruntime_cxx_api.h>
#include <string>

inline bool file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

inline std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring result;
    result.assign(str.begin(), str.end());
    return result;
}

class BaseInference
{
public:
    virtual ~BaseInference() = default;

    virtual void inference(cv::Mat image) = 0;

    virtual void load_model(const std::string &model_path) = 0;

    virtual std::vector<float> get_output() = 0;

    void set_amp_map(const std::unordered_map<std::string, int>& amp_map);

    const std::unordered_map<std::string, size_t>& getBlendShapeIndexMap();

    void set_use_filter(bool use);

    void set_dt(float dt);

    void set_q_factor(float factor);

    void set_r_factor(float factor);

    bool use_filter_status() const;
protected:
    virtual void init_kalman_filter() = 0;

    // 预处理图像
    virtual void preprocess(const cv::Mat& input) = 0;

    // 运行模型
    virtual void run_model() = 0;

    // 处理结果
    virtual void process_results() = 0;

    void plot_curve(float raw, float filtered);

    // 初始化ARKit模型输出的映射表
    virtual void initBlendShapeIndexMap() = 0;

    // 将原始数据放大增益
    void AmpMapToOutput(std::vector<float>& output);

    // 初始化输入输出名称
    void init_io_names();

    // 预分配所有缓冲区
    void allocate_buffers();


    // 会话和会话选项
    std::shared_ptr<Ort::Session> session_;
    Ort::SessionOptions session_options;

    std::shared_ptr<Ort::IoBinding> io_binding_;

    // 输入输出名称
    std::vector<std::string> input_names_;
    std::vector<std::string> output_names_;
    std::vector<const char*> input_name_ptrs_;
    std::vector<const char*> output_name_ptrs_;

    // 保存ARKit模型输出的映射表
    std::unordered_map<std::string, size_t> blendShapeIndexMap;
    std::unordered_map<std::string, int> blendShapeAmpMap;
    std::vector<std::string> blendShapes;

    // 输入形状
    std::vector<std::vector<int64_t>> input_shapes_;

    // 输入尺寸
    int input_h_{};
    int input_w_{};
    int input_c_{};

    // ONNX Runtime资源
    Ort::MemoryInfo memory_info_{nullptr};
    Ort::Value input_tensor_{nullptr};
    std::vector<Ort::Value> output_tensors_;
    std::vector<float> input_data_; // 输入数据缓冲区

    // 预分配的缓冲区
    cv::Mat gray_image_;           // 灰度转换缓冲区
    cv::Mat processed_image_;      // 预处理图像缓冲区

    bool use_filter = false;
    bool last_use_filter = use_filter;
    KalmanFilter kalman_filter_;
    std::vector<float> raw_data;       // 存储原始数据
    std::vector<float> filtered_data;  // 存储滤波后数据
    int max_points = 200;        // 只保留最近 200 个点，防止图像过长

    float dt = 0.02f;
    float q_factor = 5e-1f;
    float r_factor = 5e-5f;
};


#endif //BASE_INFERENCE_HPP
