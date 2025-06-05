//
// Created by JellyfishKnight on 25-4-18.
//

#ifndef EYE_INFERENCE_HPP
#define EYE_INFERENCE_HPP
#include "base_inference.hpp"
#include <vector>

#define EYE_OUTPUT_SIZE 14

class EyeInference : public BaseInference
{
public:
    EyeInference();

    virtual ~EyeInference() = default;

    void inference(cv::Mat image) override;

    std::vector<float> get_output() override;

    void load_model(const std::string &model_path) override;

protected:
    void init_kalman_filter() override;

    void preprocess(const cv::Mat& input) override;

    void run_model() override;

    void process_results() override;

    void initBlendShapeIndexMap() override;
};


#endif //EYE_INFERENCE_HPP
