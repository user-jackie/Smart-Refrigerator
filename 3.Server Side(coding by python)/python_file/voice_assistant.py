import pyaudio
import dashscope
from dashscope.audio.asr import TranslationRecognizerRealtime, TranslationRecognizerCallback,TranscriptionResult,TranslationResult
from dashscope.audio.tts_v2 import SpeechSynthesizer, ResultCallback,AudioFormat
from dashscope import Generation
from http import HTTPStatus
import threading
import time
import os
import socket
import json
import queue

# 设置 DashScope API Key
dashscope.api_key = "sk-xxx"    #替换为自己的阿里云api

# 音频参数
ASR_FORMAT = "pcm"
ASR_SAMPLE_RATE = 16000
TTS_FORMAT = AudioFormat.PCM_22050HZ_MONO_16BIT
TTS_RATE = 22050

# TCP配置
TCP_HOST = '0.0.0.0'
TCP_PORT = 9999

# 全局状态变量
asr_callback = None
recognizer = None
user_input_ready = threading.Event()
user_input_text = ""
is_active = False  # 对话激活状态
last_reply_time = 0  # 最后回复时间
tcp_clients = []  # TCP客户端列表
tcp_lock = threading.Lock()  # TCP客户端列表锁
status_message_queue = queue.Queue()  # 状态消息队列
tts_lock = threading.Lock()  # TTS语音合成锁，确保同一时间只有一个TTS在运行
waiting_for_status = False  # 标记是否正在等待状态消息（用于区分主动查询和自动通知）

# 回调类 - ASR
class ASRCallback(TranslationRecognizerCallback):
    def __init__(self):
        self.transcription_buffer = ""
        self.timer = None
        self.is_listening = True
        self.mic=None
        self.stream=None

    def on_open(self):
        #global mic, stream
        self.mic = pyaudio.PyAudio()
        self.stream = self.mic.open(
            format=pyaudio.paInt16,
            channels=1,
            rate=ASR_SAMPLE_RATE,
            input=True
        )
        print("ASR: 语音识别已启动，请开始说话...")

    def on_close(self):
        #global mic, stream
        if self.stream:
            self.stream.stop_stream()
            self.stream.close()
            self.stream=None
        if self.mic:
            self.mic.terminate()
            self.mic=None
        print("ASR: 语音识别已关闭。")

    def on_event(self, request_id, transcription_result: TranscriptionResult, translation_result: TranslationResult, usage):
        global user_input_text, user_input_ready

        if transcription_result:
            current_text = transcription_result.text.strip()
            if current_text:
                self.update_buffer(current_text)

    def update_buffer(self, text):
        global user_input_text
        self.transcription_buffer = text
        self.reset_timer()

    def reset_timer(self):
        if self.timer:
            self.timer.cancel()
        self.timer = threading.Timer(1, self.on_timeout)
        self.timer.start()

    def on_timeout(self):
        global user_input_text, user_input_ready
        user_input_text = self.transcription_buffer.strip()
        if user_input_text:
            print("检测到停顿，用户输入完成：", user_input_text)
            self.is_listening = False
            user_input_ready.set()

# 回调类 - TTS
class TTSCallback(ResultCallback):
    def __init__(self):
        self._player = None
        self._stream = None

    def on_open(self):
        self._player = pyaudio.PyAudio()
        self._stream = self._player.open(
            format=pyaudio.paInt16,
            channels=1,
            rate=TTS_RATE,
            output=True,
            frames_per_buffer=4096  # 增加缓冲区大小，减少失真
        )
        print("TTS: 语音合成已启动。")

    def on_close(self):
        if self._stream:
            # 等待音频缓冲区播放完成
            time.sleep(0.5)  # 增加等待时间，确保最后的音频播放完
            self._stream.stop_stream()
            self._stream.close()
            self._stream=None
        if self._player:
            self._player.terminate()
            self._player=None
        print("TTS: 语音合成已关闭。")

    def on_data(self, data: bytes):
        if self._stream:
            try:
                self._stream.write(data)
            except Exception as e:
                print(f"TTS播放错误: {e}")

# 分析用户意图并提取控制指令
def analyze_user_intent(user_input, ai_reply):
    """分析用户意图，提取控制指令"""
    control_commands = []
    
    import re
    
    # 杀菌控制
    if "杀菌" in user_input or "紫外线" in user_input:
        # 提取时间
        time_match = re.search(r'(\d+)\s*分钟', user_input)
        minutes = int(time_match.group(1)) if time_match else None
        
        # 判断是否同时设置时间并开始
        has_set_and_start = ("设置" in user_input or "设定" in user_input or "设为" in user_input) and \
                           ("开始" in user_input or "开启" in user_input or "打开" in user_input or "启动" in user_input) and \
                           minutes is not None
        
        if has_set_and_start:
            # 设置时间并开始
            control_commands.append({
                "action": "sterilization_start",
                "duration": minutes
            })
        elif "设置" in user_input or "设定" in user_input or "修改" in user_input or "改为" in user_input or "调整" in user_input:
            # 只设置时间，不启动
            if minutes:
                control_commands.append({
                    "action": "sterilization_set_time",
                    "duration": minutes
                })
        elif "开始" in user_input or "开启" in user_input or "打开" in user_input or "启动" in user_input:
            # 启动杀菌
            if minutes:
                control_commands.append({
                    "action": "sterilization_start",
                    "duration": minutes
                })
            else:
                # 没有指定时间，使用默认时间或当前设置的时间
                control_commands.append({
                    "action": "sterilization_start",
                    "duration": 30  # 默认30分钟
                })
        elif "停止" in user_input or "关闭" in user_input or "禁止" in user_input or "结束" in user_input:
            control_commands.append({"action": "sterilization_stop"})
        elif "剩" in user_input or "还有" in user_input or "多少" in user_input:
            control_commands.append({"action": "sterilization_query"})
    
    # 空气净化控制
    if "净化" in user_input or "空气" in user_input:
        # 提取时间
        time_match = re.search(r'(\d+)\s*分钟', user_input)
        minutes = int(time_match.group(1)) if time_match else None
        
        # 判断是否同时设置时间并开始
        has_set_and_start = ("设置" in user_input or "设定" in user_input or "设为" in user_input) and \
                           ("开始" in user_input or "开启" in user_input or "打开" in user_input or "启动" in user_input) and \
                           minutes is not None
        
        if has_set_and_start:
            # 设置时间并开始
            control_commands.append({
                "action": "purification_start",
                "duration": minutes
            })
        elif "设置" in user_input or "设定" in user_input or "修改" in user_input or "改为" in user_input or "调整" in user_input:
            # 只设置时间，不启动
            if minutes:
                control_commands.append({
                    "action": "purification_set_time",
                    "duration": minutes
                })
        elif "开始" in user_input or "开启" in user_input or "打开" in user_input or "启动" in user_input:
            # 启动净化
            if minutes:
                control_commands.append({
                    "action": "purification_start",
                    "duration": minutes
                })
            else:
                # 没有指定时间，使用默认时间或当前设置的时间
                control_commands.append({
                    "action": "purification_start",
                    "duration": 60  # 默认60分钟
                })
        elif "停止" in user_input or "关闭" in user_input or "禁止" in user_input or "结束" in user_input:
            control_commands.append({"action": "purification_stop"})
        elif "剩" in user_input or "还有" in user_input or "多少" in user_input:
            control_commands.append({"action": "purification_query"})
    
    # 制冷控制（兼容"自冷"等误识别）
    if any(word in user_input for word in ["制冷", "自冷", "降温", "冷却"]):
        # 提取温度
        temp_match = re.search(r'(\d+)\s*度', user_input)
        temperature = int(temp_match.group(1)) if temp_match else None
        
        if "设置" in user_input or "设定" in user_input or "调" in user_input:
            # 只设置温度，不启动
            if temperature:
                control_commands.append({
                    "action": "cooling_set_temp",
                    "temperature": temperature
                })
        elif "开始" in user_input or "开启" in user_input or "打开" in user_input or "启动" in user_input:
            # 启动制冷
            if temperature:
                control_commands.append({
                    "action": "cooling_start",
                    "temperature": temperature
                })
            else:
                control_commands.append({"action": "cooling_start"})
        elif "停止" in user_input or "关闭" in user_input or "禁止" in user_input or "结束" in user_input:
            control_commands.append({"action": "cooling_stop"})
        elif "温度" in user_input and ("多少" in user_input or "几" in user_input):
            control_commands.append({"action": "temperature_query"})
    
    # 制热控制（兼容"自热"等误识别）
    if any(word in user_input for word in ["制热", "自热", "加热", "升温", "保温"]):
        # 提取温度
        temp_match = re.search(r'(\d+)\s*度', user_input)
        temperature = int(temp_match.group(1)) if temp_match else None
        
        if "设置" in user_input or "设定" in user_input or "调" in user_input:
            # 只设置温度，不启动
            if temperature:
                control_commands.append({
                    "action": "heating_set_temp",
                    "temperature": temperature
                })
        elif "开始" in user_input or "开启" in user_input or "打开" in user_input or "启动" in user_input:
            # 启动制热
            if temperature:
                control_commands.append({
                    "action": "heating_start",
                    "temperature": temperature
                })
            else:
                control_commands.append({"action": "heating_start"})
        elif "停止" in user_input or "关闭" in user_input or "禁止" in user_input or "结束" in user_input:
            control_commands.append({"action": "heating_stop"})
        elif "温度" in user_input and ("多少" in user_input or "几" in user_input):
            control_commands.append({"action": "temperature_query"})
    
    # 查询温度（通用）
    if "温度" in user_input and ("多少" in user_input or "几" in user_input or "现在" in user_input):
        if "temperature_query" not in [cmd.get("action") for cmd in control_commands]:
            control_commands.append({"action": "temperature_query"})
    
    return control_commands

messages=[]

# 发送消息到TCP客户端
def send_to_clients(message_type, content, extra_data=None):
    """发送消息到所有连接的TCP客户端"""
    data = {
        "type": message_type,  # "user" 或 "assistant" 或 "control"
        "content": content,
        "timestamp": time.time()
    }
    
    # 如果有额外数据（如控制指令），添加到数据中
    if extra_data:
        data.update(extra_data)
    
    json_data = json.dumps(data, ensure_ascii=False) + "\n"
    
    print(f"[TCP发送] 类型: {message_type}, 内容长度: {len(content)}")
    
    with tcp_lock:
        disconnected = []
        for client in tcp_clients:
            try:
                client.send(json_data.encode('utf-8'))
                print(f"[TCP发送成功] 已发送到客户端")
            except Exception as e:
                print(f"[TCP发送失败] {e}")
                disconnected.append(client)
        
        # 移除断开的客户端
        for client in disconnected:
            tcp_clients.remove(client)
            try:
                client.close()
            except:
                pass

# 处理状态消息的函数
def process_status_message(status_msg):
    """处理从Qt接收的状态消息"""
    global last_reply_time
    
    print(f"[处理状态消息] {status_msg}")
    
    # 发送到TCP客户端显示
    send_to_clients("assistant", status_msg)
    
    # 使用TTS锁确保同一时间只有一个语音合成
    with tts_lock:
        # 进行语音合成
        print("开始状态语音合成...")
        tts_callback = TTSCallback()
        
        try:
            synthesizer = SpeechSynthesizer(
                model="cosyvoice-v1",
                voice="longxiaochun",
                format=TTS_FORMAT,
                callback=tts_callback
            )
            
            synthesizer.streaming_call(status_msg)
            synthesizer.streaming_complete()
            
            # 等待音频播放完成
            time.sleep(0.5)
            print("状态语音合成完成")
        except Exception as e:
            print(f"状态语音合成错误: {e}")
            import traceback
            traceback.print_exc()
    
    # 更新最后回复时间
    last_reply_time = time.time()

# 处理用户输入，调用大模型生成回复
def process_input(user_input):
    global messages, last_reply_time
    print("处理用户输入：", user_input)
    
    # 发送用户消息到TCP客户端
    send_to_clients("user", user_input)
    
    # 检查是否是设备控制指令
    is_device_control = any(keyword in user_input for keyword in 
                           ["杀菌", "紫外线", "净化", "空气", "制冷", "降温", "制热", "加热", "升温"])
    
    # 检查是否是查询指令
    is_query = any(keyword in user_input for keyword in ["剩", "还有", "多少", "几", "现在", "当前"])
    
    # 如果是设备控制，添加系统提示
    if is_device_control and not is_query and (not messages or messages[0].get("role") != "system"):
        messages.insert(0, {
            "role": "system",
            "content": "你是一个智能冰箱助手。当用户要求控制设备（如杀菌、净化、制冷、制热）时，请用简短的一句话确认操作，例如：'已开始杀菌'、'已将杀菌时间设定为30分钟，并开始杀菌'、'已停止空气净化'、'已开启制冷，目标温度20度'。不要解释原理或提供额外信息。"
        })
    
    messages += [{"role": "user", "content": user_input}]
    
    # 分析用户意图并发送控制指令
    control_commands = analyze_user_intent(user_input, "")
    
    # 如果是查询指令，等待Qt返回状态后再让AI回复
    if control_commands and any(cmd.get("action", "").endswith("_query") for cmd in control_commands):
        global waiting_for_status
        waiting_for_status = True
        
        print(f"检测到查询指令: {control_commands}")
        for cmd in control_commands:
            send_to_clients("control", "控制指令", extra_data=cmd)
        
        # 等待Qt返回状态（最多等待1秒）
        try:
            status_msg = status_message_queue.get(timeout=1.0)
            process_status_message(status_msg)
        except queue.Empty:
            print("等待状态消息超时")
        finally:
            waiting_for_status = False
        
        return  # 不调用大模型，直接返回
    
    # 如果是控制指令（非查询），先发送控制指令，等待Qt返回状态
    if control_commands:
        print(f"检测到控制指令: {control_commands}")
        
        # 检查是否是启动类指令或设置时间指令（可能设备已在运行，需要等待状态反馈）
        is_start_command = any(cmd.get("action", "").endswith("_start") for cmd in control_commands)
        is_set_time_command = any(cmd.get("action", "").endswith("_set_time") for cmd in control_commands)
        
        for cmd in control_commands:
            send_to_clients("control", "控制指令", extra_data=cmd)
        
        # 如果是启动类指令或设置时间指令，等待Qt返回状态（最多等待1秒）
        if is_start_command or is_set_time_command:
            waiting_for_status = True
            try:
                status_msg = status_message_queue.get(timeout=1.0)
                # 如果收到状态消息，直接播放，不调用大模型
                print(f"收到Qt状态反馈: {status_msg}")
                process_status_message(status_msg)
                return
            except queue.Empty:
                # 没有收到状态消息
                if is_set_time_command:
                    # 设置时间指令没有收到反馈，说明设备未运行，让大模型回复
                    print("未收到Qt状态反馈，设备未运行，继续大模型回复")
                else:
                    # 启动指令没有收到反馈，说明设备未运行，继续让大模型回复确认信息
                    print("未收到Qt状态反馈，设备可能未运行，继续大模型回复")
                pass
            finally:
                waiting_for_status = False
    
    responses = Generation.call(
        model="qwen-turbo",
        messages=messages,
        result_format="message",
        stream=True,
        incremental_output=True
    )

    reply = ""
    for response in responses:
        if response.status_code == HTTPStatus.OK:
            content = response.output.choices[0]["message"]["content"]
            reply += content
            print(content,end="",flush=True)
        else:
            print("生成回复失败：", response.message)

    messages += [{"role": "assistant", "content": reply}]
    
    # 大模型生成完成后立即发送到Qt页面
    print("\n大模型回复生成完成，发送到前端...")
    send_to_clients("assistant", reply)
    
    # 使用TTS锁确保同一时间只有一个语音合成
    with tts_lock:
        # 然后进行语音合成
        print("开始语音合成...")
        tts_callback = TTSCallback()
        
        try:
            synthesizer = SpeechSynthesizer(
                model="cosyvoice-v1",
                voice="longxiaochun",
                format=TTS_FORMAT,
                callback=tts_callback
            )
            
            print(f"开始合成文本，长度: {len(reply)}")
            synthesizer.streaming_call(reply)
            synthesizer.streaming_complete()
            
            # 等待音频播放完成
            time.sleep(0.5)
            print("语音合成完成")
        except Exception as e:
            print(f"语音合成错误: {e}")
            import traceback
            traceback.print_exc()
    
    # 更新最后回复时间
    last_reply_time = time.time()
    
    print('回复播放完成，重新进入监听状态。')

# 检查是否需要休眠
def check_sleep_status():
    """检查是否超过30秒无对话，自动休眠"""
    global is_active, last_reply_time
    if is_active and last_reply_time > 0:
        elapsed = time.time() - last_reply_time
        if elapsed > 30:
            is_active = False
            print(f"30秒无对话（已过{elapsed:.1f}秒），进入休眠状态...")
            send_to_clients("system", "AI助手已休眠")
            return True
    return False

# 定时检查休眠状态的线程
def sleep_check_thread():
    """定时检查是否需要休眠"""
    while True:
        time.sleep(5)  # 每5秒检查一次
        check_sleep_status()

# 状态消息处理线程
def status_message_thread():
    """处理状态消息队列 - 只处理自动通知（如杀菌完成）"""
    global waiting_for_status
    while True:
        try:
            # 非阻塞地检查队列，如果有消息就处理
            try:
                # 如果主线程正在等待状态消息，跳过处理
                if waiting_for_status:
                    time.sleep(0.1)
                    continue
                
                status_msg = status_message_queue.get(timeout=0.5)
                # 只处理自动通知（如杀菌完成），不处理"剩余时间"类的查询响应
                if "完成" in status_msg and "剩余时间" not in status_msg:
                    print(f"[后台线程] 收到自动通知: {status_msg}")
                    process_status_message(status_msg)
                else:
                    # 如果是查询响应但没有被主线程处理，放回队列
                    print(f"[后台线程] 忽略消息（可能是查询响应）: {status_msg}")
            except queue.Empty:
                pass
        except Exception as e:
            print(f"状态消息处理错误: {e}")
        time.sleep(0.1)

# TCP服务器处理客户端连接
def handle_tcp_client(client_socket, address):
    """处理TCP客户端连接"""
    print(f"TCP客户端连接: {address}")
    # 禁用Nagle算法，立即发送数据
    client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    # 设置超时
    client_socket.settimeout(0.5)
    
    with tcp_lock:
        tcp_clients.append(client_socket)
    
    try:
        while True:
            # 接收客户端发送的状态信息
            try:
                data = client_socket.recv(4096)
                if data:
                    try:
                        json_str = data.decode('utf-8').strip()
                        if json_str:
                            obj = json.loads(json_str)
                            if obj.get("type") == "status":
                                status_msg = obj.get("content", "")
                                print(f"[收到设备状态] {status_msg}")
                                # 将状态消息放入队列，由主线程处理
                                status_message_queue.put(status_msg)
                    except:
                        pass
            except socket.timeout:
                pass
            except:
                break
            time.sleep(0.1)
    except:
        pass
    finally:
        with tcp_lock:
            if client_socket in tcp_clients:
                tcp_clients.remove(client_socket)
        try:
            client_socket.close()
        except:
            pass
        print(f"TCP客户端断开: {address}")

# TCP服务器线程
def tcp_server_thread():
    """TCP服务器线程"""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((TCP_HOST, TCP_PORT))
    server_socket.listen(5)
    print(f"TCP服务器启动: {TCP_HOST}:{TCP_PORT}")
    
    while True:
        try:
            client_socket, address = server_socket.accept()
            client_thread = threading.Thread(
                target=handle_tcp_client,
                args=(client_socket, address),
                daemon=True
            )
            client_thread.start()
        except Exception as e:
            print(f"TCP服务器错误: {e}")

# 主循环：持续监听并处理语音输入
def run_assistant():
    global is_active, last_reply_time
    
    while True:
        print("等待用户输入...")
        global asr_callback, recognizer
        
        # 重置旧的 ASR 实例
        if asr_callback:
            asr_callback = None
        if recognizer:
            recognizer = None

        asr_callback = ASRCallback()
        recognizer = TranslationRecognizerRealtime(
            model="gummy-realtime-v1",
            format=ASR_FORMAT,
            sample_rate=ASR_SAMPLE_RATE,
            transcription_enabled=True,
            translation_enabled=False,
            callback=asr_callback
        )
        recognizer.start()
        asr_callback.is_listening = True

        while asr_callback.is_listening:
            if asr_callback.stream:
                try:
                    data = asr_callback.stream.read(3200, exception_on_overflow=False)
                    recognizer.send_audio_frame(data)
                except Exception as e:
                    print("录音出错：", e)
                    break
            else:
                break

        recognizer.stop()

        if user_input_ready.is_set():
            user_text = user_input_text.strip()
            
            # 检查是否是激活词
            if "你好" in user_text:
                if not is_active:
                    is_active = True
                    print("对话已激活")
                    send_to_clients("system", "AI助手已激活")
                # 激活后继续处理对话
                process_input(user_text)
            elif is_active:
                # 已激活状态，处理对话
                process_input(user_text)
            else:
                # 未激活状态，忽略非激活词
                print("未激活状态，等待'你好'激活...")
            
            user_input_ready.clear()

# 启动语音助手
if __name__ == "__main__":
    # 启动TCP服务器
    tcp_thread = threading.Thread(target=tcp_server_thread, daemon=True)
    tcp_thread.start()
    
    # 启动休眠检查线程
    sleep_thread = threading.Thread(target=sleep_check_thread, daemon=True)
    sleep_thread.start()
    
    # 启动状态消息处理线程
    status_thread = threading.Thread(target=status_message_thread, daemon=True)
    status_thread.start()
    
    # 启动语音助手
    user_input_ready.clear()
    run_assistant()