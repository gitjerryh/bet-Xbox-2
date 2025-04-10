from flask import Flask, Response, request
from picamera2 import Picamera2
import cv2
import time
import threading
import numpy as np
import os
import signal
import sys
import logging
import socket
import subprocess

# 使用当前用户的主目录
home_dir = os.path.expanduser("~")
log_file = os.path.join(home_dir, "camera_server.log")

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler(log_file)
    ]
)
logger = logging.getLogger('camera_server')

app = Flask(__name__)
picam2 = None
camera_lock = threading.Lock()
running = True
last_client_time = time.time()
active_clients = 0
max_clients = 5
clients_lock = threading.Lock()
health_check_interval = 30

# 添加帧超时检测变量
last_frame_time = 0
frame_timeout = 5  # 5秒没有新帧就重启摄像头

# 监控摄像头性能的变量
frame_times = []  # 用于FPS计算的帧时间列表
fps_stats = {"current": 0, "min": 0, "max": 0, "avg": 0}
stats_lock = threading.Lock()
latest_frame = None  # 存储最新的帧
frame_lock = threading.Lock()  # 用于保护latest_frame

# 用于控制图像处理复杂度的标志
reduce_processing = False  # 默认使用完整处理
processing_level = 1  # 默认中等处理级别
max_processing_level = 2  # 最高处理级别
previous_processing_level = 1  # 上一次的处理级别
frame_counter = 0  # 帧计数器，用于控制处理频率

# 内存和资源监控
memory_usage_history = []  # 用于跟踪内存使用趋势
last_memory_reset = time.time()  # 上次内存重置时间
memory_reset_needed = False  # 是否需要重置内存
resource_monitor_interval = 60  # 资源监控间隔(秒)
last_resource_check = time.time()  # 上次资源检查时间
last_fps_check = time.time()  # 上次FPS检查时间
processing_adjustment_interval = 5  # 处理级别调整间隔

# 缓存管理
cached_frame = None
cached_frame_time = 0
cache_validity_period = 0.1  # 帧缓存有效期(秒)

# 添加预处理的查找表，以加速颜色调整
b_lut = np.clip(np.arange(0, 256) * 0.85, 0, 255).astype(np.uint8)  # 降低蓝色通道但更轻微
# 略微提高红色通道增益以改善颜色平衡但不过度
r_lut = np.clip(np.arange(0, 256) * 1.05, 0, 255).astype(np.uint8)  # 轻微增强红色通道
# 调整亮度和对比度以提高细节可见性但保持稳定
alpha_beta_lut = np.clip(np.arange(0, 256) * 1.05 + 5, 0, 255).astype(np.uint8)

# 将已编码的帧缓存
encoded_frames_cache = []
encoded_frames_cache_lock = threading.Lock()
frame_cache_size = 3
frame_cache_index = 0

# 添加时间戳缓存变量
last_timestamp = ""
last_timestamp_update = 0
timestamp_update_interval = 1.0  # 时间戳每秒更新一次，但每帧都显示

# 帧间平滑过渡变量
previous_frame = None
transition_alpha = 0.8  # 增加当前帧权重，减少前一帧权重以提高响应速度
use_frame_blending = True  # 可以动态控制是否使用帧混合

# 创建一个固定的锐化核，避免每次重新计算
sharpening_kernel = np.array([[-0.1, -0.1, -0.1],
                              [-0.1,  1.8, -0.1],
                              [-0.1, -0.1, -0.1]])

# 红外夜视相关变量
night_vision_enabled = True   # 是否启用夜视模式（默认开启）
night_vision_auto = True      # 是否自动切换夜视模式
night_vision_active = False   # 夜视模式是否激活
light_threshold = 50.0        # 光线阈值，低于此值启用夜视
last_light_level = 255.0      # 上次检测到的光线水平
ir_led_available = False      # 是否有可控制的红外LED
ir_led_pin = 17               # 红外LED的GPIO引脚
night_vision_lock = threading.Lock()  # 夜视模式的锁
night_vision_strength = 0.8   # 夜视增强强度
enable_green_tint = True      # 是否启用绿色夜视效果

# 添加运动检测相关的变量
motion_detected = False       # 是否检测到运动
motion_detection_threshold = 25  # 运动检测阈值
last_motion_time = 0          # 上次检测到运动的时间
reduced_processing_until = 0  # 降低处理复杂度直到此时间
motion_detection_interval = 0.5  # 运动检测间隔(秒)
motion_frame_buffer = None    # 用于运动检测的前一帧缓存

# 创建夜视模式查找表
# 提高暗部和中间亮度区域，使暗处细节更加可见
# 使用更温和的曲线，避免过度增强导致噪点
night_mode_lut = np.clip(np.power(np.arange(0, 256) / 255.0, 0.7) * 255 + 15, 0, 255).astype(np.uint8)

# 创建夜视静态缓存对象，避免重复创建
night_vision_buffer = {
    "green_mask": None,
    "reduced": None,
    "last_frame_shape": None,
    "frames_since_reset": 0
}

def signal_handler(sig, frame):
    global running
    logger.info("正在关闭摄像头服务...")
    running = False
    if picam2 is not None:
        try:
            picam2.stop()
        except Exception as e:
            logger.error(f"关闭摄像头时出错: {e}")
    sys.exit(0)

# 注册信号处理器
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

def get_ip_address():
    try:
        # 获取本机IP地址(非回环地址)
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception as e:
        logger.error(f"获取IP地址失败: {e}")
        return "127.0.0.1"

def reset_camera():
    """重置摄像头，在出现问题时调用"""
    global picam2
    logger.warning("正在重置摄像头...")
    
    with camera_lock:
        if picam2 is not None:
            try:
                picam2.stop()
                logger.info("摄像头已停止")
            except Exception as e:
                logger.error(f"停止摄像头时出错: {e}")
            finally:
                picam2 = None
    
    # 等待一段时间再重新初始化
    time.sleep(2)
    return init_camera()

def init_camera():
    global picam2, last_frame_time
    try:
        if picam2 is not None:
            try:
                picam2.stop()
            except:
                pass
            picam2 = None
            
        attempts = 0
        logger.info("初始化摄像头...")
        
        for attempt in range(3):  # 尝试3次
            try:
                picam2 = Picamera2()
                
                # 针对OV5647摄像头特性优化的配置
                config = picam2.create_video_configuration(
                    main={
                        "size": (640, 480),
                        "format": "RGB888"
                    },
                    buffer_count=6,  # 对于4GB内存的树莓派，使用6而不是8更合适
                    controls={
                        "FrameDurationLimits": (33333, 60000),  # 更宽松的帧率限制，介于16-30fps
                        "AwbEnable": True,      # 保持自动白平衡
                        "AwbMode": 1,           # 使用日光模式
                        "Brightness": 0.12,     # 稍微提高默认亮度，对OV5647夜视有帮助
                        "Contrast": 1.1,        # 适度增加对比度
                        "Saturation": 1.0,      # 使用标准饱和度
                        "Sharpness": 1.0,       # 使用标准锐度
                        "NoiseReductionMode": 2,  # 增强降噪
                        "FrameRate": 25.0,      # 稍低的目标帧率，更适合稳定性
                        "ExposureTime": 60000,  # 更长的曝光时间，OV5647在低光下需要这个
                        "AnalogueGain": 6.0     # OV5647适用的模拟增益
                    }
                )
                
                # 初始化夜视模式设置
                global night_vision_enabled, night_vision_auto
                try:
                    night_vision_enabled = True  # 默认启用夜视功能
                    night_vision_auto = True     # 默认为自动模式
                    logger.info("夜视功能已初始化")
                except Exception as e:
                    logger.warning(f"初始化夜视功能时出错: {e}")
                
                picam2.configure(config)
                time.sleep(0.5)
                
                # 启动摄像头
                picam2.start()
                logger.info(f"摄像头初始化成功 (尝试 {attempt+1}/3)")
                
                # 丢弃前几帧
                for _ in range(10):  # 增加丢弃帧数
                    picam2.capture_array()
                    time.sleep(0.05)  # 稍微延长间隔，给OV5647更多时间稳定
                
                return True
                
            except Exception as e:
                logger.error(f"摄像头初始化尝试 {attempt+1}/3 失败: {e}")
                if picam2 is not None:
                    try:
                        picam2.stop()
                    except:
                        pass
                    picam2 = None
                time.sleep(2)
        
        return False
        
    except Exception as e:
        logger.error(f"初始化摄像头过程中发生错误: {e}")
        return False

def adjust_colors_fast(frame):
    """使用查找表快速调整颜色"""
    try:
        if frame is None or frame.size == 0:
            return None
            
        # 使用OpenCV的split/LUT/merge操作，这些操作经过高度优化
        b, g, r = cv2.split(frame)
        b = cv2.LUT(b, b_lut)
        r = cv2.LUT(r, r_lut)
        
        # 使用优化的合并操作
        adjusted = cv2.merge([b, g, r])
        
        # 使用查找表进行亮度和对比度调整，避免逐像素计算
        adjusted = cv2.LUT(adjusted, alpha_beta_lut)
        
        return adjusted
    except Exception as e:
        logger.error(f"快速颜色调整出错: {e}")
        return frame

def apply_sharpening(frame):
    """应用预计算的锐化核以提高速度"""
    try:
        if frame is None:
            return frame
            
        # 直接使用预计算的锐化核，避免每次重新创建
        sharpened = cv2.filter2D(frame, -1, sharpening_kernel)
        
        # 使用加权混合但减少计算复杂度
        result = cv2.addWeighted(frame, 0.7, sharpened, 0.3, 0)
        
        return result
    except Exception as e:
        logger.error(f"锐化处理出错: {e}")
        return frame

def detect_low_light(frame):
    """超级简化的低光检测算法 - 专注于稳定性和可靠性，增强防闪烁效果"""
    global last_light_level, light_threshold
    
    try:
        if frame is None or frame.size == 0:
            return False
            
        # 取中心区域进行分析，减少计算量并关注主体区域
        h, w = frame.shape[:2]
        center_y, center_x = h // 2, w // 2
        size = min(h, w) // 4  # 取中心1/4区域
        
        center_roi = frame[center_y-size:center_y+size, center_x-size:center_x+size]
        
        # 直接转灰度后计算平均亮度 - 最简单可靠的方法
        gray = cv2.cvtColor(center_roi, cv2.COLOR_BGR2GRAY)
        avg_brightness = np.mean(gray)
        
        # 添加更强的平滑，确保稳定过渡
        if not hasattr(detect_low_light, 'smooth_brightness'):
            detect_low_light.smooth_brightness = avg_brightness
            detect_low_light.last_result = False
            detect_low_light.stability_counter = 0
        
        # 使用更长时间的历史平滑，98%旧值+2%新值，确保超级平稳的过渡
        detect_low_light.smooth_brightness = detect_low_light.smooth_brightness * 0.98 + avg_brightness * 0.02
        
        last_light_level = detect_low_light.smooth_brightness
        
        # 添加滞后效应（Hysteresis）减少在临界值附近的状态波动
        # 如果当前是低光状态，需要更高的亮度才能退出；如果当前非低光状态，需要更低的亮度才能进入
        current_threshold = light_threshold - 5 if detect_low_light.last_result else light_threshold + 5
        
        # 初步判断
        is_low_light_current = detect_low_light.smooth_brightness < current_threshold
        
        # 状态稳定性增强 - 只有当连续多帧判断结果相同时才改变状态
        if is_low_light_current == detect_low_light.last_result:
            # 判断结果一致，重置计数器
            detect_low_light.stability_counter = 0
        else:
            # 判断结果不一致，增加计数器
            detect_low_light.stability_counter += 1
            # 需要至少5帧一致的判断才改变状态 - 进一步减少频繁切换
            if detect_low_light.stability_counter >= 5:
                detect_low_light.last_result = is_low_light_current
                detect_low_light.stability_counter = 0
        
        # 减少日志频率，每90帧记录一次
        if frame_counter % 90 == 0:
            logger.info(f"光线水平: {detect_low_light.smooth_brightness:.1f}, 阈值: {current_threshold}, 低光状态: {detect_low_light.last_result}")
        
        return detect_low_light.last_result
        
    except Exception as e:
        logger.error(f"光线检测错误: {e}")
        # 出错时保持之前的判断结果
        return hasattr(detect_low_light, 'last_result') and detect_low_light.last_result

def apply_night_vision(frame):
    """OV5647专用夜视增强 - 防闪烁优化版本"""
    global night_vision_strength, enable_green_tint, night_vision_buffer, frame_counter
    global motion_detected, reduced_processing_until
    
    try:
        if frame is None or frame.size == 0:
            return frame
        
        # 每隔10帧才清理一次内存计数，减少计数器操作频率
        if frame_counter % 10 == 0:
            night_vision_buffer["frames_since_reset"] += 1
        
        # 延长清理间隔，仅每1500帧清理一次缓存
        if night_vision_buffer["frames_since_reset"] > 1500:
            night_vision_buffer["green_mask"] = None
            night_vision_buffer["reduced"] = None
            night_vision_buffer["last_frame_shape"] = None
            night_vision_buffer["frames_since_reset"] = 0
        
        # 添加亮度平滑处理，减少帧间亮度波动
        if not hasattr(apply_night_vision, 'last_brightness_factor'):
            apply_night_vision.last_brightness_factor = 1.6
            apply_night_vision.last_brightness_offset = 12
        
        # 目标亮度参数
        target_brightness_factor = 1.6  # 适中的亮度提升
        target_brightness_offset = 12
        
        # 平滑过渡亮度参数 (90%旧值 + 10%新值)
        apply_night_vision.last_brightness_factor = apply_night_vision.last_brightness_factor * 0.9 + target_brightness_factor * 0.1
        apply_night_vision.last_brightness_offset = apply_night_vision.last_brightness_offset * 0.9 + target_brightness_offset * 0.1
        
        # 使用平滑后的亮度参数
        brightness_factor = apply_night_vision.last_brightness_factor
        brightness_offset = apply_night_vision.last_brightness_offset
        
        # 检查当前帧率，根据帧率确定处理级别
        with stats_lock:
            current_fps = fps_stats.get("current", 20)
        
        # 检查是否检测到运动或是否处于降级处理阶段
        use_simple_mode = motion_detected or time.time() < reduced_processing_until
        
        # 防止处理级别频繁变化
        if not hasattr(apply_night_vision, 'processing_mode'):
            apply_night_vision.processing_mode = 'normal'  # 'simple', 'normal', 'enhanced'
            apply_night_vision.mode_change_time = time.time()
        
        current_time = time.time()
        mode_duration = current_time - apply_night_vision.mode_change_time
        
        # 只有当处理模式持续至少2秒后才考虑改变 - 避免频繁切换导致闪烁
        if mode_duration > 2.0 and not use_simple_mode:
            # 根据帧率选择处理模式
            if current_fps < 10 and apply_night_vision.processing_mode != 'simple':
                apply_night_vision.processing_mode = 'simple'
                apply_night_vision.mode_change_time = current_time
                logger.info(f"夜视处理模式切换为简单模式 (当前帧率: {current_fps:.1f})")
            elif current_fps >= 10 and current_fps < 18 and apply_night_vision.processing_mode != 'normal':
                apply_night_vision.processing_mode = 'normal'
                apply_night_vision.mode_change_time = current_time
                logger.info(f"夜视处理模式切换为标准模式 (当前帧率: {current_fps:.1f})")
            elif current_fps >= 18 and apply_night_vision.processing_mode != 'enhanced':
                apply_night_vision.processing_mode = 'enhanced'
                apply_night_vision.mode_change_time = current_time
                logger.info(f"夜视处理模式切换为增强模式 (当前帧率: {current_fps:.1f})")
        
        # 如果检测到运动，强制使用简单模式
        processing_mode = 'simple' if use_simple_mode else apply_night_vision.processing_mode
        
        # 应用亮度增强 - 对所有模式都执行
        enhanced = cv2.convertScaleAbs(frame, alpha=brightness_factor, beta=brightness_offset)
        
        # 简单模式 - 最基础的处理
        if processing_mode == 'simple':
            # 如果需要绿色夜视，使用极简方法
            if enable_green_tint:
                # 增强绿色效果：减弱红蓝通道，增强绿色通道
                enhanced[:,:,2] = (enhanced[:,:,2] * 0.4).astype(np.uint8)  # 红色通道更弱
                enhanced[:,:,0] = (enhanced[:,:,0] * 0.4).astype(np.uint8)  # 蓝色通道更弱
                green_boost = np.clip(enhanced[:,:,1] * 1.5, 0, 255).astype(np.uint8)  # 绿色通道增强更多
                enhanced[:,:,1] = green_boost
            
            return enhanced
        
        # 标准模式 - 常规处理
        if processing_mode == 'normal':
            # 根据帧计数隔帧应用降噪 - 确保稳定性
            if frame_counter % 2 == 0:  # 每两帧才处理一次
                enhanced = cv2.GaussianBlur(enhanced, (3, 3), 0)
            
            # 绿色夜视处理
            if enable_green_tint:
                # 稳定的绿色蒙版处理
                current_shape = enhanced.shape
                if (night_vision_buffer["green_mask"] is None or 
                    night_vision_buffer["last_frame_shape"] != current_shape):
                    night_vision_buffer["green_mask"] = np.zeros_like(enhanced)
                    night_vision_buffer["green_mask"][:,:,1] = 160  # 增强绿色通道强度从100提高到160
                    night_vision_buffer["last_frame_shape"] = current_shape
                
                # 增强混合因子，使绿色效果更明显
                if not hasattr(apply_night_vision, 'blend_factor'):
                    apply_night_vision.blend_factor = min(night_vision_strength * 0.7, 0.8)  # 增加基础系数和上限
                else:
                    # 平滑过渡混合因子
                    target_blend = min(night_vision_strength * 0.7, 0.8)  # 增加目标混合系数
                    apply_night_vision.blend_factor = apply_night_vision.blend_factor * 0.9 + target_blend * 0.1
                
                # 应用绿色效果
                enhanced = cv2.addWeighted(enhanced, 1.0, 
                                         night_vision_buffer["green_mask"], 
                                         apply_night_vision.blend_factor, 0)
                
                # 平滑通道参数过渡，避免突变 - 降低红蓝通道强度
                if not hasattr(apply_night_vision, 'r_factor') or not hasattr(apply_night_vision, 'b_factor'):
                    apply_night_vision.r_factor = 0.35  # 减弱红色通道从0.5降到0.35
                    apply_night_vision.b_factor = 0.35  # 减弱蓝色通道从0.5降到0.35
                
                enhanced[:,:,0] = (enhanced[:,:,0] * apply_night_vision.b_factor).astype(np.uint8)
                enhanced[:,:,2] = (enhanced[:,:,2] * apply_night_vision.r_factor).astype(np.uint8)
            
            return enhanced
        
        # 增强模式 - 额外应用对比度增强
        if processing_mode == 'enhanced':
            # 应用标准模式的所有处理
            if frame_counter % 2 == 0:
                enhanced = cv2.GaussianBlur(enhanced, (3, 3), 0)
            
            # 绿色夜视效果
            if enable_green_tint:
                if (night_vision_buffer["green_mask"] is None or 
                    night_vision_buffer["last_frame_shape"] != enhanced.shape):
                    night_vision_buffer["green_mask"] = np.zeros_like(enhanced)
                    night_vision_buffer["green_mask"][:,:,1] = 180  # 在增强模式下使用更强的绿色强度
                    night_vision_buffer["last_frame_shape"] = enhanced.shape
                
                if not hasattr(apply_night_vision, 'blend_factor'):
                    apply_night_vision.blend_factor = min(night_vision_strength * 0.7, 0.8)  # 增强混合因子
                else:
                    target_blend = min(night_vision_strength * 0.7, 0.8)
                    apply_night_vision.blend_factor = apply_night_vision.blend_factor * 0.9 + target_blend * 0.1
                
                enhanced = cv2.addWeighted(enhanced, 1.0, 
                                         night_vision_buffer["green_mask"], 
                                         apply_night_vision.blend_factor, 0)
                
                if not hasattr(apply_night_vision, 'r_factor') or not hasattr(apply_night_vision, 'b_factor'):
                    apply_night_vision.r_factor = 0.3  # 进一步减弱红色
                    apply_night_vision.b_factor = 0.3  # 进一步减弱蓝色
                
                enhanced[:,:,0] = (enhanced[:,:,0] * apply_night_vision.b_factor).astype(np.uint8)
                enhanced[:,:,2] = (enhanced[:,:,2] * apply_night_vision.r_factor).astype(np.uint8)
                
                # 增强模式下额外增强绿色通道
                green_boost = np.clip(enhanced[:,:,1] * 1.2, 0, 255).astype(np.uint8)
                enhanced[:,:,1] = green_boost
            
            # 每3帧应用一次高级增强，减轻计算负担
            if frame_counter % 3 == 0:
                # 使用CLAHE增强局部对比度，只处理亮度通道
                lab = cv2.cvtColor(enhanced, cv2.COLOR_BGR2LAB)
                l, a, b = cv2.split(lab)
                
                # 固定的CLAHE参数，避免参数变化引起的闪烁
                clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(2, 2))
                cl = clahe.apply(l)
                
                enhanced_lab = cv2.merge((cl, a, b))
                enhanced = cv2.cvtColor(enhanced_lab, cv2.COLOR_LAB2BGR)
            
            return enhanced
        
        # 默认返回简单增强的帧
        return enhanced
    
    except Exception as e:
        logger.error(f"夜视模式处理出错: {e}")
        return frame  # 发生错误时返回原始帧

def check_and_update_night_vision(frame):
    """检查是否需要启用或关闭夜视模式 - 增强防闪烁的稳定性处理"""
    global night_vision_enabled, night_vision_auto, night_vision_active
    
    try:
        # 如果夜视功能未启用，直接返回False
        if not night_vision_enabled:
            if night_vision_active:
                # 如果之前是激活状态，现在要关闭
                with night_vision_lock:
                    night_vision_active = False
                    logger.info("夜视模式已关闭")
            return False
        
        current_time = time.time()
        
        # 非自动模式，直接使用设置的状态
        if not night_vision_auto:
            # 如果之前不是激活状态，现在启用
            if not night_vision_active:
                with night_vision_lock:
                    night_vision_active = True
                    logger.info("夜视模式已手动开启")
            return True
        
        # 自动模式下，通过光线检测决定
        # 检测光线强度
        low_light = detect_low_light(frame)
        
        # 获取当前状态
        current_status = night_vision_active
        
        # 大幅增加状态切换的稳定时间 - 至少5秒才允许切换一次状态
        # 这能有效减少在临界光线条件下的反复切换导致的闪烁
        if not hasattr(check_and_update_night_vision, 'last_change_time'):
            check_and_update_night_vision.last_change_time = 0
            
        if low_light != current_status and current_time - check_and_update_night_vision.last_change_time > 5.0:
            # 添加状态计数器，确保多次连续检测到同一状态才切换
            if not hasattr(check_and_update_night_vision, 'state_counter'):
                check_and_update_night_vision.state_counter = 0
                check_and_update_night_vision.pending_state = low_light
            
            # 如果待处理状态改变，重置计数器
            if check_and_update_night_vision.pending_state != low_light:
                check_and_update_night_vision.state_counter = 0
                check_and_update_night_vision.pending_state = low_light
            else:
                # 状态一致，增加计数器
                check_and_update_night_vision.state_counter += 1
                
                # 需要连续3次检测到同一状态才真正切换 - 进一步防止临时波动
                if check_and_update_night_vision.state_counter >= 3:
                    with night_vision_lock:
                        night_vision_active = low_light
                        check_and_update_night_vision.last_change_time = current_time
                        check_and_update_night_vision.state_counter = 0
                        
                        if low_light:
                            logger.info("检测到持续光线不足，启用夜视模式")
                        else:
                            logger.info("检测到持续光线充足，关闭夜视模式")
        
        return night_vision_active
    
    except Exception as e:
        logger.error(f"检查夜视状态出错: {e}")
        return night_vision_active  # 保持当前状态

def detect_motion(frame):
    """检测帧中的运动，简化版本仅用于夜视模式调整处理级别"""
    global motion_detected, motion_frame_buffer, last_motion_time, reduced_processing_until
    
    try:
        # 简单的运动检测实现
        current_time = time.time()
        
        # 检查运动检测间隔以减少处理负担
        if current_time - last_motion_time < motion_detection_interval:
            return False
            
        # 初始化运动检测缓冲区
        if motion_frame_buffer is None or motion_frame_buffer.shape != frame.shape:
            # 首次运行或帧大小变化，初始化缓冲区
            motion_frame_buffer = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            motion_detected = False
            return False
        
        # 将当前帧转换为灰度
        current_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # 对比当前帧与缓冲帧
        frame_diff = cv2.absdiff(current_gray, motion_frame_buffer)
        
        # 应用阈值
        _, thresholded = cv2.threshold(frame_diff, motion_detection_threshold, 255, cv2.THRESH_BINARY)
        
        # 减少噪点影响
        thresholded = cv2.erode(thresholded, None, iterations=2)
        thresholded = cv2.dilate(thresholded, None, iterations=4)
        
        # 计算非零像素的百分比（移动区域）
        motion_ratio = cv2.countNonZero(thresholded) / (frame.shape[0] * frame.shape[1])
        
        # 更新缓冲帧 (使用当前帧的70%和缓冲帧的30%进行混合，减少噪点影响)
        motion_frame_buffer = cv2.addWeighted(current_gray, 0.7, motion_frame_buffer, 0.3, 0)
        
        # 更新运动状态
        old_motion_state = motion_detected
        motion_detected = motion_ratio > 0.01  # 如果超过1%的像素有变化，认为有运动
        
        # 如果检测到运动，临时降低处理复杂度以提高响应性
        if motion_detected:
            last_motion_time = current_time
            reduced_processing_until = current_time + 1.0  # 降级处理1秒
            
            # 记录运动检测状态变化
            if not old_motion_state and motion_detected:
                logger.debug(f"检测到运动，移动比例: {motion_ratio*100:.2f}%")
        
        return motion_detected
        
    except Exception as e:
        logger.error(f"运动检测出错: {e}")
        return False

def enhance_frame(frame):
    """优化的帧增强函数，使用简单可靠的处理流程"""
    global previous_frame, night_vision_active, frame_counter, motion_detected, reduced_processing_until
    
    if frame is None or frame.size == 0:
        return None
    
    try:
        # 增加帧计数器
        frame_counter += 1
        
        # 更新夜视模式状态
        night_mode_enabled = check_and_update_night_vision(frame)
        
        # 根据夜视模式选择处理流程
        if night_mode_enabled:
            # 如果检测到运动或者正在降低处理级别，使用简单处理
            if motion_detected or time.time() < reduced_processing_until:
                # 使用简单模式的夜视增强
                logger.debug("使用简单夜视模式 - 已检测到运动")
            
            # 应用夜视模式增强
            return apply_night_vision(frame)
        else:
            # 标准图像处理流程
            return adjust_colors_fast(frame)
    
    except Exception as e:
        logger.error(f"帧增强处理出错: {e}")
        return frame  # 返回原始帧，确保不中断

def update_fps_stats(frame_time):
    """计算并更新FPS统计信息"""
    global fps_stats, frame_times
    with stats_lock:
        # 保留最近10帧的时间，减少计算量
        frame_times.append(frame_time)
        if len(frame_times) > 10:
            frame_times.pop(0)
            
        # 计算FPS
        if len(frame_times) > 1:
            # 使用简单的FPS计算方法
            elapsed = frame_times[-1] - frame_times[0]
            if elapsed > 0:
                fps = (len(frame_times) - 1) / elapsed
                
                fps_stats["current"] = fps
                if fps_stats["min"] == 0 or fps < fps_stats["min"]:
                    fps_stats["min"] = fps
                if fps > fps_stats["max"]:
                    fps_stats["max"] = fps
                # 使用简单平均
                fps_stats["avg"] = fps

def process_frame(frame):
    """处理捕获的帧 - 优化版本，增加运动检测和动态处理"""
    global frame_counter, night_vision_active, reduced_processing_until, motion_detected, motion_frame_buffer, last_motion_time
    
    if frame is None:
        return None
        
    try:
        # 增加帧计数
        frame_counter += 1
        
        # 检测是否有运动（仅在夜视模式下）
        if night_vision_active and frame_counter % 5 == 0:  # 每5帧检查一次运动
            try:
                detect_motion(frame)
            except Exception as e:
                logger.error(f"运动检测错误: {e}")
        
        # 夜视模式检查
        is_night_vision = check_and_update_night_vision(frame)
        
        # 在夜视模式下，根据运动状态或临时降级标志决定处理级别
        result_frame = None
        if is_night_vision:
            # 如果检测到运动或帧率过低，减少处理复杂度
            if motion_detected or time.time() < reduced_processing_until:
                result_frame = apply_night_vision(frame)
            else:
                result_frame = apply_night_vision(frame)
        else:
            # 标准图像增强
            result_frame = enhance_frame(frame)
            
        # 每隔2帧才添加文字信息，减少处理负担
        if frame_counter % 2 == 0 and result_frame is not None:
            # 添加时间戳和FPS信息 (高效版本)
            with stats_lock:
                current_fps = fps_stats.get("current", 0)
                
            # 只在有足够帧率时显示信息，避免低帧率时的额外负担
            if current_fps >= 10 or frame_counter % 10 == 0:
                current_time = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
                fps_text = f"FPS: {current_fps:.1f}"
                
                # 使用更高效的文字渲染方式
                cv2.putText(result_frame, current_time, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 
                          0.5, (0, 0, 0), 2, cv2.LINE_AA)  # 黑色外边框
                cv2.putText(result_frame, current_time, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 
                          0.5, (255, 255, 255), 1, cv2.LINE_AA)
                
                cv2.putText(result_frame, fps_text, (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 
                          0.5, (0, 0, 0), 2, cv2.LINE_AA)
                cv2.putText(result_frame, fps_text, (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 
                          0.5, (255, 255, 255), 1, cv2.LINE_AA)
        
        return result_frame
        
    except Exception as e:
        logger.error(f"处理帧出错: {e}")
        return frame  # 发生错误时返回原始帧

def is_valid_frame(frame):
    """快速帧验证，减少处理时间提高帧率"""
    if frame is None or frame.size == 0:
        return False
    
    try:
        # 仅执行基本检查，减少处理时间
        # 检查帧形状
        if len(frame.shape) != 3:
            return False
        
        # 只检查图像部分区域以加速处理
        # 从中心取样本区域
        h, w = frame.shape[:2]
        center_y, center_x = h // 2, w // 2
        sample_size = 50  # 采样区域大小
        
        sample = frame[
            max(0, center_y - sample_size):min(h, center_y + sample_size),
            max(0, center_x - sample_size):min(w, center_x + sample_size)
        ]
        
        # 快速检查样本区域
        avg_value = np.mean(sample)
        if avg_value < 5:  # 几乎全黑
            return False
            
        # 快速检查颜色分布
        std_value = np.std(sample)
        if std_value < 3:  # 几乎单色
            return False
            
        return True
    except Exception as e:
        logger.error(f"帧验证错误: {e}")
        return False

def capture_continuous():
    """优化的帧捕获函数，专注于提高帧率和稳定性，增加资源监控"""
    global picam2, running, latest_frame, last_frame_time, frame_counter
    global last_resource_check, memory_reset_needed
    
    logger.info("开始后台帧捕获线程")
    
    # 记录性能数据
    frame_times = []
    last_perf_check = time.time()
    
    # 添加关键模块的导入
    try:
        import psutil
        has_psutil = True
        logger.info("已启用psutil资源监控功能")
    except ImportError:
        has_psutil = False
        logger.warning("未找到psutil模块，资源监控将不可用。请使用 'pip install psutil' 安装")
    
    # 定义内联资源监控函数，避免全局函数未定义的问题
    def inline_monitor_resources():
        try:
            if not has_psutil:
                return {"memory_percent": 0, "available_memory_mb": 0, "cpu_temp": 0, "cpu_percent": 0, "reset_needed": False}
                
            # 获取内存使用情况
            memory = psutil.virtual_memory()
            memory_percent = memory.percent
            available_memory_mb = memory.available / (1024 * 1024)
            
            # 获取CPU温度
            cpu_temp = 0
            try:
                with open('/sys/class/thermal/thermal_zone0/temp', 'r') as f:
                    cpu_temp = int(f.read().strip()) / 1000.0
            except:
                pass
                
            # 获取CPU使用率
            try:
                cpu_percent = psutil.cpu_percent(interval=0.1)
            except:
                cpu_percent = 0
                
            # 检测内存问题
            need_reset = False
            if memory_percent > 75 or available_memory_mb < 400:
                # 避免频繁重置，至少间隔5分钟
                current_time = time.time()
                if current_time - last_memory_reset > 300:
                    need_reset = True
                    logger.warning(f"内存不足 (使用率: {memory_percent:.1f}%, 可用: {available_memory_mb:.0f}MB)，将执行内存优化")
            
            return {
                "memory_percent": memory_percent,
                "available_memory_mb": available_memory_mb,
                "cpu_temp": cpu_temp,
                "cpu_percent": cpu_percent,
                "reset_needed": need_reset
            }
        except Exception as e:
            logger.error(f"资源监控出错: {e}")
            return {"memory_percent": 0, "available_memory_mb": 0, "cpu_temp": 0, "cpu_percent": 0, "reset_needed": False}
    
    # 定义内联内存重置函数
    def inline_reset_memory():
        try:
            logger.info("执行简化版内存优化...")
            
            # 清理夜视缓存
            global night_vision_buffer
            night_vision_buffer["green_mask"] = None
            night_vision_buffer["reduced"] = None
            night_vision_buffer["last_frame_shape"] = None
            night_vision_buffer["frames_since_reset"] = 0
            
            # 简单的垃圾收集
            import gc
            gc.collect()
            
            # 重置标志和时间戳
            global last_memory_reset
            last_memory_reset = time.time()
            logger.info("内存优化完成")
        except Exception as e:
            logger.error(f"重置内存使用出错: {e}")
    
    while running:
        try:
            if picam2 is None:
                if not init_camera():
                    time.sleep(1)
                    continue
            
            # 开始计时
            frame_start_time = time.time()
            current_time = frame_start_time
            
            # 定期调用性能调整
            if current_time - last_perf_check > 2.0:  # 每2秒检查一次
                adjust_performance()
                last_perf_check = current_time
            
            # 定期监控系统资源 - 使用内联函数替代全局函数
            if has_psutil and current_time - last_resource_check > resource_monitor_interval:
                resource_info = inline_monitor_resources()
                last_resource_check = current_time
                
                # 如果内存使用过高，执行优化
                if resource_info["reset_needed"]:
                    inline_reset_memory()
                    
                # 如果CPU温度过高，降低处理级别
                if resource_info["cpu_temp"] > 75:
                    global processing_level, reduce_processing
                    processing_level = 0
                    reduce_processing = True
                    logger.warning(f"CPU温度过高 ({resource_info['cpu_temp']:.1f}°C)，降低处理级别以避免过热")
            
            try:
                # 尽量减少锁的持有时间
                with camera_lock:
                    if picam2 is None:
                        continue
                    frame = picam2.capture_array()
                
                if frame is not None and frame.size > 0:
                    # 每一帧都做相同处理，保持一致性
                    processed_frame = process_frame(frame)
                    
                    if processed_frame is not None:
                        # 更新帧数据
                        with frame_lock:
                            latest_frame = processed_frame.copy()  # 使用copy避免引用问题
                            last_frame_time = time.time()
                            update_fps_stats(time.time())
                        
                        # 编码缓存
                        encode_and_cache_frame(processed_frame)
                        
                        # 记录帧处理时间
                        frame_times.append(time.time() - frame_start_time)
                        if len(frame_times) > 30:
                            # 每30帧记录一次平均处理时间
                            avg_time = sum(frame_times) / len(frame_times)
                            logger.debug(f"平均帧处理时间: {avg_time*1000:.1f}ms，约等于 {1/avg_time:.1f}fps")
                            frame_times = []  # 清空列表避免内存增长
                    
                    # 适当释放帧引用，帮助垃圾回收
                    del frame
                    if processed_frame is not None:
                        del processed_frame
                    
                    # 动态休眠控制帧率
                    elapsed = time.time() - frame_start_time
                    if elapsed < 0.03:  # 目标30+fps
                        # 非常短的休眠以节省CPU，同时保持高帧率
                        time.sleep(0.001)
                
            except Exception as e:
                logger.error(f"捕获帧异常: {e}")
                time.sleep(0.1)
                
        except Exception as e:
            logger.error(f"帧捕获线程错误: {e}")
            time.sleep(0.1)

def encode_and_cache_frame(frame):
    """编码并缓存当前帧为JPEG格式"""
    global encoded_frames_cache
    
    if frame is None:
        logger.warning("无法编码空帧")
        return
    
    try:
        # 确保清晰的图像质量，但避免过大
        _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
        
        with encoded_frames_cache_lock:
            # 单帧缓存，确保最新
            encoded_frames_cache = [{"time": time.time(), "data": buffer.tobytes()}]
    except Exception as e:
        logger.error(f"编码帧出错: {e}")

def get_cached_frame():
    """获取最新的缓存帧"""
    global encoded_frames_cache
    
    with encoded_frames_cache_lock:
        if not encoded_frames_cache:
            return b''
        return encoded_frames_cache[0]["data"]

def generate_frames():
    """优化的帧生成器，提高帧率和减少闪烁"""
    global running, active_clients
    client_id = time.time()
    target_interval = 1.0 / 30.0  # 目标30fps
    
    with clients_lock:
        active_clients += 1
        logger.info(f"客户端 {client_id:.2f} 连接，当前活跃客户端: {active_clients}")
    
    try:
        last_frame_time = time.time()
        while running:
            try:
                # 保持固定间隔发送帧
                now = time.time()
                time_to_wait = target_interval - (now - last_frame_time)
                
                if time_to_wait > 0:
                    # 使用极短的休眠
                    time.sleep(0.001)
                    continue
                
                # 重置时间
                last_frame_time = now
                
                # 获取缓存帧
                buffer = get_cached_frame()
                if buffer is None:
                    continue
                
                # 发送帧数据
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + buffer + b'\r\n')
                
            except Exception as e:
                logger.error(f"生成帧异常: {e}")
                time.sleep(0.03)
                
    except Exception as e:
        logger.error(f"生成帧异常: {e}")
    finally:
        with clients_lock:
            active_clients -= 1
            logger.info(f"客户端 {client_id:.2f} 断开，当前活跃客户端: {active_clients}")

@app.route('/')
def index():
    # 获取当前IP和服务URL
    ip = get_ip_address()
    service_url = f"http://{ip}:8000/video_feed"
    
    return f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>摄像头流</title>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body {{
                font-family: Arial, sans-serif;
                margin: 0;
                padding: 20px;
                text-align: center;
                background-color: #f0f0f0;
            }}
            h1 {{
                color: #333;
            }}
            .container {{
                max-width: 800px;
                margin: 0 auto;
                background: white;
                padding: 20px;
                border-radius: 8px;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }}
            img {{
                max-width: 100%;
                border-radius: 4px;
            }}
            .info {{
                margin-top: 20px;
                padding: 10px;
                background: #e9f7fe;
                border-radius: 4px;
                text-align: left;
            }}
            .reload {{
                display: inline-block;
                margin-top: 10px;
                padding: 8px 15px;
                background-color: #4CAF50;
                color: white;
                border: none;
                border-radius: 4px;
                cursor: pointer;
            }}
            .reload:hover {{
                background-color: #45a049;
            }}
        </style>
        <script>
            // 自动重连功能
            function setupAutoReconnect() {{
                const img = document.getElementById('stream');
                let reconnectTimer = null;
                let errorCounter = 0;
                
                img.onload = function() {{
                    console.log('视频流已加载');
                    errorCounter = 0;
                }};
                
                img.onerror = function() {{
                    errorCounter++;
                    console.log(`视频流错误 (${{errorCounter}}次)`);
                    
                    if (errorCounter <= 5) {{
                        if (reconnectTimer) clearTimeout(reconnectTimer);
                        
                        reconnectTimer = setTimeout(function() {{
                            console.log('尝试重新加载视频流...');
                            img.src = '/video_feed?t=' + new Date().getTime();
                        }}, 2000);
                    }} else {{
                        console.log('多次重连失败，请手动刷新页面');
                        document.getElementById('reconnect-msg').style.display = 'block';
                    }}
                }};
            }}
            
            window.onload = function() {{
                setupAutoReconnect();
                
                // 手动重新加载
                document.getElementById('reload-btn').onclick = function() {{
                    location.reload();
                }};
            }};
        </script>
    </head>
    <body>
        <div class="container">
            <h1>树莓派摄像头流</h1>
            <img id="stream" src="/video_feed" alt="摄像头流">
            <div id="reconnect-msg" style="display: none; color: red; margin-top: 10px;">
                视频流连接出现问题。
                <button id="reload-btn" class="reload">刷新页面</button>
            </div>
            <div class="info">
                <p><strong>服务器IP:</strong> {ip}</p>
                <p><strong>视频流URL:</strong> {service_url}</p>
                <p><strong>活跃连接:</strong> {active_clients}/{max_clients}</p>
                <p>树莓派摄像头服务正在运行</p>
            </div>
        </div>
    </body>
    </html>
    """

@app.route('/video_feed')
def video_feed():
    # 限制最大客户端数量
    with clients_lock:
        if active_clients >= max_clients:
            return "达到最大连接数，请稍后再试", 503
    
    # 返回视频流
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/status')
def status():
    """返回服务器状态信息"""
    with stats_lock:
        status_data = {
            "active_clients": active_clients,
            "max_clients": max_clients,
            "fps": fps_stats,
            "uptime": time.time() - service_start_time,
            "camera_status": "running" if picam2 is not None else "stopped",
            "server_ip": get_ip_address(),
            "reduce_processing": reduce_processing
        }
    return status_data

@app.route('/reset_camera', methods=['POST'])
def reset_camera_endpoint():
    """手动重置摄像头的API端点"""
    success = reset_camera()
    if success:
        return {"status": "success", "message": "摄像头已重置"}
    else:
        return {"status": "error", "message": "摄像头重置失败"}, 500

@app.route('/restart_service', methods=['POST'])
def restart_service_endpoint():
    """重启整个视频流服务的API端点"""
    try:
        logger.info("收到重启服务请求，准备重启服务...")
        # 这里我们创建一个独立进程来重启服务，这样当前进程可以正常返回响应
        subprocess.Popen(['sudo', 'systemctl', 'restart', 'camera-service'], 
                          stdout=subprocess.PIPE, 
                          stderr=subprocess.PIPE)
        return {"status": "success", "message": "重启服务请求已发送，服务即将重启"}
    except Exception as e:
        logger.error(f"重启服务失败: {e}")
        return {"status": "error", "message": f"重启服务失败: {e}"}, 500

@app.route('/debug')
def debug_info():
    """返回详细的调试信息页面"""
    with stats_lock, clients_lock, night_vision_lock:
        debug_html = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>摄像头调试信息</title>
            <meta charset="utf-8">
            <meta http-equiv="refresh" content="5">
            <style>
                body {{ font-family: monospace; padding: 20px; }}
                .stat {{ margin-bottom: 5px; }}
                .error {{ color: red; }}
                .good {{ color: green; }}
            </style>
        </head>
        <body>
            <h1>摄像头服务调试信息</h1>
            <div class="stat">摄像头状态: <span class="{'good' if picam2 is not None else 'error'}">{('运行中' if picam2 is not None else '未运行')}</span></div>
            <div class="stat">服务运行时间: {int(time.time() - service_start_time)}秒</div>
            <div class="stat">最后一帧时间: {int(time.time() - last_frame_time)}秒前</div>
            <div class="stat">活跃客户端: {active_clients}/{max_clients}</div>
            <div class="stat">当前FPS: <span class="{'error' if fps_stats['current'] < 5 else 'good'}">{fps_stats['current']:.2f}</span></div>
            <div class="stat">最小FPS: {fps_stats['min']:.2f}</div>
            <div class="stat">最大FPS: {fps_stats['max']:.2f}</div>
            <div class="stat">处理模式: {'简化' if reduce_processing else '完整'}</div>
            <div class="stat">服务器IP: {get_ip_address()}</div>
            <div>
                <h3>操作</h3>
                <form action="/reset_camera" method="post">
                    <button type="submit">重置摄像头</button>
                </form>
            </div>
            <div class="section">
                <h2>夜视模式状态</h2>
                <div class="stat">夜视功能: <span class="{'good' if night_vision_enabled else 'error'}">{('已启用' if night_vision_enabled else '未启用')}</span></div>
                <div class="stat">夜视模式: <span class="{('good' if night_vision_auto else '')}">{('自动' if night_vision_auto else '手动')}</span></div>
                <div class="stat">当前状态: <span class="{'good' if night_vision_active else ''}">{('活跃' if night_vision_active else '未活跃')}</span></div>
                <div class="stat">当前光线水平: <span>{last_light_level:.1f}</span></div>
                <div class="stat">光线阈值: <span>{light_threshold}</span></div>
                <div class="stat">夜视强度: <span>{night_vision_strength * 100:.0f}%</span></div>
                <div class="stat">绿色夜视效果: <span>{('开启' if enable_green_tint else '关闭')}</span></div>
            </div>
            
            <div class="section">
                <h2>操作</h2>
                <form action="/toggle_night_vision" method="post">
                    <button type="submit">切换夜视功能</button>
                </form>
                <form action="/toggle_night_vision_mode" method="post">
                    <button type="submit">切换夜视模式</button>
                </form>
                <form action="/set_night_vision_strength" method="post">
                    <label for="strength">夜视增强强度 (0.1-1.0):</label>
                    <input type="number" id="strength" name="strength" min="0.1" max="1.0" step="0.01" value="{night_vision_strength:.1f}">
                    <button type="submit">设置夜视增强强度</button>
                </form>
                <form action="/toggle_green_night_vision" method="post">
                    <button type="submit">切换绿色夜视效果</button>
                </form>
                <form action="/set_light_threshold" method="post">
                    <label for="threshold">光线阈值 (10-150):</label>
                    <input type="number" id="threshold" name="threshold" min="10" max="150" value="{light_threshold:.0f}">
                    <button type="submit">设置光线阈值</button>
                </form>
            </div>
        </body>
        </html>
        """
        return debug_html

def health_check():
    """健康检查函数，监控和维护系统状态"""
    global picam2, running, last_frame_time
    last_check = time.time()
    
    while running:
        current_time = time.time()
        
        # 每隔设定时间进行一次健康检查
        if current_time - last_check >= health_check_interval:
            logger.info("执行健康检查...")
            
            # 检查最后一帧的时间
            if current_time - last_frame_time > frame_timeout:
                logger.warning(f"帧超时 ({current_time - last_frame_time:.1f}秒没有新帧)，重置摄像头")
                reset_camera()
            
            # 输出当前状态
            with clients_lock, stats_lock:
                logger.info(f"服务状态 - 活跃客户端: {active_clients}/{max_clients}, FPS: {fps_stats['current']:.2f}")
            
            last_check = current_time
        
        # 短暂休眠以减少CPU使用
        time.sleep(1)

def adjust_performance():
    """针对树莓派优化的性能调整，增加更多的帧率保护措施"""
    global processing_level, previous_processing_level, max_processing_level
    global last_fps_check, reduce_processing, night_vision_enabled
    global night_vision_auto, memory_reset_needed
    
    try:
        with stats_lock:
            current_fps = fps_stats.get("current", 0)
            avg_fps = fps_stats.get("avg", 0)
            min_fps = fps_stats.get("min", current_fps)  # 获取最低帧率
        
        # 记录当前处理级别，用于判断变化
        previous_processing_level = processing_level
        
        # 当前时间，用于计算间隔
        current_time = time.time()
        
        # 帧率极低时的紧急措施（不到8FPS）
        if current_fps < 8:
            # 立即降低到最低处理级别
            processing_level = 0
            # 如果帧率极低并且夜视开启，可能是夜视处理导致卡顿
            if night_vision_enabled:
                logger.warning(f"夜视模式下帧率极低 ({current_fps:.1f} FPS)，强制执行内存清理")
                memory_reset_needed = True  # 标记需要内存清理
            reduce_processing = True
            logger.warning(f"帧率极低 ({current_fps:.1f} FPS, 最低: {min_fps:.1f} FPS)，降低到最低处理级别")
            return
            
        # 如果是夜视模式，使用较低的帧率阈值和更激进的调整
        fps_threshold_high = 18 if night_vision_enabled else 25
        fps_threshold_low = 12 if night_vision_enabled else 15
        
        # 帧率不足时立即降低处理级别
        if current_fps < fps_threshold_low:
            # 检查是否有明显的帧率下降
            fps_drop = avg_fps - current_fps
            if fps_drop > 5 or current_fps < 10:  # 帧率下降超过5或低于10时
                # 急剧降低处理级别
                processing_level = max(0, processing_level - 2)
                reduce_processing = True
                
                # 如果是夜视模式下帧率过低，记录警告
                if night_vision_enabled:
                    logger.warning(f"夜视模式下帧率过低 ({current_fps:.1f} FPS)，切换到最低处理级别")
            else:
                # 正常降低处理级别
                processing_level = max(0, processing_level - 1)
                reduce_processing = True
                
            logger.info(f"性能调整: 降低处理级别到 {processing_level} (帧率: {current_fps:.1f}, 最低: {min_fps:.1f})")
        
        # 帧率充足且未降级时，考虑提高处理级别
        elif current_fps > fps_threshold_high and not reduce_processing:
            # 每30秒才考虑提高处理级别，避免频繁调整
            if current_time - last_fps_check > 30:
                # 仅当帧率比阈值高出一定余量时提高级别
                if current_fps > fps_threshold_high + 5:
                    processing_level = min(max_processing_level, processing_level + 1)
                    logger.info(f"性能调整: 提高处理级别到 {processing_level} (帧率: {current_fps:.1f}, 最低: {min_fps:.1f})")
                
                # 更新最后检查时间
                last_fps_check = current_time
                
                # 每30秒重置最低帧率统计
                with stats_lock:
                    fps_stats["min"] = current_fps
        
        # 处理级别有变化时记录
        if previous_processing_level != processing_level:
            logger.info(f"处理级别调整: {previous_processing_level} -> {processing_level} (帧率: {current_fps:.1f} FPS)")
    
    except Exception as e:
        logger.error(f"性能调整错误: {e}")

# 增加主线程服务启动
if __name__ == '__main__':
    # 记录启动时间
    service_start_time = time.time()
    ip_address = get_ip_address()
    
    try:
        logger.info(f"摄像头服务器开始启动，IP: {ip_address}")
        
        # 确保全局变量已正确初始化
        # 使用全局变量时不需要再次使用global声明，因为这些变量已经在文件顶部定义为全局变量
        motion_detected = False
        motion_frame_buffer = None
        last_motion_time = time.time()
        reduced_processing_until = 0
        
        # 初始化摄像头
        if not init_camera():
            logger.error("摄像头初始化失败，服务无法启动")
            sys.exit(1)
        
        # 启动帧捕获线程
        capture_thread = threading.Thread(target=capture_continuous)
        capture_thread.daemon = True
        capture_thread.start()
        
        # 启动健康检查线程
        health_thread = threading.Thread(target=health_check)
        health_thread.daemon = True
        health_thread.start()
        
        # 启动Flask应用
        logger.info(f"摄像头服务器开始运行在 http://{ip_address}:8000")
        app.run(host='0.0.0.0', port=8000, threaded=True, use_reloader=False)
        
    except KeyboardInterrupt:
        logger.info("接收到终止信号，关闭服务...")
    except Exception as e:
        logger.error(f"服务器运行出错: {e}")
    finally:
        running = False
        if picam2 is not None:
            try:
                picam2.stop()
            except:
                pass
        logger.info("摄像头服务已关闭")

@app.route('/toggle_night_vision', methods=['POST'])
def toggle_night_vision_endpoint():
    """切换夜视功能开关的API端点"""
    global night_vision_enabled
    
    try:
        with night_vision_lock:
            night_vision_enabled = not night_vision_enabled
            status = '开启' if night_vision_enabled else '关闭'
            logger.info(f"夜视功能已{status}")
        return {"status": "success", "enabled": night_vision_enabled, "message": f"夜视功能已{status}"}
    except Exception as e:
        logger.error(f"切换夜视功能失败: {e}")
        return {"status": "error", "message": f"切换夜视功能失败: {e}"}, 500

@app.route('/toggle_night_vision_mode', methods=['POST'])
def toggle_night_vision_mode_endpoint():
    """切换夜视模式(自动/手动)的API端点"""
    global night_vision_auto
    
    try:
        with night_vision_lock:
            night_vision_auto = not night_vision_auto
            mode = '自动' if night_vision_auto else '手动'
            logger.info(f"夜视模式已切换为{mode}模式")
        return {"status": "success", "auto": night_vision_auto, "message": f"夜视模式已切换为{mode}模式"}
    except Exception as e:
        logger.error(f"切换夜视模式失败: {e}")
        return {"status": "error", "message": f"切换夜视模式失败: {e}"}, 500

@app.route('/set_night_vision_strength', methods=['POST'])
def set_night_vision_strength_endpoint():
    """设置夜视增强强度的API端点"""
    global night_vision_strength
    
    try:
        data = request.get_json()
        if not data or 'strength' not in data:
            return {"status": "error", "message": "缺少强度参数"}, 400
            
        strength = float(data['strength'])
        if strength < 0.1 or strength > 1.0:
            return {"status": "error", "message": "强度必须在0.1到1.0之间"}, 400
            
        with night_vision_lock:
            night_vision_strength = strength
            logger.info(f"夜视增强强度已设置为: {strength}")
            
        return {"status": "success", "strength": strength, "message": f"夜视增强强度已设置为: {strength:.1f}"}
    except Exception as e:
        logger.error(f"设置夜视增强强度失败: {e}")
        return {"status": "error", "message": f"设置夜视增强强度失败: {e}"}, 500

@app.route('/toggle_green_night_vision', methods=['POST'])
def toggle_green_night_vision_endpoint():
    """切换绿色夜视效果的API端点"""
    global enable_green_tint
    
    try:
        with night_vision_lock:
            enable_green_tint = not enable_green_tint
            status = '开启' if enable_green_tint else '关闭'
            logger.info(f"绿色夜视效果已{status}")
        return {"status": "success", "enabled": enable_green_tint, "message": f"绿色夜视效果已{status}"}
    except Exception as e:
        logger.error(f"切换绿色夜视效果失败: {e}")
        return {"status": "error", "message": f"切换绿色夜视效果失败: {e}"}, 500

@app.route('/set_light_threshold', methods=['POST'])
def set_light_threshold_endpoint():
    """设置光线阈值的API端点"""
    try:
        data = request.get_json()
        threshold = float(data.get("threshold", 50))
        
        if threshold < 10 or threshold > 150:
            return {"status": "error", "message": "阈值必须在10到150之间"}, 400
            
        with night_vision_lock:
            light_threshold = threshold
            logger.info(f"光线阈值已设置为: {threshold}")
            
        return {"status": "success", "threshold": threshold, "message": f"光线阈值已设置为: {threshold:.1f}"}
    except Exception as e:
        logger.error(f"设置光线阈值失败: {e}")
        return {"status": "error", "message": f"设置光线阈值失败: {e}"}, 500
