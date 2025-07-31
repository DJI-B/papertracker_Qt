#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
面部轮廓提取脚本
基于颜色差异的边缘检测算法
适用于后期移植到C++/OpenCV
"""

import cv2
import numpy as np
import os

class FaceContourExtractor:
    def __init__(self):
        # 扩展肤色范围参数（HSV）- 适应特殊光照条件
        self.lower_skin = np.array([0, 30, 50])
        self.upper_skin = np.array([30, 255, 255])
        
        # 备用肤色范围（紫红色调）
        self.lower_skin_alt = np.array([140, 30, 50])
        self.upper_skin_alt = np.array([180, 255, 255])
        
        # 形态学操作核大小
        self.morph_kernel_size = (7, 7)
        
        # Canny边缘检测参数
        self.canny_low = 50
        self.canny_high = 150
        
        # 高斯模糊核大小
        self.blur_kernel = (3, 3)
        
        # 自适应阈值参数
        self.use_adaptive = True

    def extract_face_contour(self, image_path, show_steps=True):
        """
        提取面部轮廓主函数
        
        Args:
            image_path: 输入图像路径
            show_steps: 是否显示中间处理步骤
        
        Returns:
            result: 最终轮廓图像
            steps: 中间处理步骤图像字典
        """
        if not os.path.exists(image_path):
            print(f"错误：文件 {image_path} 不存在")
            return None, None
        
        # 步骤1：读取图像
        img = cv2.imread(image_path)
        if img is None:
            print(f"错误：无法读取图像 {image_path}")
            return None, None
        
        print(f"输入图像尺寸: {img.shape}")
        
        # 步骤2：预处理 - 高斯模糊降噪
        img_blur = cv2.GaussianBlur(img, self.blur_kernel, 0)
        
        # 步骤3：颜色空间转换
        hsv = cv2.cvtColor(img_blur, cv2.COLOR_BGR2HSV)
        
        # 步骤4：创建肤色掩码 - 多范围检测
        mask1 = cv2.inRange(hsv, self.lower_skin, self.upper_skin)
        mask2 = cv2.inRange(hsv, self.lower_skin_alt, self.upper_skin_alt)
        mask = cv2.bitwise_or(mask1, mask2)
        
        # 如果掩码太少，尝试基于亮度的方法
        if cv2.countNonZero(mask) < img.shape[0] * img.shape[1] * 0.05:
            print("肤色检测效果不佳，尝试基于亮度的检测方法...")
            gray = cv2.cvtColor(img_blur, cv2.COLOR_BGR2GRAY)
            
            # 自适应阈值
            if self.use_adaptive:
                mask_adaptive = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
                                                    cv2.THRESH_BINARY, 11, 2)
                # 反转掩码（假设面部比背景亮）
                mask_adaptive = cv2.bitwise_not(mask_adaptive)
                mask = cv2.bitwise_or(mask, mask_adaptive)
            
            # 基于亮度范围的检测
            mean_val = np.mean(gray)
            std_val = np.std(gray)
            lower_gray = max(0, mean_val - std_val)
            upper_gray = min(255, mean_val + std_val)
            mask_gray = cv2.inRange(gray, lower_gray, upper_gray)
            mask = cv2.bitwise_or(mask, mask_gray)
        
        # 步骤5：形态学操作优化掩码
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, self.morph_kernel_size)
        
        # 闭运算：填充内部小洞
        mask_closed = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
        
        # 开运算：去除外部噪声
        mask_final = cv2.morphologyEx(mask_closed, cv2.MORPH_OPEN, kernel)
        
        # 步骤6：边缘检测
        edges = cv2.Canny(mask_final, self.canny_low, self.canny_high)
        
        # 步骤7：边缘膨胀使线条更明显
        dilate_kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
        edges_dilated = cv2.dilate(edges, dilate_kernel, iterations=1)
        
        # 步骤8：创建最终结果 - 白色背景，黑色轮廓
        result = np.ones_like(img) * 255
        result[edges_dilated > 0] = [0, 0, 0]
        
        # 保存中间处理步骤
        steps = {
            'original': img,
            'hsv': hsv,
            'mask1': mask1,
            'mask2': mask2,
            'mask_raw': mask,
            'mask_closed': mask_closed,
            'mask_final': mask_final,
            'edges': edges,
            'edges_dilated': edges_dilated,
            'result': result
        }
        
        if show_steps:
            self._display_steps(steps)
        
        return result, steps

    def _display_steps(self, steps):
        """显示处理步骤"""
        print("显示处理步骤...")
        
        # 创建显示窗口
        cv2.namedWindow('Original', cv2.WINDOW_NORMAL)
        cv2.namedWindow('HSV', cv2.WINDOW_NORMAL)
        cv2.namedWindow('Mask1', cv2.WINDOW_NORMAL)
        cv2.namedWindow('Mask2', cv2.WINDOW_NORMAL)
        cv2.namedWindow('Mask Raw', cv2.WINDOW_NORMAL)
        cv2.namedWindow('Mask Final', cv2.WINDOW_NORMAL)
        cv2.namedWindow('Edges', cv2.WINDOW_NORMAL)
        cv2.namedWindow('Result', cv2.WINDOW_NORMAL)
        
        # 显示图像
        cv2.imshow('Original', steps['original'])
        cv2.imshow('HSV', steps['hsv'])
        cv2.imshow('Mask1', steps['mask1'])
        cv2.imshow('Mask2', steps['mask2'])
        cv2.imshow('Mask Raw', steps['mask_raw'])
        cv2.imshow('Mask Final', steps['mask_final'])
        cv2.imshow('Edges', steps['edges_dilated'])
        cv2.imshow('Result', steps['result'])
        
        # 输出掩码统计信息
        total_pixels = steps['original'].shape[0] * steps['original'].shape[1]
        mask_pixels = cv2.countNonZero(steps['mask_final'])
        print(f"掩码覆盖率: {mask_pixels/total_pixels*100:.2f}%")
        
        print("按任意键继续，按'q'退出...")
        key = cv2.waitKey(0) & 0xFF
        cv2.destroyAllWindows()
        
        return key

    def adjust_skin_range(self, h_min=0, h_max=20, s_min=20, s_max=255, v_min=70, v_max=255):
        """调整肤色范围参数"""
        self.lower_skin = np.array([h_min, s_min, v_min])
        self.upper_skin = np.array([h_max, s_max, v_max])
        print(f"肤色范围已更新: H({h_min}-{h_max}), S({s_min}-{s_max}), V({v_min}-{v_max})")

    def adjust_canny_params(self, low_threshold=100, high_threshold=200):
        """调整Canny边缘检测参数"""
        self.canny_low = low_threshold
        self.canny_high = high_threshold
        print(f"Canny参数已更新: 低阈值={low_threshold}, 高阈值={high_threshold}")

    def save_result(self, result, output_path):
        """保存结果图像"""
        if result is not None:
            cv2.imwrite(output_path, result)
            print(f"结果已保存至: {output_path}")
        else:
            print("错误：结果图像为空，无法保存")

def main():
    """主函数 - 使用示例"""
    # 创建提取器实例
    extractor = FaceContourExtractor()
    
    # 输入图像路径（请替换为您的图像路径）
    input_image = "input_face.jpg"  # 修改为您的图像文件名
    
    # 检查文件是否存在
    if not os.path.exists(input_image):
        print("使用示例:")
        print("1. 将您的图像文件命名为 'input_face.jpg' 并放在脚本同一目录")
        print("2. 或者修改 input_image 变量为您的图像路径")
        print("3. 运行脚本")
        return
    
    print("开始面部轮廓提取...")
    
    # 执行轮廓提取
    result, steps = extractor.extract_face_contour(input_image, show_steps=True)
    
    if result is not None:
        # 保存结果
        output_path = "face_contour_result.jpg"
        extractor.save_result(result, output_path)
        
        print("处理完成！")
        print(f"输入: {input_image}")
        print(f"输出: {output_path}")
        
        # 参数调整示例
        print("\n参数调整示例:")
        print("如果效果不理想，可以尝试以下调整:")
        print("1. 调整肤色范围: extractor.adjust_skin_range(h_min=0, h_max=30)")
        print("2. 调整边缘检测: extractor.adjust_canny_params(low_threshold=50, high_threshold=150)")
        print("3. 重新处理图像")
    else:
        print("处理失败，请检查输入图像")

if __name__ == "__main__":
    main()

# C++移植参考注释
"""
C++移植要点:

1. 头文件包含:
   #include <opencv2/opencv.hpp>
   #include <opencv2/imgproc.hpp>

2. 主要函数对应:
   - cv::imread() 替代 cv2.imread()
   - cv::cvtColor() 替代 cv2.cvtColor()
   - cv::inRange() 替代 cv2.inRange()
   - cv::morphologyEx() 替代 cv2.morphologyEx()
   - cv::Canny() 替代 cv2.Canny()
   - cv::dilate() 替代 cv2.dilate()

3. 数据类型:
   - cv::Mat 替代 numpy.ndarray
   - cv::Scalar 替代 numpy.array (颜色范围)

4. 内存管理:
   - Mat对象自动管理内存
   - 注意拷贝vs引用语义

5. 参数传递:
   - 使用const cv::Mat& 作为输入参数
   - cv::Mat& 作为输出参数

示例C++函数签名:
bool extractFaceContour(const cv::Mat& input, cv::Mat& result, 
                       const cv::Scalar& lowerSkin, const cv::Scalar& upperSkin);
"""