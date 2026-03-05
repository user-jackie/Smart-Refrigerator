#!/bin/bash
# 启动和风天气TCP服务器

echo "正在启动和风天气TCP服务器..."
echo "服务器地址: 0.0.0.0:8888"
echo "更新间隔: 3600秒（1小时）"
echo "按 Ctrl+C 停止服务器"
echo ""

python3 weather_tcp_server.py
python3 voice_assistant.py
