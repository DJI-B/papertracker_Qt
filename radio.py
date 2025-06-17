"""
VRChat形态键演示程序（极简UI版）
只保留最基本的UI组件，避免闪退问题

依赖：pip install python-osc PyQt5
"""

import sys
import time
import threading
from typing import List

# Qt导入
try:
    from PyQt5.QtWidgets import (
        QApplication, QMainWindow, QVBoxLayout, QHBoxLayout,
        QWidget, QPushButton, QLabel, QLineEdit, QTextEdit,
        QCheckBox, QScrollArea
    )
    from PyQt5.QtCore import Qt, QTimer, pyqtSignal
    print("使用PyQt5")
except ImportError:
    try:
        from PyQt6.QtWidgets import (
            QApplication, QMainWindow, QVBoxLayout, QHBoxLayout,
            QWidget, QPushButton, QLabel, QLineEdit, QTextEdit,
            QCheckBox, QScrollArea
        )
        from PyQt6.QtCore import Qt, QTimer, pyqtSignal
        print("使用PyQt6")
    except ImportError:
        print("请安装PyQt: pip install PyQt5")
        sys.exit(1)

# OSC导入
try:
    from pythonosc import udp_client
except ImportError:
    print("请安装python-osc库: pip install python-osc")
    sys.exit(1)

class MinimalDemo(QMainWindow):
    # 定义信号用于线程安全的UI更新
    log_signal = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.init_data()
        self.init_ui()
        self.demo_thread = None
        self.is_demo_running = False

        # 连接信号
        self.log_signal.connect(self.add_log_message)

    def init_data(self):
        """初始化数据"""
        # 完整的45个形态键
        self.all_blendshapes = [
            "cheekPuffLeft", "cheekPuffRight", "cheekSuckLeft", "cheekSuckRight",
            "jawOpen", "jawForward", "jawLeft", "jawRight",
            "noseSneerLeft", "noseSneerRight", "mouthFunnel", "mouthPucker",
            "mouthLeft", "mouthRight", "mouthRollUpper", "mouthRollLower",
            "mouthShrugUpper", "mouthShrugLower", "mouthClose",
            "mouthSmileLeft", "mouthSmileRight", "mouthFrownLeft", "mouthFrownRight",
            "mouthDimpleLeft", "mouthDimpleRight", "mouthUpperUpLeft", "mouthUpperUpRight",
            "mouthLowerDownLeft", "mouthLowerDownRight", "mouthPressLeft", "mouthPressRight",
            "mouthStretchLeft", "mouthStretchRight", "tongueOut", "tongueUp", "tongueDown",
            "tongueLeft", "tongueRight", "tongueRoll", "tongueBendDown",
            "tongueCurlUp", "tongueSquish", "tongueFlat", "tongueTwistLeft", "tongueTwistRight"
        ]

        # 形态键中文名称映射
        self.blendshape_chinese_names = {
            "cheekPuffLeft": "左脸颊鼓起",
            "cheekPuffRight": "右脸颊鼓起",
            "cheekSuckLeft": "左脸颊吸入",
            "cheekSuckRight": "右脸颊吸入",
            "jawOpen": "张嘴",
            "jawForward": "下颌前伸",
            "jawLeft": "下颌左移",
            "jawRight": "下颌右移",
            "noseSneerLeft": "左鼻翼收缩",
            "noseSneerRight": "右鼻翼收缩",
            "mouthFunnel": "嘴部漏斗状",
            "mouthPucker": "撅嘴",
            "mouthLeft": "嘴角左移",
            "mouthRight": "嘴角右移",
            "mouthRollUpper": "上唇内卷",
            "mouthRollLower": "下唇内卷",
            "mouthShrugUpper": "上唇耸起",
            "mouthShrugLower": "下唇耸起",
            "mouthClose": "闭嘴",
            "mouthSmileLeft": "左嘴角微笑",
            "mouthSmileRight": "右嘴角微笑",
            "mouthFrownLeft": "左嘴角皱眉",
            "mouthFrownRight": "右嘴角皱眉",
            "mouthDimpleLeft": "左酒窝",
            "mouthDimpleRight": "右酒窝",
            "mouthUpperUpLeft": "左上唇上拉",
            "mouthUpperUpRight": "右上唇上拉",
            "mouthLowerDownLeft": "左下唇下拉",
            "mouthLowerDownRight": "右下唇下拉",
            "mouthPressLeft": "左嘴角紧压",
            "mouthPressRight": "右嘴角紧压",
            "mouthStretchLeft": "左嘴角拉伸",
            "mouthStretchRight": "右嘴角拉伸",
            "tongueOut": "舌头外伸",
            "tongueUp": "舌头向上",
            "tongueDown": "舌头向下",
            "tongueLeft": "舌头向左",
            "tongueRight": "舌头向右",
            "tongueRoll": "舌头卷曲",
            "tongueBendDown": "舌头向下弯曲",
            "tongueCurlUp": "舌头向上卷曲",
            "tongueSquish": "舌头挤压",
            "tongueFlat": "舌头压平",
            "tongueTwistLeft": "舌头左扭",
            "tongueTwistRight": "舌头右扭"
        }

        # OSC地址
        self.blend_shapes_addresses = [f"/{bs}" for bs in self.all_blendshapes]

        # 输出数组
        self.outputs = [0.0] * 45

        # OSC连接
        self.osc_client = None
        self.is_connected = False

        # 形态键选择状态
        self.blendshape_checkboxes = {}

    def init_ui(self):
        """初始化极简UI"""
        self.setWindowTitle("VRChat形态键演示（极简版）")
        self.setGeometry(200, 200, 600, 400)

        # 中央窗口
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # OSC连接区域
        osc_layout = QHBoxLayout()
        osc_layout.addWidget(QLabel("OSC IP:"))

        self.ip_input = QLineEdit("127.0.0.1")
        self.ip_input.setMaximumWidth(120)
        osc_layout.addWidget(self.ip_input)

        osc_layout.addWidget(QLabel("端口:"))

        self.port_input = QLineEdit("8888")
        self.port_input.setMaximumWidth(80)
        osc_layout.addWidget(self.port_input)

        self.connect_btn = QPushButton("连接")
        self.connect_btn.clicked.connect(self.toggle_connection)
        self.connect_btn.setMaximumWidth(60)
        osc_layout.addWidget(self.connect_btn)

        osc_layout.addStretch()
        layout.addLayout(osc_layout)

        # 状态显示
        self.status_label = QLabel("状态: 未连接")
        layout.addWidget(self.status_label)

        # 控制按钮区域
        control_layout = QHBoxLayout()

        self.demo_all_btn = QPushButton("演示所有形态键")
        self.demo_all_btn.clicked.connect(self.start_demo_all)
        self.demo_all_btn.setEnabled(False)
        control_layout.addWidget(self.demo_all_btn)

        self.demo_expr_btn = QPushButton("演示表情组合")
        self.demo_expr_btn.clicked.connect(self.start_demo_expressions)
        self.demo_expr_btn.setEnabled(False)
        control_layout.addWidget(self.demo_expr_btn)

        self.demo_selected_btn = QPushButton("演示选中形态键")
        self.demo_selected_btn.clicked.connect(self.start_demo_selected)
        self.demo_selected_btn.setEnabled(False)
        control_layout.addWidget(self.demo_selected_btn)

        self.stop_btn = QPushButton("停止")
        self.stop_btn.clicked.connect(self.stop_demo)
        self.stop_btn.setEnabled(False)
        control_layout.addWidget(self.stop_btn)

        self.reset_btn = QPushButton("重置")
        self.reset_btn.clicked.connect(self.reset_all)
        self.reset_btn.setEnabled(False)
        control_layout.addWidget(self.reset_btn)

        layout.addLayout(control_layout)

        # 形态键选择区域
        self.init_blendshape_selection_ui(layout)

        # 日志区域
        layout.addWidget(QLabel("日志:"))
        self.log_text = QTextEdit()
        self.log_text.setMaximumHeight(150)
        layout.addWidget(self.log_text)

    def init_blendshape_selection_ui(self, layout):
        """初始化形态键选择界面"""
        # 形态键选择标题
        selection_label = QLabel("选择要录制的形态键:")
        selection_label.setStyleSheet("font-weight: bold; font-size: 14px;")
        layout.addWidget(selection_label)

        # 快速选择按钮
        quick_select_layout = QHBoxLayout()

        select_all_btn = QPushButton("全选")
        select_all_btn.clicked.connect(self.select_all_blendshapes)
        select_all_btn.setMaximumWidth(60)
        quick_select_layout.addWidget(select_all_btn)

        select_none_btn = QPushButton("清空")
        select_none_btn.clicked.connect(self.select_none_blendshapes)
        select_none_btn.setMaximumWidth(60)
        quick_select_layout.addWidget(select_none_btn)

        select_mouth_btn = QPushButton("嘴部")
        select_mouth_btn.clicked.connect(lambda: self.select_category("mouth"))
        select_mouth_btn.setMaximumWidth(60)
        quick_select_layout.addWidget(select_mouth_btn)

        select_tongue_btn = QPushButton("舌头")
        select_tongue_btn.clicked.connect(lambda: self.select_category("tongue"))
        select_tongue_btn.setMaximumWidth(60)
        quick_select_layout.addWidget(select_tongue_btn)

        select_face_btn = QPushButton("脸部")
        select_face_btn.clicked.connect(lambda: self.select_category("face"))
        select_face_btn.setMaximumWidth(60)
        quick_select_layout.addWidget(select_face_btn)

        quick_select_layout.addStretch()
        layout.addLayout(quick_select_layout)

        # 创建滚动区域包含形态键列表
        try:
            from PyQt5.QtWidgets import QScrollArea
        except ImportError:
            from PyQt6.QtWidgets import QScrollArea

        scroll_area = QScrollArea()
        scroll_area.setMaximumHeight(200)
        scroll_area.setWidgetResizable(True)

        scroll_widget = QWidget()
        scroll_layout = QVBoxLayout(scroll_widget)

        # 按类别组织形态键
        categories = {
            "脸颊和鼻子": ["cheekPuffLeft", "cheekPuffRight", "cheekSuckLeft", "cheekSuckRight", "noseSneerLeft", "noseSneerRight"],
            "下颌": ["jawOpen", "jawForward", "jawLeft", "jawRight"],
            "嘴部基础": ["mouthFunnel", "mouthPucker", "mouthLeft", "mouthRight", "mouthClose"],
            "嘴唇": ["mouthRollUpper", "mouthRollLower", "mouthShrugUpper", "mouthShrugLower"],
            "表情": ["mouthSmileLeft", "mouthSmileRight", "mouthFrownLeft", "mouthFrownRight", "mouthDimpleLeft", "mouthDimpleRight"],
            "嘴部精细": ["mouthUpperUpLeft", "mouthUpperUpRight", "mouthLowerDownLeft", "mouthLowerDownRight", "mouthPressLeft", "mouthPressRight", "mouthStretchLeft", "mouthStretchRight"],
            "舌头": ["tongueOut", "tongueUp", "tongueDown", "tongueLeft", "tongueRight", "tongueRoll", "tongueBendDown", "tongueCurlUp", "tongueSquish", "tongueFlat", "tongueTwistLeft", "tongueTwistRight"]
        }

        for category_name, blendshapes in categories.items():
            # 类别标题
            category_label = QLabel(f"● {category_name}")
            category_label.setStyleSheet("font-weight: bold; color: #0066cc; margin-top: 8px;")
            scroll_layout.addWidget(category_label)

            # 该类别的形态键
            for blendshape in blendshapes:
                if blendshape in self.all_blendshapes:
                    checkbox_layout = QHBoxLayout()
                    checkbox_layout.setContentsMargins(20, 2, 0, 2)

                    checkbox = QCheckBox()
                    checkbox.setChecked(True)  # 默认全选
                    checkbox_layout.addWidget(checkbox)

                    # 中文名 + 英文名
                    chinese_name = self.blendshape_chinese_names.get(blendshape, blendshape)
                    label_text = f"{chinese_name} ({blendshape})"
                    label = QLabel(label_text)
                    label.setStyleSheet("font-size: 12px;")
                    checkbox_layout.addWidget(label)

                    checkbox_layout.addStretch()

                    checkbox_widget = QWidget()
                    checkbox_widget.setLayout(checkbox_layout)
                    scroll_layout.addWidget(checkbox_widget)

                    # 保存复选框引用
                    self.blendshape_checkboxes[blendshape] = checkbox

        scroll_layout.addStretch()
        scroll_area.setWidget(scroll_widget)
        layout.addWidget(scroll_area)

    def toggle_connection(self):
        """切换OSC连接状态"""
        if self.is_connected:
            self.disconnect_osc()
        else:
            self.connect_osc()

    def connect_osc(self):
        """连接OSC"""
        try:
            ip = self.ip_input.text().strip()
            port = int(self.port_input.text().strip())

            self.osc_client = udp_client.SimpleUDPClient(ip, port)
            self.is_connected = True

            self.status_label.setText(f"状态: 已连接 {ip}:{port}")
            self.connect_btn.setText("断开")

            # 启用控制按钮
            self.demo_all_btn.setEnabled(True)
            self.demo_expr_btn.setEnabled(True)
            self.demo_selected_btn.setEnabled(True)
            self.reset_btn.setEnabled(True)

            self.log(f"OSC连接成功: {ip}:{port}")

        except Exception as e:
            self.log(f"OSC连接失败: {e}")
            self.status_label.setText("状态: 连接失败")

    def disconnect_osc(self):
        """断开OSC连接"""
        self.osc_client = None
        self.is_connected = False

        self.status_label.setText("状态: 已断开")
        self.connect_btn.setText("连接")

        # 禁用控制按钮
        self.demo_all_btn.setEnabled(False)
        self.demo_expr_btn.setEnabled(False)
        self.demo_selected_btn.setEnabled(False)
        self.reset_btn.setEnabled(False)

        self.log("OSC连接已断开")

    def send_model_output(self):
        """发送OSC数据"""
        if not self.is_connected or not self.osc_client:
            return False

        try:
            for i in range(len(self.outputs)):
                if i < len(self.blend_shapes_addresses):
                    value = max(0.0, min(1.0, self.outputs[i]))
                    address = self.blend_shapes_addresses[i]
                    self.osc_client.send_message(address, value)
            return True
        except Exception as e:
            self.log(f"发送OSC数据失败: {e}")
            return False

    def start_demo_all(self):
        """开始演示所有形态键"""
        if self.is_demo_running:
            return

        self.is_demo_running = True
        self.demo_all_btn.setEnabled(False)
        self.demo_expr_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)

        self.log("开始演示所有形态键（含依赖关系）...")

        self.demo_thread = threading.Thread(target=self.run_demo_all)
        self.demo_thread.daemon = True
        self.demo_thread.start()

    def start_demo_expressions(self):
        """开始演示表情组合"""
        if self.is_demo_running:
            return

        self.is_demo_running = True
        self.demo_all_btn.setEnabled(False)
        self.demo_expr_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)

        self.log("开始演示实用表情组合（含依赖关系）...")

        self.demo_thread = threading.Thread(target=self.run_demo_expressions)
        self.demo_thread.daemon = True
        self.demo_thread.start()

    def start_demo_selected(self):
        """开始演示选中的形态键"""
        if self.is_demo_running:
            return

        # 获取选中的形态键
        selected_blendshapes = []
        for blendshape, checkbox in self.blendshape_checkboxes.items():
            if checkbox.isChecked():
                selected_blendshapes.append(blendshape)

        if not selected_blendshapes:
            self.log("请至少选择一个形态键进行录制")
            return

        self.is_demo_running = True
        self.demo_all_btn.setEnabled(False)
        self.demo_expr_btn.setEnabled(False)
        self.demo_selected_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)

        self.log(f"开始演示选中的{len(selected_blendshapes)}个形态键...")

        self.demo_thread = threading.Thread(target=self.run_demo_selected, args=(selected_blendshapes,))
        self.demo_thread.daemon = True
        self.demo_thread.start()

    def select_all_blendshapes(self):
        """全选形态键"""
        for checkbox in self.blendshape_checkboxes.values():
            checkbox.setChecked(True)
        self.log("已全选所有形态键")

    def select_none_blendshapes(self):
        """清空选择"""
        for checkbox in self.blendshape_checkboxes.values():
            checkbox.setChecked(False)
        self.log("已清空所有选择")

    def select_category(self, category: str):
        """按类别选择形态键"""
        # 先清空所有选择
        for checkbox in self.blendshape_checkboxes.values():
            checkbox.setChecked(False)

        if category == "mouth":
            mouth_blendshapes = [
                "jawOpen", "jawForward", "jawLeft", "jawRight",
                "mouthFunnel", "mouthPucker", "mouthLeft", "mouthRight", "mouthClose",
                "mouthRollUpper", "mouthRollLower", "mouthShrugUpper", "mouthShrugLower",
                "mouthSmileLeft", "mouthSmileRight", "mouthFrownLeft", "mouthFrownRight",
                "mouthDimpleLeft", "mouthDimpleRight", "mouthUpperUpLeft", "mouthUpperUpRight",
                "mouthLowerDownLeft", "mouthLowerDownRight", "mouthPressLeft", "mouthPressRight",
                "mouthStretchLeft", "mouthStretchRight"
            ]
            for bs in mouth_blendshapes:
                if bs in self.blendshape_checkboxes:
                    self.blendshape_checkboxes[bs].setChecked(True)
            self.log("已选择所有嘴部形态键")

        elif category == "tongue":
            tongue_blendshapes = [
                "tongueOut", "tongueUp", "tongueDown", "tongueLeft", "tongueRight",
                "tongueRoll", "tongueBendDown", "tongueCurlUp", "tongueSquish",
                "tongueFlat", "tongueTwistLeft", "tongueTwistRight"
            ]
            for bs in tongue_blendshapes:
                if bs in self.blendshape_checkboxes:
                    self.blendshape_checkboxes[bs].setChecked(True)
            self.log("已选择所有舌头形态键")

        elif category == "face":
            face_blendshapes = [
                "cheekPuffLeft", "cheekPuffRight", "cheekSuckLeft", "cheekSuckRight",
                "noseSneerLeft", "noseSneerRight"
            ]
            for bs in face_blendshapes:
                if bs in self.blendshape_checkboxes:
                    self.blendshape_checkboxes[bs].setChecked(True)
            self.log("已选择所有脸部形态键")

    def run_demo_selected(self, selected_blendshapes: List[str]):
        """运行选中形态键的演示"""
        try:
            current_index = 0
            total = len(selected_blendshapes)

            while self.is_demo_running:
                if current_index < total:
                    blendshape_name = selected_blendshapes[current_index]
                    chinese_name = self.blendshape_chinese_names.get(blendshape_name, blendshape_name)

                    self.log(f"录制: {chinese_name} ({blendshape_name}) [{current_index + 1}/{total}]")

                    # 获取形态键在全局列表中的索引
                    if blendshape_name in self.all_blendshapes:
                        blendshape_index = self.all_blendshapes.index(blendshape_name)
                        self.animate_single_blendshape(blendshape_index, 2.0, 1.0)

                    if self.is_demo_running:
                        time.sleep(0.5)  # 间隔

                    current_index += 1
                else:
                    # 重新开始
                    current_index = 0
                    self.log("一轮录制完成，重新开始...")

        except Exception as e:
            self.log(f"演示错误: {e}")
        finally:
            if self.is_demo_running:
                self.stop_demo()

    def run_demo_all(self):
        """运行全部形态键演示"""
        try:
            current_index = 0
            while self.is_demo_running:
                if current_index < len(self.all_blendshapes):
                    blendshape_name = self.all_blendshapes[current_index]
                    self.log(f"演示: {blendshape_name} ({current_index + 1}/{len(self.all_blendshapes)})")

                    # 执行动画
                    self.animate_single_blendshape(current_index, 2.0, 1.0)

                    if self.is_demo_running:
                        time.sleep(0.5)  # 间隔

                    current_index += 1
                else:
                    # 重新开始
                    current_index = 0
                    self.log("一轮完成，重新开始...")

        except Exception as e:
            self.log(f"演示错误: {e}")
        finally:
            if self.is_demo_running:
                self.stop_demo()

    def run_demo_expressions(self):
        """运行表情组合演示（改进版，考虑实际效果）"""
        expressions = [
            {
                "name": "微笑",
                "indices": [19, 20],
                "values": [0.8, 0.8],
                "description": "左右嘴角上扬"
            },
            {
                "name": "大笑",
                "indices": [4, 19, 20, 0, 1],
                "values": [0.3, 0.9, 0.9, 0.3, 0.3],
                "description": "张嘴微笑并鼓腮"
            },
            {
                "name": "皱眉",
                "indices": [21, 22],
                "values": [0.7, 0.7],
                "description": "左右嘴角下拉"
            },
            {
                "name": "惊讶张嘴",
                "indices": [4, 11],
                "values": [0.6, 0.4],
                "description": "张嘴并漏斗状"
            },
            {
                "name": "撅嘴",
                "indices": [11, 18],
                "values": [0.8, 0.2],
                "description": "撅嘴配合轻微闭嘴"
            },
            {
                "name": "鼓腮",
                "indices": [0, 1],
                "values": [0.8, 0.8],
                "description": "双侧鼓腮"
            },
            {
                "name": "吸腮",
                "indices": [2, 3],
                "values": [0.7, 0.7],
                "description": "双侧吸腮"
            },
            {
                "name": "舌头外伸",
                "indices": [4, 32],
                "values": [0.5, 0.8],
                "description": "张嘴伸舌头"
            },
            {
                "name": "舌头上舔",
                "indices": [4, 32, 33],
                "values": [0.4, 0.6, 0.7],
                "description": "张嘴伸舌头向上"
            },
            {
                "name": "舌头下压",
                "indices": [4, 32, 34],
                "values": [0.4, 0.6, 0.6],
                "description": "张嘴伸舌头向下"
            },
            {
                "name": "舌头左右摆动",
                "indices": [4, 32, 35],
                "values": [0.4, 0.6, 0.6],
                "description": "张嘴伸舌头向左"
            },
            {
                "name": "舌头右摆",
                "indices": [4, 32, 36],
                "values": [0.4, 0.6, 0.6],
                "description": "张嘴伸舌头向右"
            },
            {
                "name": "舌头卷曲",
                "indices": [4, 32, 37],
                "values": [0.4, 0.7, 0.8],
                "description": "张嘴伸舌头卷曲"
            },
            {
                "name": "舌头扭转",
                "indices": [4, 32, 41],
                "values": [0.4, 0.6, 0.7],
                "description": "张嘴伸舌头扭转"
            },
            {
                "name": "下颌左移",
                "indices": [4, 6],
                "values": [0.2, 0.6],
                "description": "轻微张嘴，下颌左移"
            },
            {
                "name": "下颌右移",
                "indices": [4, 7],
                "values": [0.2, 0.6],
                "description": "轻微张嘴，下颌右移"
            },
            {
                "name": "鼻翼收缩",
                "indices": [8, 9],
                "values": [0.6, 0.6],
                "description": "双侧鼻翼收缩"
            }
        ]

        try:
            while self.is_demo_running:
                for expr in expressions:
                    if not self.is_demo_running:
                        break

                    self.log(f"演示表情: {expr['name']} - {expr['description']}")
                    self.animate_expression(expr['indices'], expr['values'], 3.0)

                    if self.is_demo_running:
                        time.sleep(1.0)  # 表情间隔

        except Exception as e:
            self.log(f"演示错误: {e}")
        finally:
            if self.is_demo_running:
                self.stop_demo()

    def animate_single_blendshape(self, index: int, duration: float, max_intensity: float):
        """动画化单个形态键（考虑依赖关系）"""
        blendshape_name = self.all_blendshapes[index]

        # 检查是否需要依赖形态键
        dependencies = self.get_blendshape_dependencies(blendshape_name)

        fps = 60
        frame_time = 1.0 / fps
        steps = int(duration * fps)

        # 先激活依赖的形态键
        for dep_name, dep_value in dependencies.items():
            if dep_name in self.all_blendshapes:
                dep_index = self.all_blendshapes.index(dep_name)
                self.outputs[dep_index] = dep_value

        # 发送依赖形态键
        if dependencies:
            self.send_model_output()
            time.sleep(0.2)  # 短暂等待，让依赖形态键生效

        # 渐入目标形态键
        for i in range(steps // 2):
            if not self.is_demo_running:
                break
            progress = i / (steps // 2 - 1)
            value = progress * max_intensity
            self.outputs[index] = value
            self.send_model_output()
            time.sleep(frame_time)

        # 渐出目标形态键
        for i in range(steps // 2):
            if not self.is_demo_running:
                break
            progress = 1.0 - (i / (steps // 2 - 1))
            value = progress * max_intensity
            self.outputs[index] = value
            self.send_model_output()
            time.sleep(frame_time)

        # 重置目标形态键
        self.outputs[index] = 0.0

        # 渐出依赖形态键
        if dependencies:
            dep_steps = 30  # 0.5秒渐出依赖形态键
            for i in range(dep_steps):
                if not self.is_demo_running:
                    break
                progress = 1.0 - (i / (dep_steps - 1))
                for dep_name, dep_value in dependencies.items():
                    if dep_name in self.all_blendshapes:
                        dep_index = self.all_blendshapes.index(dep_name)
                        self.outputs[dep_index] = progress * dep_value
                self.send_model_output()
                time.sleep(frame_time)

        # 最终重置所有相关形态键
        self.outputs[index] = 0.0
        for dep_name in dependencies.keys():
            if dep_name in self.all_blendshapes:
                dep_index = self.all_blendshapes.index(dep_name)
                self.outputs[dep_index] = 0.0
        self.send_model_output()

    def get_blendshape_dependencies(self, blendshape_name: str) -> dict:
        """获取形态键的依赖关系"""
        dependencies = {}

        # 舌头伸出动作：只需要张嘴
        if blendshape_name == "tongueOut":
            dependencies["jawOpen"] = 0.4  # 中等程度张嘴

        # 其他舌头动作：需要张嘴 + 伸出舌头
        other_tongue_blendshapes = [
            "tongueUp", "tongueDown", "tongueLeft", "tongueRight",
            "tongueRoll", "tongueBendDown", "tongueCurlUp", "tongueSquish",
            "tongueFlat", "tongueTwistLeft", "tongueTwistRight"
        ]

        if blendshape_name in other_tongue_blendshapes:
            dependencies["jawOpen"] = 0.4      # 张嘴
            dependencies["tongueOut"] = 0.6    # 先伸出舌头

        # 嘴部内侧动作需要稍微张嘴或张唇
        mouth_inner_blendshapes = [
            "mouthRollUpper", "mouthRollLower", "mouthShrugUpper", "mouthShrugLower"
        ]

        if blendshape_name in mouth_inner_blendshapes:
            dependencies["jawOpen"] = 0.2  # 轻微张嘴

        # 某些嘴部动作需要基础嘴型
        if blendshape_name == "mouthFunnel":
            dependencies["jawOpen"] = 0.3  # 漏斗状需要张嘴

        if blendshape_name == "mouthPucker":
            dependencies["mouthClose"] = 0.2  # 撅嘴时嘴唇稍微合拢

        # 极端下颌动作可能需要稍微张嘴
        extreme_jaw_blendshapes = ["jawLeft", "jawRight"]
        if blendshape_name in extreme_jaw_blendshapes:
            dependencies["jawOpen"] = 0.15  # 很轻微张嘴，便于观察下颌移动

        # 鼻翼动作配合轻微皱眉更明显
        nose_blendshapes = ["noseSneerLeft", "noseSneerRight"]
        if blendshape_name in nose_blendshapes:
            # 可以选择性添加轻微皱眉，但这里保持简单
            pass

        return dependencies

    def animate_expression(self, indices: List[int], values: List[float], duration: float):
        """动画化表情组合"""
        fps = 60
        frame_time = 1.0 / fps
        steps = int(duration * fps)

        # 渐入
        for i in range(steps // 3):
            if not self.is_demo_running:
                break
            progress = i / (steps // 3 - 1)
            for j, index in enumerate(indices):
                if j < len(values):
                    self.outputs[index] = progress * values[j]
            self.send_model_output()
            time.sleep(frame_time)

        # 保持
        for _ in range(steps // 3):
            if not self.is_demo_running:
                break
            self.send_model_output()
            time.sleep(frame_time)

        # 渐出
        for i in range(steps // 3):
            if not self.is_demo_running:
                break
            progress = 1.0 - (i / (steps // 3 - 1))
            for j, index in enumerate(indices):
                if j < len(values):
                    self.outputs[index] = progress * values[j]
            self.send_model_output()
            time.sleep(frame_time)

        # 重置
        for index in indices:
            self.outputs[index] = 0.0
        self.send_model_output()

    def stop_demo(self):
        """停止演示"""
        self.is_demo_running = False

        # 恢复按钮状态
        if self.is_connected:
            self.demo_all_btn.setEnabled(True)
            self.demo_expr_btn.setEnabled(True)
            self.demo_selected_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)

        # 重置所有形态键
        self.reset_all()
        self.log("演示已停止")

    def reset_all(self):
        """重置所有形态键"""
        for i in range(len(self.outputs)):
            self.outputs[i] = 0.0
        self.send_model_output()
        self.log("所有形态键已重置")

    def log(self, message: str):
        """添加日志（线程安全）"""
        # 使用信号机制确保线程安全
        self.log_signal.emit(message)

    def add_log_message(self, message: str):
        """实际添加日志消息（在主线程中执行）"""
        timestamp = time.strftime("%H:%M:%S")
        log_message = f"[{timestamp}] {message}"

        try:
            self.log_text.append(log_message)

            # 简单的日志清理
            if self.log_text.document().blockCount() > 50:
                cursor = self.log_text.textCursor()
                cursor.movePosition(cursor.Start)
                cursor.select(cursor.BlockUnderCursor)
                cursor.removeSelectedText()

            # 滚动到底部
            scrollbar = self.log_text.verticalScrollBar()
            if scrollbar:
                scrollbar.setValue(scrollbar.maximum())
        except Exception:
            pass  # 忽略日志更新错误

    def closeEvent(self, event):
        """程序关闭事件"""
        self.stop_demo()
        if self.is_connected:
            self.disconnect_osc()
        event.accept()

def main():
    """主函数"""
    app = QApplication(sys.argv)

    try:
        window = MinimalDemo()
        window.show()

        window.log("极简UI版本启动完成")
        window.log("已优化舌头动作依赖关系：舌头运动会先伸出舌头")
        window.log("新增形态键选择功能：可选择特定形态键进行录制")
        window.log("包含完整中英文对照：方便理解每个形态键的含义")
        window.log("请先连接OSC，然后选择演示模式")

        sys.exit(app.exec_() if hasattr(app, 'exec_') else app.exec())

    except Exception as e:
        print(f"程序启动失败: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()