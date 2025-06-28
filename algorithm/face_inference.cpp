//
// Created by JellyfishKnight on 2025/2/26.
//
/*
 * PaperTracker - 面部追踪应用程序
 * Copyright (C) 2025 PAPER TRACKER
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file contains code from projectbabble:
 * Copyright 2023 Sameer Suri
 * Licensed under the Apache License, Version 2.0
 */
#include "face_inference.hpp"
#include <iostream>
#include <fstream>
#include <onnxruntime_cxx_api.h>
#include <onnxruntime_c_api.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QFile>  // 用于读取资源文件
#include <QString>  // 用于字符串转换
#include <QFile>
#include <QStandardPaths>
#ifdef USE_CUDA
#include <cuda_runtime.h>
#endif
FaceInference::FaceInference()
{
    init_kalman_filter();
    initBlendShapeIndexMap();
    input_h_ = 224;
    input_w_ = 224;
    input_c_ = 1;
}
void FaceInference::load_model(const std::string &model_path) {
    try {
        std::string actual_model_path = ":/models/model/face_model.onnx";

        // 创建环境 - 只需要创建一次
        static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ONNXRuntimeDemo");

        // 配置会话选项
        session_options.SetIntraOpNumThreads(1);  // 对于GPU推理，减少CPU线程数
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        session_options.AddConfigEntry("session.intra_op.allow_spinning", "0");
        session_options.EnableCpuMemArena();
        session_options.DisableMemPattern();
        
        // 添加针对CUDA的优化配置
        session_options.AddConfigEntry("session.disable_prepacking", "0");
        session_options.AddConfigEntry("session.enable_memory_pattern", "0");

        // 检查CUDA可用性并启用
        auto providers = Ort::GetAvailableProviders();
        LOG_INFO("Available ONNX Runtime providers:");
        for (const auto& p : providers) {
            LOG_INFO("  - {}", p);
        }
        
        bool cuda_is_available = false;
        for (const auto& p : providers) {
            if (p == "CUDAExecutionProvider") {
                cuda_is_available = true;
                break;
            }
        }

        if (cuda_is_available) {
            LOG_INFO("CUDA is available, attempting to enable CUDA execution provider for face inference.");
            
            try {
                // 使用官方推荐的CUDA初始化方式
                OrtCUDAProviderOptionsV2* cuda_options = nullptr;
                Ort::ThrowOnError(Ort::GetApi().CreateCUDAProviderOptions(&cuda_options));
                
                std::vector<const char*> keys{"device_id", "gpu_mem_limit", "arena_extend_strategy", 
                                             "cudnn_conv_algo_search", "do_copy_in_default_stream", 
                                             "cudnn_conv_use_max_workspace", "cudnn_conv1d_pad_to_nc1d",
                                             "enable_cuda_graph"};
                std::vector<const char*> values{"0", "4294967296", "kNextPowerOfTwo", 
                                               "EXHAUSTIVE", "1", "1", "1", "0"};
                
                Ort::ThrowOnError(Ort::GetApi().UpdateCUDAProviderOptions(cuda_options, keys.data(), values.data(), keys.size()));
                
                // 添加CUDA执行提供者
                Ort::ThrowOnError(Ort::GetApi().SessionOptionsAppendExecutionProvider_CUDA_V2(
                    static_cast<OrtSessionOptions*>(session_options), cuda_options));
                
                // 释放提供者选项
                Ort::GetApi().ReleaseCUDAProviderOptions(cuda_options);
                LOG_INFO("CUDA execution provider successfully configured for face inference.");
            } catch (const Ort::Exception& e) {
                LOG_WARN("Failed to configure CUDA execution provider: {}. Falling back to CPU.", e.what());
                cuda_is_available = false;
            } catch (const std::exception& e) {
                LOG_WARN("Failed to configure CUDA execution provider: {}. Falling back to CPU.", e.what());
                cuda_is_available = false;
            }
        }
        
        if (!cuda_is_available) {
            LOG_INFO("Using CPU execution provider for face inference.");
            // CPU执行提供者会自动添加，无需显式配置
        }

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

            // 使用内存数据创建会话
            try {
                session_ = std::make_shared<Ort::Session>(
                    env,
                    reinterpret_cast<const void*>(modelData.constData()),
                    static_cast<size_t>(modelData.size()),
                    session_options);
            } catch (const Ort::Exception& e) {
                LOG_WARN("Failed to create session with current configuration: {}. Retrying with CPU-only configuration.", e.what());
                
                // 重置会话选项，移除所有CUDA相关配置
                session_options = Ort::SessionOptions{};
                session_options.SetIntraOpNumThreads(2);
                session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
                session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
                session_options.EnableCpuMemArena();
                session_options.DisableMemPattern();
                
                // 重新尝试创建会话（仅使用CPU）
                session_ = std::make_shared<Ort::Session>(
                    env,
                    reinterpret_cast<const void*>(modelData.constData()),
                    static_cast<size_t>(modelData.size()),
                    session_options);
                
                LOG_INFO("Successfully created session with CPU-only configuration.");
            }
        } else {
            // 从文件系统加载（保留原有代码的兼容性）
            //LOG_INFO("从文件系统加载眼睛模型: {}", actual_model_path);

            // if (!file_exists(actual_model_path)) {
            //     LOG_ERROR("错误：模型文件不存在: {}", actual_model_path);
            //     return;
            // }

            // 转换为宽字符路径
            std::wstring wmodel_path = utf8_to_wstring(actual_model_path);

            // 创建会话
            try {
                session_ = std::make_shared<Ort::Session>(env, wmodel_path.c_str(), session_options);
            } catch (const Ort::Exception& e) {
                LOG_WARN("Failed to create session with current configuration: {}. Retrying with CPU-only configuration.", e.what());
                
                // 重置会话选项，移除所有CUDA相关配置
                session_options = Ort::SessionOptions{};
                session_options.SetIntraOpNumThreads(2);
                session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
                session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
                session_options.EnableCpuMemArena();
                session_options.DisableMemPattern();
                
                // 重新尝试创建会话（仅使用CPU）
                session_ = std::make_shared<Ort::Session>(env, wmodel_path.c_str(), session_options);
                
                LOG_INFO("Successfully created session with CPU-only configuration.");
            }
        }

        io_binding_ = std::make_shared<Ort::IoBinding>(*session_);

        // 初始化输入和输出名称
        init_io_names();

        // 提前分配内存
        allocate_buffers();

        LOG_INFO("模型加载完成");
    } catch (const Ort::Exception& e) {
        LOG_ERROR("ONNX Runtime 错误: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("标准异常: {}", e.what());
    }
}
void FaceInference::inference(cv::Mat image) {
    if (!image.empty()) {
        // 预处理图像 - 直接修改预分配的内存
        preprocess(image);
        // 运行模型
        run_model();
        // 处理结果
        process_results();
    }
}

void FaceInference::preprocess(const cv::Mat& input) {
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
    processed_image_.convertTo(processed_image_, CV_32F, 1.0/255.0);
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

void FaceInference::run_model() {
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

void FaceInference::process_results() {
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
        size_t output_size = 1;
        for (auto dim : output_shape) {
            output_size *= dim;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("处理结果出错: {}", e.what());
    }
}

std::vector<float> FaceInference::get_output()
{
    if (output_tensors_.empty()) {
        return std::vector<float>();
    }

    try {
        // 获取第一个输出张量
        const Ort::Value& output_tensor = output_tensors_.front();
        const float* output_data = output_tensor.GetTensorData<float>();

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
        result.resize(90);

        if (use_filter)
        {
#ifdef DEBUG
            float raw = 0;
            if (!result.empty())
            {
                raw = result[4];
            }
#endif
            if (last_use_filter != use_filter)
            {
                last_use_filter = use_filter;
                cv::Mat input = cv::Mat(cv::Size(1, 90), CV_32F, result.data());
                kalman_filter_.set_state(input.clone());
            }
            auto temp = kalman_filter_.predict();
            if (!result.empty())
            {
                auto measure = cv::Mat(cv::Size(1, 45), CV_32F, result.data());
                kalman_filter_.correct(measure.clone());
                result = kalman_filter_.state_post_.clone();
            } else
            {
                result = temp.clone();
            }
#ifdef DEBUG
            if (!result.empty())
            {
                float filtered = result[4];
                plot_curve(raw, filtered);
            }
#endif
        }
        result.resize(45);
        // 输出限幅以及增益调整
        // 应用偏置值 - 在这里添加代码
        for (int i = 0; i < blendShapes.size() && i < result.size(); i++)
        {
            if (blendShapeOffsetMap.contains(blendShapes[i]))
            {
                // 应用偏置值，确保结果在0-1之间
                result[i] = std::max(0.0f, std::min(1.0f, result[i] + blendShapeOffsetMap[blendShapes[i]]));
            }
        }

        AmpMapToOutput(result);


        return result;
    }
    catch (const std::exception& e) {
        LOG_ERROR("获取输出数据错误: {}", e.what());
        return std::vector<float>();
    }
}

void FaceInference::init_kalman_filter()
{
    int pos_size = 45;
    int state_size = pos_size * 2;  // 状态向量扩展为 [位置, 速度]

    // 初始化协方差矩阵 P，状态维度为 90
    cv::Mat P = cv::Mat::eye(state_size, state_size, CV_32F) * 1.0f;

    // 过程噪声 Q：这里采用常数速度模型的过程噪声，使用 dt 的多项式因子
    auto update_Q = [state_size, pos_size, this]() {
        cv::Mat Q = cv::Mat::zeros(state_size, state_size, CV_32F);
        float dt2 = dt * dt;
        float dt3 = dt2 * dt;
        float dt4 = dt3 * dt;
        cv::Mat I_pos = cv::Mat::eye(pos_size, pos_size, CV_32F);

        // 位置部分：dt^4/4 * q_factor^2
        Q(cv::Rect(0, 0, pos_size, pos_size)) = I_pos * (dt4 / 4.0f * q_factor * q_factor);

        // 位置-速度 交叉项：dt^3/2 * q_factor^2
        Q(cv::Rect(pos_size, 0, pos_size, pos_size)) = I_pos * (dt3 / 2.0f * q_factor * q_factor);
        Q(cv::Rect(0, pos_size, pos_size, pos_size)) = I_pos * (dt3 / 2.0f * q_factor * q_factor);

        // 速度部分：dt^2 * q_factor^2
        Q(cv::Rect(pos_size, pos_size, pos_size, pos_size)) = I_pos * (dt2 * q_factor * q_factor);

        return Q;
    };

    // 测量噪声 R：假设测量仅包含位置部分，尺寸为 45x45
    auto update_R = [pos_size, this](const cv::Mat& measurement) {
        return cv::Mat::eye(pos_size, pos_size, CV_32F) * r_factor;
    };

    // 定义状态转移矩阵 F = [ I, dt*I; 0, I ]
    auto TransMat = [state_size, pos_size, this](const cv::Mat& state) {
        cv::Mat F = cv::Mat::eye(state_size, state_size, CV_32F);
        cv::Mat I_pos = cv::Mat::eye(pos_size, pos_size, CV_32F);
        // 将上半部分右侧的子矩阵设为 dt * I，即位置更新依赖于速度
        F(cv::Rect(pos_size, 0, pos_size, pos_size)) = I_pos * dt;

        return F;
    };

    // 定义测量矩阵函数 H = [ I, 0 ]，只观测位置部分
    auto MeasureMat = [pos_size, state_size](const cv::Mat& state) {
        cv::Mat H = cv::Mat::zeros(pos_size, state_size, CV_32F);
        cv::Mat I_pos = cv::Mat::eye(pos_size, pos_size, CV_32F);
        I_pos.copyTo(H(cv::Rect(0, 0, pos_size, pos_size)));
        return H;
    };

    kalman_filter_ = KalmanFilter(TransMat, MeasureMat, update_Q, update_R, P.clone());
}

void FaceInference::initBlendShapeIndexMap()
{
    // 定义所有ARKit模型输出名称及其索引
    blendShapes = {
        "cheekPuffLeft", "cheekPuffRight",
        "cheekSuckLeft", "cheekSuckRight",
        "jawOpen", "jawForward", "jawLeft", "jawRight",
        "noseSneerLeft", "noseSneerRight",
        "mouthFunnel", "mouthPucker",
        "mouthLeft", "mouthRight",
        "mouthRollUpper", "mouthRollLower",
        "mouthShrugUpper", "mouthShrugLower",
        "mouthClose",
        "mouthSmileLeft", "mouthSmileRight",
        "mouthFrownLeft", "mouthFrownRight",
        "mouthDimpleLeft", "mouthDimpleRight",
        "mouthUpperUpLeft", "mouthUpperUpRight",
        "mouthLowerDownLeft", "mouthLowerDownRight",
        "mouthPressLeft", "mouthPressRight",
        "mouthStretchLeft", "mouthStretchRight",
        "tongueOut", "tongueUp", "tongueDown",
        "tongueLeft", "tongueRight",
        "tongueRoll", "tongueBendDown",
        "tongueCurlUp", "tongueSquish",
        "tongueFlat", "tongueTwistLeft",
        "tongueTwistRight"
    };

    // 构建映射
    for (size_t i = 0; i < blendShapes.size(); ++i) {
        blendShapeIndexMap[blendShapes[i]] = i;
        blendShapeAmpMap.insert_or_assign(blendShapes[i], 0);
    }
}

