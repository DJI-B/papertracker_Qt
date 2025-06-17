#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
纯Python标准库OSC眼部追踪数据验证脚本
不依赖任何第三方库，使用socket直接解析OSC数据
"""

import socket
import struct
import threading
import time
import argparse
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field
from datetime import datetime
import statistics

@dataclass
class EyeDataSample:
    """单个眼部数据样本"""
    timestamp: float
    left_x: Optional[float] = None
    right_x: Optional[float] = None
    left_y: Optional[float] = None
    right_y: Optional[float] = None
    left_lid: Optional[float] = None
    right_lid: Optional[float] = None
    pupil_dilation: Optional[float] = None

@dataclass
class ValidationResults:
    """验证结果统计"""
    total_samples: int = 0
    matching_samples: int = 0
    non_matching_samples: int = 0
    max_difference: float = 0.0
    differences: List[float] = field(default_factory=list)
    
    @property
    def match_percentage(self) -> float:
        if self.total_samples == 0:
            return 0.0
        return (self.matching_samples / self.total_samples) * 100

class SimpleOSCParser:
    """简单的OSC消息解析器"""
    
    @staticmethod
    def parse_osc_string(data: bytes, offset: int) -> Tuple[str, int]:
        """解析OSC字符串（以null结尾，4字节对齐）"""
        end = data.find(b'\0', offset)
        if end == -1:
            return "", len(data)
        
        string = data[offset:end].decode('utf-8', errors='ignore')
        # OSC字符串必须4字节对齐
        padded_length = ((end - offset + 1) + 3) // 4 * 4
        return string, offset + padded_length
    
    @staticmethod
    def parse_osc_types(data: bytes, offset: int) -> Tuple[str, int]:
        """解析OSC类型标签"""
        return SimpleOSCParser.parse_osc_string(data, offset)
    
    @staticmethod
    def parse_osc_float(data: bytes, offset: int) -> Tuple[float, int]:
        """解析OSC浮点数（大端序）"""
        if offset + 4 > len(data):
            return 0.0, offset + 4
        value = struct.unpack('>f', data[offset:offset+4])[0]
        return value, offset + 4
    
    @classmethod
    def parse_osc_message(cls, data: bytes) -> Optional[Tuple[str, List[float]]]:
        """解析完整的OSC消息"""
        try:
            if len(data) < 8:  # 最小OSC消息长度
                return None
            
            # 解析地址模式
            address, offset = cls.parse_osc_string(data, 0)
            if not address.startswith('/'):
                return None
            
            # 解析类型标签
            type_tags, offset = cls.parse_osc_types(data, offset)
            if not type_tags.startswith(','):
                return None
            
            # 解析参数
            args = []
            for tag in type_tags[1:]:  # 跳过逗号
                if tag == 'f':  # 浮点数
                    value, offset = cls.parse_osc_float(data, offset)
                    args.append(value)
                elif tag == 'i':  # 整数（转为浮点数）
                    if offset + 4 <= len(data):
                        value = struct.unpack('>i', data[offset:offset+4])[0]
                        args.append(float(value))
                        offset += 4
                else:
                    # 跳过不支持的类型
                    offset += 4
            
            return address, args
            
        except Exception as e:
            print(f"⚠️  OSC解析错误: {e}")
            return None

class OSCEyeDataMonitor:
    """OSC眼部数据监控器（使用标准库socket）"""
    
    def __init__(self, ip: str = "127.0.0.1", port: int = 9001, tolerance: float = 0.001):
        self.ip = ip
        self.port = port
        self.tolerance = tolerance
        self.running = False
        
        # 数据存储
        self.current_sample = EyeDataSample(timestamp=time.time())
        self.samples: List[EyeDataSample] = []
        self.data_lock = threading.Lock()
        
        # 验证结果
        self.results = ValidationResults()
        
        # socket
        self.socket = None
        
        # OSC路径映射
        self.osc_handlers = {
            "/avatar/parameters/v2/EyeLeftX": self.handle_left_x,
            "/avatar/parameters/v2/EyeRightX": self.handle_right_x,
            "/avatar/parameters/v2/EyeLeftY": self.handle_left_y,
            "/avatar/parameters/v2/EyeRightY": self.handle_right_y,
            "/avatar/parameters/v2/EyeLidLeft": self.handle_left_lid,
            "/avatar/parameters/v2/EyeLidRight": self.handle_right_lid,
            "/avatar/parameters/v2/PupilDilation": self.handle_pupil_dilation,
        }
        
    def handle_left_x(self, value: float):
        """处理左眼X轴数据"""
        with self.data_lock:
            self.current_sample.left_x = value
            self.current_sample.timestamp = time.time()
            self.check_and_validate_sample()

    def handle_right_x(self, value: float):
        """处理右眼X轴数据"""
        with self.data_lock:
            self.current_sample.right_x = value
            self.current_sample.timestamp = time.time()
            self.check_and_validate_sample()

    def handle_left_y(self, value: float):
        """处理左眼Y轴数据"""
        with self.data_lock:
            self.current_sample.left_y = value

    def handle_right_y(self, value: float):
        """处理右眼Y轴数据"""
        with self.data_lock:
            self.current_sample.right_y = value

    def handle_left_lid(self, value: float):
        """处理左眼睑数据"""
        with self.data_lock:
            self.current_sample.left_lid = value

    def handle_right_lid(self, value: float):
        """处理右眼睑数据"""
        with self.data_lock:
            self.current_sample.right_lid = value

    def handle_pupil_dilation(self, value: float):
        """处理瞳孔扩张数据"""
        with self.data_lock:
            self.current_sample.pupil_dilation = value

    def check_and_validate_sample(self):
        """检查当前样本是否完整并进行验证"""
        if (self.current_sample.left_x is not None and 
            self.current_sample.right_x is not None):
            
            # 创建样本副本并添加到列表
            sample_copy = EyeDataSample(
                timestamp=self.current_sample.timestamp,
                left_x=self.current_sample.left_x,
                right_x=self.current_sample.right_x,
                left_y=self.current_sample.left_y,
                right_y=self.current_sample.right_y,
                left_lid=self.current_sample.left_lid,
                right_lid=self.current_sample.right_lid,
                pupil_dilation=self.current_sample.pupil_dilation
            )
            
            self.samples.append(sample_copy)
            
            # 验证左右眼X值
            self.validate_x_values(sample_copy)
            
            # 重置当前样本的X值以准备下一次采样
            self.current_sample.left_x = None
            self.current_sample.right_x = None

    def validate_x_values(self, sample: EyeDataSample):
        """验证左右眼X值是否相同"""
        self.results.total_samples += 1
        
        difference = abs(sample.left_x - sample.right_x)
        self.results.differences.append(difference)
        
        if difference <= self.tolerance:
            self.results.matching_samples += 1
        else:
            self.results.non_matching_samples += 1
            
        self.results.max_difference = max(self.results.max_difference, difference)
        
        # 实时输出不匹配的样本
        if difference > self.tolerance:
            timestamp_str = datetime.fromtimestamp(sample.timestamp).strftime("%H:%M:%S.%f")[:-3]
            print(f"⚠️  [{timestamp_str}] 左右眼X值不匹配! "
                  f"左眼: {sample.left_x:.6f}, 右眼: {sample.right_x:.6f}, "
                  f"差值: {difference:.6f}")

    def process_osc_packet(self, data: bytes):
        """处理OSC数据包"""
        result = SimpleOSCParser.parse_osc_message(data)
        if result is None:
            return
            
        address, args = result
        if not args:  # 没有参数
            return
            
        # 查找对应的处理器
        handler = self.osc_handlers.get(address)
        if handler:
            try:
                handler(args[0])  # 使用第一个参数
            except (IndexError, TypeError) as e:
                print(f"⚠️  处理OSC消息错误 {address}: {e}")

    def udp_listener(self):
        """UDP监听线程"""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(1024)
                self.process_osc_packet(data)
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:  # 只在运行时报错
                    print(f"⚠️  UDP接收错误: {e}")
                break

    def start_monitoring(self):
        """开始监控OSC数据"""
        print(f"🎯 开始监听OSC数据 - {self.ip}:{self.port}")
        print(f"📊 浮点数比较容差: {self.tolerance}")
        print("📡 使用纯Python标准库socket实现")
        print("=" * 60)
        
        try:
            # 创建UDP socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket.settimeout(1.0)  # 1秒超时
            self.socket.bind((self.ip, self.port))
            
            self.running = True
            
            # 启动UDP监听线程
            self.udp_thread = threading.Thread(target=self.udp_listener)
            self.udp_thread.daemon = True
            self.udp_thread.start()
            
            # 启动状态报告线程
            self.status_thread = threading.Thread(target=self.status_reporter)
            self.status_thread.daemon = True
            self.status_thread.start()
            
            print(f"✅ UDP服务器已启动，监听端口 {self.port}")
            
        except Exception as e:
            print(f"❌ 启动UDP服务器失败: {e}")
            print(f"💡 请确认端口 {self.port} 未被占用")
            self.running = False

    def status_reporter(self):
        """定期报告状态"""
        last_sample_count = 0
        no_data_count = 0
        
        while self.running:
            time.sleep(5)  # 每5秒报告一次
            with self.data_lock:
                current_count = self.results.total_samples
                if current_count > 0:
                    if current_count > last_sample_count:
                        print(f"📈 已采集 {current_count} 个样本 | "
                              f"匹配率: {self.results.match_percentage:.1f}% | "
                              f"最大差值: {self.results.max_difference:.6f}")
                        last_sample_count = current_count
                        no_data_count = 0
                    else:
                        no_data_count += 1
                        if no_data_count >= 3:  # 15秒没有新数据
                            print("⏳ 等待OSC数据中... 请确认眼部追踪软件正在发送数据")
                else:
                    print("🔄 等待首个OSC数据包...")

    def stop_monitoring(self):
        """停止监控"""
        self.running = False
        if self.socket:
            self.socket.close()

    def print_final_report(self):
        """打印最终验证报告"""
        print("\n" + "=" * 60)
        print("🔍 最终验证报告")
        print("=" * 60)
        
        if self.results.total_samples == 0:
            print("❌ 未收集到任何有效样本")
            print("💡 可能的原因:")
            print("   - 眼部追踪软件未运行")
            print("   - OSC发送端口配置不匹配")
            print("   - 防火墙阻止了UDP数据")
            return
            
        print(f"📊 总样本数: {self.results.total_samples}")
        print(f"✅ 匹配样本: {self.results.matching_samples}")
        print(f"❌ 不匹配样本: {self.results.non_matching_samples}")
        print(f"📈 匹配率: {self.results.match_percentage:.2f}%")
        print(f"📏 最大差值: {self.results.max_difference:.6f}")
        
        if self.results.differences:
            avg_diff = statistics.mean(self.results.differences)
            median_diff = statistics.median(self.results.differences)
            print(f"📊 平均差值: {avg_diff:.6f}")
            print(f"📊 中位数差值: {median_diff:.6f}")
            
        print("\n🔍 结论分析:")
        if self.results.match_percentage >= 99.0:
            print("✅ 验证通过: 左右眼X值基本保持一致")
            print("💡 这证明了targetLeftEyeState.eyeXValue和targetRightEyeState.eyeXValue确实相同")
        elif self.results.match_percentage >= 95.0:
            print("⚠️  大部分情况下一致，但存在少量差异")
            print("💡 可能在特殊情况下（如眨眼检测）存在短暂不一致")
        else:
            print("❌ 验证失败: 左右眼X值存在显著差异")
            print("💡 这可能表示代码实现与理论分析有差异")
            
        print(f"💡 使用的容差值: {self.tolerance}")

    def export_to_csv(self, filename: str = None):
        """导出数据到CSV文件"""
        if not filename:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"osc_eye_data_{timestamp}.csv"
            
        try:
            import csv
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                fieldnames = ['timestamp', 'left_x', 'right_x', 'difference', 
                             'left_y', 'right_y', 'left_lid', 'right_lid', 'pupil_dilation']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                
                writer.writeheader()
                for sample in self.samples:
                    difference = abs(sample.left_x - sample.right_x) if (sample.left_x and sample.right_x) else None
                    writer.writerow({
                        'timestamp': sample.timestamp,
                        'left_x': sample.left_x,
                        'right_x': sample.right_x,
                        'difference': difference,
                        'left_y': sample.left_y,
                        'right_y': sample.right_y,
                        'left_lid': sample.left_lid,
                        'right_lid': sample.right_lid,
                        'pupil_dilation': sample.pupil_dilation
                    })
                    
            print(f"📁 数据已导出到: {filename}")
            
        except Exception as e:
            print(f"❌ 导出CSV失败: {e}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="OSC眼部追踪数据验证工具（纯标准库实现）")
    parser.add_argument("--ip", default="127.0.0.1", help="OSC服务器IP地址 (默认: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8889, help="OSC服务器端口 (默认: 9001)")
    parser.add_argument("--tolerance", type=float, default=0.001, 
                       help="浮点数比较容差 (默认: 0.001)")
    parser.add_argument("--duration", type=int, default=60, 
                       help="监控持续时间(秒) (默认: 60秒, 0=无限制)")
    parser.add_argument("--export", action="store_true", help="导出数据到CSV文件")
    
    args = parser.parse_args()
    
    print("🔍 OSC眼部追踪数据验证工具")
    print("用于验证targetLeftEyeState.eyeXValue和targetRightEyeState.eyeXValue是否相同")
    print("📡 使用Python标准库socket实现，无需第三方依赖")
    print()
    
    monitor = OSCEyeDataMonitor(args.ip, args.port, args.tolerance)
    
    try:
        monitor.start_monitoring()
        
        if not monitor.running:
            return
        
        if args.duration > 0:
            print(f"⏱️  将监控 {args.duration} 秒...")
            time.sleep(args.duration)
        else:
            print("⏱️  无限制监控模式，按Ctrl+C停止...")
            while True:
                time.sleep(1)
                
    except KeyboardInterrupt:
        print("\n🛑 用户中断监控")
    except Exception as e:
        print(f"\n❌ 监控过程中发生错误: {e}")
    finally:
        monitor.stop_monitoring()
        monitor.print_final_report()
        
        if args.export:
            monitor.export_to_csv()

if __name__ == "__main__":
    main()