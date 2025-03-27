static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!doctype html>
<html>

<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>ESP32 CAM Robot</title>
    <style>
    body {
        font-family: Arial, Helvetica, sans-serif;
        background: #151515;
        color: #efefef;
        font-size: 16px;
        margin: 0;
        padding: 0;
    }

    h2 {
        font-size: 18px;
        margin: 10px 0;
    }

    .page-container {
        display: flex;
        flex-direction: column;
        align-items: center;
        padding: 20px;
        max-width: 1200px;
        margin: 0 auto;
    }

    .header {
        width: 100%;
        text-align: center;
        margin-bottom: 20px;
    }

    .content-container {
        display: flex;
        flex-direction: column;
        align-items: center;
        width: 100%;
    }

    .video-container {
        width: 100%;
        max-width: 640px;
        margin-bottom: 20px;
        position: relative;
        border-radius: 8px;
        overflow: hidden;
        box-shadow: 0 4px 12px rgba(0,0,0,0.5);
    }

    .video-container img {
        width: 100%;
        height: auto;
        display: block;
        border-radius: 8px;
    }

    .controls-container {
        width: 100%;
        max-width: 640px;
    }

    .button-row {
        display: flex;
        justify-content: center;
        margin-bottom: 10px;
    }

    .button-grid {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        gap: 10px;
        margin-bottom: 10px;
    }

    button {
        display: block;
        padding: 10px 15px;
        border: 0;
        line-height: 28px;
        cursor: pointer;
        color: #fff;
        background: #4247b7;
        border-radius: 8px;
        font-size: 16px;
        outline: 0;
        width: 100%;
        transition: background 0.3s;
        user-select: none;
    }

    .button2 {
        background-color: #1cb8bd;
    }

    .button4 {
        background-color: #e7e7e7;
        color: #000;
    }

    button:hover {
        background: #ff494d;
        transform: translateY(-2px);
        box-shadow: 0 2px 5px rgba(0,0,0,0.2);
    }

    button:active {
        background: #f21c21;
        transform: translateY(0);
    }

    .close {
        position: absolute;
        right: 10px;
        top: 10px;
        background: #ff3034;
        width: 25px;
        height: 25px;
        border-radius: 50%;
        color: #fff;
        text-align: center;
        line-height: 25px;
        cursor: pointer;
        font-size: 16px;
        z-index: 10;
        box-shadow: 0 2px 5px rgba(0,0,0,0.3);
    }

    .hidden {
        display: none;
    }

    .rotate0 {
        transform: rotate(0deg);
    }

    .connection-status {
        position: fixed;
        top: 10px;
        right: 10px;
        background-color: rgba(0,0,0,0.7);
        color: white;
        padding: 8px 12px;
        border-radius: 5px;
        font-size: 14px;
        z-index: 1000;
        box-shadow: 0 2px 5px rgba(0,0,0,0.3);
    }
    
    .status-connected { color: #4CAF50; }
    .status-connecting { color: #FFC107; }
    .status-error { color: #F44336; }
    .status-frozen { color: #FF9800; }
    
    .config-section {
        margin-top: 20px;
        border-top: 1px solid #333;
        padding-top: 20px;
    }
    
    .config-section h3 {
        text-align: center;
        margin-bottom: 15px;
        color: #1cb8bd;
    }

    /* 响应式布局 */
    @media (min-width: 768px) {
        .content-container {
            flex-direction: row;
            align-items: flex-start;
            justify-content: center;
            gap: 20px;
        }
        
        .video-container {
            flex: 1;
            margin-bottom: 0;
        }
        
        .controls-container {
            flex: 1;
        }
    }
    </style>
</head>

<body>
    <div class="page-container">
        <div class="header">
            <h1>ESP32 机械狗控制面板</h1>
        </div>
        
        <div class="content-container">
            <div id="pi-stream-container" class="video-container">
                <div class="close" id="close-pi-stream">×</div>
                <img id="pi-stream" src="" class="rotate0">
            </div>
            
            <div class="controls-container">
                <div class="button-row">
                    <button class="button" id="toggle-pi-stream">启动摄像头</button>
                    <button class="button button4" id="reset-camera">重启视频流服务</button>
                </div>
                
                <div class="button-grid">
                    <div></div>
                    <button class="button button2" id="forward" 
                        onmousedown="fetch(document.location.origin+'/control?var=move&val=1&cmd=0');" 
                        ontouchstart="fetch(document.location.origin+'/control?var=move&val=1&cmd=0');" 
                        onmouseup="fetch(document.location.origin+'/control?var=move&val=3&cmd=0');" 
                        ontouchend="fetch(document.location.origin+'/control?var=move&val=3&cmd=0');">FORWARD</button>
                    <div></div>
                    
                    <button class="button button2" id="turnleft" 
                        onmousedown="fetch(document.location.origin+'/control?var=move&val=2&cmd=0');" 
                        ontouchstart="fetch(document.location.origin+'/control?var=move&val=2&cmd=0');" 
                        onmouseup="fetch(document.location.origin+'/control?var=move&val=6&cmd=0');" 
                        ontouchend="fetch(document.location.origin+'/control?var=move&val=6&cmd=0');">LEFT</button>
                    <button class="button button2" id="steady" 
                        onclick="fetch(document.location.origin+'/control?var=funcMode&val=1&cmd=0');">STEADY</button>
                    <button class="button button2" id="turnright" 
                        onmousedown="fetch(document.location.origin+'/control?var=move&val=4&cmd=0');" 
                        ontouchstart="fetch(document.location.origin+'/control?var=move&val=4&cmd=0');" 
                        onmouseup="fetch(document.location.origin+'/control?var=move&val=6&cmd=0');" 
                        ontouchend="fetch(document.location.origin+'/control?var=move&val=6&cmd=0');">RIGHT</button>
                    
                    <div></div>
                    <button class="button button2" id="backward" 
                        onmousedown="fetch(document.location.origin+'/control?var=move&val=5&cmd=0');" 
                        ontouchstart="fetch(document.location.origin+'/control?var=move&val=5&cmd=0');" 
                        onmouseup="fetch(document.location.origin+'/control?var=move&val=3&cmd=0');" 
                        ontouchend="fetch(document.location.origin+'/control?var=move&val=3&cmd=0');">REVERSE</button>
                    <div></div>
                </div>
                
                <div class="button-grid">
                    <button class="button button4" id="stayLow" onclick="fetch(document.location.origin+'/control?var=funcMode&val=2&cmd=0');">StayLow</button>
                    <button class="button button4" id="handShake" onclick="fetch(document.location.origin+'/control?var=funcMode&val=3&cmd=0');">HandShake</button>
                    <button class="button button4" id="Jump" onclick="fetch(document.location.origin+'/control?var=funcMode&val=4&cmd=0');">Jump</button>
                </div>
                
                <div class="button-grid">
                    <button class="button button4" id="thanks" onclick="fetch(document.location.origin+'/control?var=funcMode&val=10&cmd=0');">Thanks</button>
                    <button class="button button4" id="actionB" onclick="fetch(document.location.origin+'/control?var=funcMode&val=6&cmd=0');">ActionB</button>
                    <button class="button button4" id="actionC" onclick="fetch(document.location.origin+'/control?var=funcMode&val=7&cmd=0');">ActionC</button>
                </div>
                
                <div class="button-grid">
                    <button class="button button4" id="initPos" onclick="fetch(document.location.origin+'/control?var=funcMode&val=8&cmd=0');">InitPos</button>
                    <div></div>
                    <button class="button button4" id="middlePos" onclick="fetch(document.location.origin+'/control?var=funcMode&val=9&cmd=0');">MiddlePos</button>
                </div>
                
                <div class="config-section">
                    <h3>舵机配置</h3>
                    <div class="button-grid">
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=0&cmd=-1');">PWM0-</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=0&cmd=1');">PWM0+</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sset&val=0&cmd=1');">0_SET</button>
                    </div>
                    <div class="button-grid">
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=1&cmd=-1');">PWM1-</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=1&cmd=1');">PWM1+</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sset&val=1&cmd=1');">1_SET</button>
                    </div>
                    <div class="button-grid">
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=2&cmd=-1');">PWM2-</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=2&cmd=1');">PWM2+</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sset&val=2&cmd=1');">2_SET</button>
                    </div>
                    <div class="button-grid">
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=3&cmd=-1');">PWM3-</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sconfig&val=3&cmd=1');">PWM3+</button>
                        <button class="button" onclick="fetch(document.location.origin+'/control?var=sset&val=3&cmd=1');">3_SET</button>
                    </div>
                    <!-- 更多舵机配置按钮可以根据需要添加 -->
                </div>
            </div>
        </div>
    </div>

    <script>
    document.addEventListener('DOMContentLoaded', function (event) {
        // 首先确保机械狗处于停止状态
        fetch(document.location.origin+'/control?var=move&val=3&cmd=0')
            .then(() => console.log("初始化机械狗为停止状态"))
            .catch(e => console.error("无法初始化机械狗状态", e));
        
        var baseHost = document.location.origin;
        // 使用树莓派的IP地址加上摄像头服务的端口号
        var piStreamUrl = 'http://192.168.10.172:8000/video_feed';
        var reconnectTimer = null;
        var reconnectAttempts = 0;
        var maxReconnectAttempts = 10; // 增加最大重连次数
        var reconnectInterval = 2000; // 减少重连等待时间到2秒
        var streamHealth = {
            loadCount: 0,
            errorCount: 0,
            lastLoadTime: 0,
            frozen: false,
            lastImageData: null
        };
        var freezeDetectionTimer = null;
        var connectionStatus = document.createElement('div');
        connectionStatus.className = 'connection-status';
        connectionStatus.innerHTML = '连接中...';
        document.querySelector('.content-container').insertAdjacentElement('beforebegin', connectionStatus);

        // 检测画面是否冻结的函数
        function checkIfFrozen() {
            if (!piStreamImg.complete || piStreamImg.naturalWidth === 0) return;
            
            try {
                var canvas = document.createElement('canvas');
                canvas.width = piStreamImg.naturalWidth;
                canvas.height = piStreamImg.naturalHeight;
                var ctx = canvas.getContext('2d');
                ctx.drawImage(piStreamImg, 0, 0);
                
                var currentImageData = ctx.getImageData(0, 0, canvas.width, canvas.height).data;
                
                // 如果有上一帧，比较当前帧和上一帧
                if (streamHealth.lastImageData) {
                    var differences = 0;
                    var samplePoints = 1000; // 采样点数量
                    var threshold = 5; // 像素差异阈值
                    
                    // 随机采样点进行比较
                    for (var i = 0; i < samplePoints; i++) {
                        var pixelIndex = Math.floor(Math.random() * currentImageData.length / 4) * 4;
                        
                        // 比较RGB值
                        if (Math.abs(currentImageData[pixelIndex] - streamHealth.lastImageData[pixelIndex]) > threshold ||
                            Math.abs(currentImageData[pixelIndex+1] - streamHealth.lastImageData[pixelIndex+1]) > threshold ||
                            Math.abs(currentImageData[pixelIndex+2] - streamHealth.lastImageData[pixelIndex+2]) > threshold) {
                            differences++;
                        }
                    }
                    
                    // 如果差异点太少，认为画面冻结
                    if (differences < samplePoints * 0.01) { // 少于1%的点有差异
                        streamHealth.frozen = true;
                        console.log("检测到画面冻结");
                        updateConnectionStatus("画面已冻结，正在尝试恢复...", "status-frozen");
                        attemptReconnect();
                        return;
                    } else {
                        streamHealth.frozen = false;
                    }
                }
                
                // 更新上一帧数据
                streamHealth.lastImageData = currentImageData;
                
            } catch (e) {
                console.error("画面冻结检测失败", e);
            }
        }

        // 更新连接状态显示
        function updateConnectionStatus(message, className) {
            connectionStatus.textContent = message;
            connectionStatus.className = 'connection-status';
            if (className) {
                connectionStatus.classList.add(className);
            }
        }

        // 开始树莓派摄像头流
        function startPiCamStream() {
            updateConnectionStatus("正在连接摄像头...", "status-connecting");
            
            // 确保之前的重连定时器被清除
            if (reconnectTimer) {
                clearTimeout(reconnectTimer);
                reconnectTimer = null;
            }
            
            // 清除之前的冻结检测定时器
            if (freezeDetectionTimer) {
                clearInterval(freezeDetectionTimer);
                freezeDetectionTimer = null;
            }
            
            // 添加随机参数防止缓存
            var piStreamImg = document.getElementById('pi-stream');
            piStreamImg.src = piStreamUrl + "?t=" + new Date().getTime();
            document.getElementById('toggle-pi-stream').innerHTML = '停止摄像头';
            piStreamContainer.style.display = 'block';
            reconnectAttempts = 0;
            streamHealth = {
                loadCount: 0,
                errorCount: 0,
                lastLoadTime: Date.now(),
                frozen: false,
                lastImageData: null
            };
            
            // 添加事件监听器以检测流是否中断
            piStreamImg.onload = function() {
                console.log("摄像头流已成功加载");
                streamHealth.loadCount++;
                streamHealth.lastLoadTime = Date.now();
                
                // 清除任何现有的重连计时器
                if (reconnectTimer) {
                    clearTimeout(reconnectTimer);
                    reconnectTimer = null;
                }
                reconnectAttempts = 0;
                
                updateConnectionStatus("摄像头已连接", "status-connected");
                
                // 启动画面冻结检测
                if (!freezeDetectionTimer) {
                    freezeDetectionTimer = setInterval(checkIfFrozen, 5000);
                }
            };
            
            piStreamImg.onerror = function() {
                console.log("摄像头流加载失败，尝试重连...");
                streamHealth.errorCount++;
                updateConnectionStatus("连接失败，正在重试...", "status-error");
                
                if (piStreamImg.src !== '') {
                    attemptReconnect();
                }
            };
        }

        // 尝试重新连接流
        function attemptReconnect() {
            var piStreamImg = document.getElementById('pi-stream');
            if (reconnectAttempts < maxReconnectAttempts && piStreamImg.src !== '') {
                reconnectAttempts++;
                console.log(`尝试重连 (${reconnectAttempts}/${maxReconnectAttempts})...`);
                updateConnectionStatus(`正在重连 (${reconnectAttempts}/${maxReconnectAttempts})...`, "status-connecting");
                
                if (reconnectTimer) {
                    clearTimeout(reconnectTimer);
                }
                
                reconnectTimer = setTimeout(function() {
                    console.log("正在重新加载流...");
                    // 使用时间戳来防止缓存
                    piStreamImg.src = piStreamUrl + '?t=' + new Date().getTime();
                }, reconnectInterval);
            } else if (reconnectAttempts >= maxReconnectAttempts) {
                console.log("达到最大重连次数，请手动刷新页面");
                updateConnectionStatus("连接失败，请刷新页面", "status-error");
                document.getElementById('toggle-pi-stream').innerHTML = '启动摄像头';
                
                // 添加重连按钮
                var reconnectButton = document.createElement('button');
                reconnectButton.innerText = '重新连接';
                reconnectButton.className = 'button';
                reconnectButton.style.marginLeft = '10px';
                reconnectButton.onclick = function() {
                    reconnectAttempts = 0;
                    startPiCamStream();
                };
                
                if (!document.getElementById('reconnect-button')) {
                    reconnectButton.id = 'reconnect-button';
                    document.getElementById('toggle-pi-stream').parentNode.appendChild(reconnectButton);
                }
            }
        }

        // 树莓派摄像头控制按钮
        var piStreamImg = document.getElementById('pi-stream');
        var piStreamContainer = document.getElementById('pi-stream-container');
        
        document.getElementById('toggle-pi-stream').onclick = function() {
            if(piStreamImg.src === '') {
                startPiCamStream();
            } else {
                stopPiCamStream();
            }
        }
        
        // 停止摄像头流
        function stopPiCamStream() {
            piStreamImg.src = '';
            document.getElementById('toggle-pi-stream').innerHTML = '启动摄像头';
            updateConnectionStatus("摄像头已停止", "");
            
            // 清除重连计时器
            if (reconnectTimer) {
                clearTimeout(reconnectTimer);
                reconnectTimer = null;
            }
            
            // 清除冻结检测定时器
            if (freezeDetectionTimer) {
                clearInterval(freezeDetectionTimer);
                freezeDetectionTimer = null;
            }
            
            // 移除重连按钮
            var reconnectButton = document.getElementById('reconnect-button');
            if (reconnectButton) {
                reconnectButton.parentNode.removeChild(reconnectButton);
            }
        }

        // 关闭按钮事件
        document.getElementById('close-pi-stream').onclick = function() {
            stopPiCamStream();
        }
        
        // 重启视频流服务按钮
        document.getElementById('reset-camera').onclick = function() {
            if (piStreamImg.src !== '') {
                updateConnectionStatus("正在重启视频流服务...", "status-connecting");
                
                // 发送请求到树莓派重启视频流服务
                fetch(piStreamUrl.substring(0, piStreamUrl.lastIndexOf('/')) + '/restart_service', {
                    method: 'POST'
                })
                .then(response => {
                    console.log("视频流服务重启请求已发送");
                    // 等待服务重启完成后再重新连接
                    setTimeout(function() {
                        stopPiCamStream();
                        startPiCamStream();
                    }, 5000); // 增加等待时间，确保服务有足够时间重启
                })
                .catch(error => {
                    console.error("视频流服务重启请求失败", error);
                    updateConnectionStatus("重启失败，正在重新连接", "status-error");
                    stopPiCamStream();
                    startPiCamStream();
                });
            }
        };
        
        // 自动启动摄像头流
        startPiCamStream();
        
        // 定期检查摄像头状态
        setInterval(function() {
            if (piStreamImg.src !== '' && !reconnectTimer) {
                // 检查是否长时间没有加载过新帧（10秒）
                var now = Date.now();
                if (now - streamHealth.lastLoadTime > 10000 && !streamHealth.frozen) {
                    console.log("摄像头长时间未收到新帧，可能已卡住");
                    streamHealth.frozen = true;
                    updateConnectionStatus("视频流可能已卡住，正在恢复...", "status-frozen");
                    attemptReconnect();
                }
            }
        }, 5000);
    });
    </script>
</body>

</html>
)rawliteral";
