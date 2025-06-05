//
// Created by JellyfishKnight on 25-4-18.
//
#include "eye_inference.hpp"

#include <logger.hpp>
#include <opencv2/imgproc.hpp>

#include <QFile>  // 用于读取资源文件
#include <QString>  // 用于字符串转换
EyeInference::EyeInference()
{
    input_h_ = 112;
    input_w_ = 112;
    input_c_ = 1;
    // 设置卡尔曼滤波参数
    dt = 0.02f;        // 假设50fps，则dt=0.02
    q_factor = 5.0f;   // 过程噪声系数
    r_factor = 0.0003f;  // 测量噪声系数
    EyeInference::init_kalman_filter();
    // 初始化BlendShape索引映射
    EyeInference::initBlendShapeIndexMap();
    // 默认开启滤波
    use_filter = true;
    last_use_filter = use_filter;
}


void EyeInference::inference(cv::Mat image) {
    if (!image.empty()) {
        // 预处理图像 - 直接修改预分配的内存
        preprocess(image);
        // 运行模型
        run_model();
        // 处理结果
        process_results();
    }
}
std::vector<float> EyeInference::get_output() {
    if (output_tensors_.empty()) {
        return {};
    }

    try {
        // 获取第一个输出张量
        const Ort::Value& output_tensor = output_tensors_.front();
        const auto* output_data = output_tensor.GetTensorData<float>();

        // 获取输出形状
        auto shape_info = output_tensor.GetTensorTypeAndShapeInfo();
        auto output_shape = shape_info.GetShape();

        // 计算输出大小
        size_t output_size = 1;
        for (auto dim : output_shape) {
            output_size *= dim;
        }

        // 复制数据到结果向量
        std::vector<float> result(output_data, output_data + output_size);
        result.resize(EYE_OUTPUT_SIZE); // 确保输出大小正确

        if (use_filter)
        {
#ifdef DEBUG
            float raw = 0;
            float eye_openness = 0;
            if (!result.empty())
            {
                raw = result[0];
                // 计算眼睛开合度 - 类似于eye_tracker_window.cpp中的计算方式
                if (result.size() >= 5) {  // 确保有足够的数据点
                    float dist_1 = cv::norm(cv::Point2f(result[1], result[2]) - cv::Point2f(result[3], result[4]));
                    float dist_2 = cv::norm(cv::Point2f(result[2], result[3]) - cv::Point2f(result[4], result[5]));
                    eye_openness = (dist_1 + dist_2) / 2;
                }
            }
#endif
            if (last_use_filter != use_filter)
            {
                last_use_filter = use_filter;
                // 使用CV_64F类型创建矩阵
                cv::Mat input(result.size(), 1, CV_64F);
                for (size_t i = 0; i < result.size(); ++i) {
                    input.at<double>(i, 0) = static_cast<double>(result[i]);
                }
                kalman_filter_.set_state(input.clone());
            }

            // 预测
            auto temp = kalman_filter_.predict();

            if (!result.empty())
            {
                // 创建一个CV_64F类型的测量矩阵
                cv::Mat measure(result.size(), 1, CV_64F);
                for (size_t i = 0; i < result.size(); ++i) {
                    measure.at<double>(i, 0) = static_cast<double>(result[i]);
                }

                // 更新滤波器
                kalman_filter_.correct(measure);

                // 将状态后验矩阵中的数据复制回result
                for (int i = 0; i < result.size(); ++i) {
                    result[i] = static_cast<float>(kalman_filter_.state_post_.at<double>(i, 0));
                }
            }
            else
            {
                // 将预测结果复制回result
                for (int i = 0; i < result.size(); ++i) {
                    result[i] = static_cast<float>(temp.at<double>(i, 0));
                }
            }

#ifdef DEBUG
            if (!result.empty())
            {
                float filtered = result[0];
                // 再次计算滤波后的眼睛开合度
                float filtered_eye_openness = 0;
                if (result.size() >= 5) {
                    float dist_1 = cv::norm(cv::Point2f(result[1], result[2]) - cv::Point2f(result[3], result[4]));
                    float dist_2 = cv::norm(cv::Point2f(result[2], result[3]) - cv::Point2f(result[4], result[5]));
                    filtered_eye_openness = (dist_1 + dist_2) / 2;
                }
                // 显示眼睛开合度的曲线，而不是原始数据
                plot_curve(eye_openness, filtered_eye_openness);
            }
#endif
        }

        // 输出限幅以及增益调整
        // AmpMapToOutput(result);

        return result;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("获取输出数据错误: {}", e.what());
        return std::vector<float>();
    }
}
void EyeInference::load_model(const std::string &model_path) {
    try {
        std::string actual_model_path = ":/models/model/eye_model.onnx";

        if (actual_model_path.substr(0, 2) == ":/") {
            // 从Qt资源加载
            //LOG_INFO("从Qt资源加载模型...");

            QFile resourceFile(QString::fromStdString(actual_model_path));
            if (!resourceFile.open(QIODevice::ReadOnly)) {
                LOG_ERROR("无法打开资源文件: {}", actual_model_path);
                LOG_ERROR("资源文件是否存在: {}", QFile::exists(QString::fromStdString(actual_model_path)) ? "是" : "否");
                return;
            }

            QByteArray modelData = resourceFile.readAll();
            resourceFile.close();

            // 创建环境 - 只需要创建一次
            static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ONNXRuntimeDemo");

            // 配置会话选项
            session_options.SetIntraOpNumThreads(2);
            session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
            session_options.AddConfigEntry("session.intra_op.allow_spinning", "0");
            session_options.EnableCpuMemArena();
            session_options.DisableMemPattern();

            // 使用内存数据创建会话
            session_ = std::make_shared<Ort::Session>(
                env,
                reinterpret_cast<const void*>(modelData.constData()),
                static_cast<size_t>(modelData.size()),
                session_options);
        } else {
            // 从文件系统加载（保留原有代码的兼容性）
            // LOG_INFO("从文件系统加载眼睛模型: {}", actual_model_path);
            //
            // if (!file_exists(actual_model_path)) {
            //     LOG_ERROR("错误：模型文件不存在: {}", actual_model_path);
            //     return;
            // }

            // 创建环境 - 只需要创建一次
            static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ONNXRuntimeDemo");

            // 配置会话选项
            session_options.SetIntraOpNumThreads(2);
            session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
            session_options.AddConfigEntry("session.intra_op.allow_spinning", "0");
            session_options.EnableCpuMemArena();
            session_options.DisableMemPattern();

            // 转换为宽字符路径
            std::wstring wmodel_path = utf8_to_wstring(actual_model_path);

            // 创建会话
            session_ = std::make_shared<Ort::Session>(env, wmodel_path.c_str(), session_options);
        }

        io_binding_ = std::make_shared<Ort::IoBinding>(*session_);

        // 初始化输入和输出名称
        init_io_names();

        // 提前分配内存
        allocate_buffers();

        LOG_INFO("眼睛模型加载完成");
    } catch (const Ort::Exception& e) {
        LOG_ERROR("ONNX Runtime 错误: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("标准异常: {}", e.what());
    }
}

void EyeInference::init_kalman_filter() {
    int pos_size = EYE_OUTPUT_SIZE; // 使用眼睛输出大小
    int state_size = pos_size * 2;  // 状态向量扩展为 [位置, 速度]

    // 初始化协方差矩阵 P，状态维度为 state_size
    cv::Mat P = cv::Mat::eye(state_size, state_size, CV_64F) * 1.0;

    // 过程噪声 Q：这里采用常数速度模型的过程噪声，使用 dt 的多项式因子
    auto update_Q = [state_size, pos_size, this]() {
        cv::Mat Q = cv::Mat::zeros(state_size, state_size, CV_64F);
        double dt2 = dt * dt;
        double dt3 = dt2 * dt;
        double dt4 = dt3 * dt;
        cv::Mat I_pos = cv::Mat::eye(pos_size, pos_size, CV_64F);

        // 位置部分：dt^4/4 * q_factor^2
        Q(cv::Rect(0, 0, pos_size, pos_size)) = I_pos * (dt4 / 4.0 * q_factor * q_factor);

        // 位置-速度 交叉项：dt^3/2 * q_factor^2
        Q(cv::Rect(pos_size, 0, pos_size, pos_size)) = I_pos * (dt3 / 2.0 * q_factor * q_factor);
        Q(cv::Rect(0, pos_size, pos_size, pos_size)) = I_pos * (dt3 / 2.0 * q_factor * q_factor);

        // 速度部分：dt^2 * q_factor^2
        Q(cv::Rect(pos_size, pos_size, pos_size, pos_size)) = I_pos * (dt2 * q_factor * q_factor);

        return Q;
    };

    // 测量噪声 R：假设测量仅包含位置部分，尺寸为 pos_size x pos_size
    auto update_R = [pos_size, this](const cv::Mat& measurement) {
        return cv::Mat::eye(pos_size, pos_size, CV_64F) * r_factor;
    };

    // 定义状态转移矩阵 F = [ I, dt*I; 0, I ]
    auto TransMat = [state_size, pos_size, this](const cv::Mat& state) {
        cv::Mat F = cv::Mat::eye(state_size, state_size, CV_64F);
        cv::Mat I_pos = cv::Mat::eye(pos_size, pos_size, CV_64F);
        // 将上半部分右侧的子矩阵设为 dt * I，即位置更新依赖于速度
        F(cv::Rect(pos_size, 0, pos_size, pos_size)) = I_pos * dt;

        return F;
    };

    // 定义测量矩阵函数 H = [ I, 0 ]，只观测位置部分
    auto MeasureMat = [pos_size, state_size](const cv::Mat& state) {
        cv::Mat H = cv::Mat::zeros(pos_size, state_size, CV_64F);
        cv::Mat I_pos = cv::Mat::eye(pos_size, pos_size, CV_64F);
        I_pos.copyTo(H(cv::Rect(0, 0, pos_size, pos_size)));
        return H;
    };

    kalman_filter_ = KalmanFilter(TransMat, MeasureMat, update_Q, update_R, P.clone());
}

void EyeInference::preprocess(const cv::Mat& input) {
    // 转换为灰度图（如果需要）
    if (input.channels() == 3 && input_c_ == 1) {
        cv::cvtColor(input, gray_image_, cv::COLOR_BGR2GRAY);
    } else if (input.channels() == 1 && input_c_ == 3) {
        cv::cvtColor(input, gray_image_, cv::COLOR_GRAY2BGR);
    } else {
        gray_image_ = input;
    }
    // 调整大小
    cv::resize(gray_image_, processed_image_, cv::Size(input_w_, input_h_), cv::INTER_NEAREST);
    // 归一化处理 - 避免不必要的数据复制
    processed_image_.convertTo(processed_image_, CV_32F, 1.0 / 255.0);
    // 直接拷贝到输入数据缓冲区
    if (processed_image_.isContinuous()) {
        std::memcpy(input_data_.data(), processed_image_.ptr<float>(),
                   processed_image_.total() * sizeof(float));
    } else {
        // 如果数据不连续，则逐行拷贝
        size_t row_bytes = processed_image_.cols * processed_image_.elemSize();
        for (int i = 0; i < processed_image_.rows; ++i) {
            std::memcpy(input_data_.data() + i * processed_image_.cols,
                       processed_image_.ptr(i), row_bytes);
        }
    }

}

void EyeInference::run_model() {
    if (!session_ || input_name_ptrs_.empty() || output_name_ptrs_.empty()) {
        return;
    }

    try {
        // 创建输入张量 - 使用预分配的内存
        input_tensor_ = Ort::Value::CreateTensor<float>(
            memory_info_,
            input_data_.data(),
            input_data_.size(),
            input_shapes_[0].data(),
            input_shapes_[0].size()
        );

        // 绑定输入
        io_binding_->BindInput(input_name_ptrs_[0], input_tensor_);
        // 绑定输出
        for (size_t i = 0; i < output_name_ptrs_.size(); i++) {
            io_binding_->BindOutput(output_name_ptrs_[i], memory_info_);
        }
        // 运行推理
        session_->Run(Ort::RunOptions{nullptr}, *io_binding_);
        // 获取输出
        output_tensors_ = io_binding_->GetOutputValues();
    } catch (const std::exception& e) {
        LOG_ERROR("推理错误: {}", e.what());
    }
}

void EyeInference::process_results() {
    if (output_tensors_.empty()) {
        return;
    }

    // 处理第一个输出
    try {
        Ort::Value& output_tensor = output_tensors_.front();
        // 获取输出形状
        auto shape_info = output_tensor.GetTensorTypeAndShapeInfo();
        auto output_shape = shape_info.GetShape();

        // 计算输出大小
        auto output_size = 1;
        for (auto dim : output_shape) {
            output_size *= dim;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("处理结果出错: {}", e.what());
    }
}

void EyeInference::initBlendShapeIndexMap() {

}


