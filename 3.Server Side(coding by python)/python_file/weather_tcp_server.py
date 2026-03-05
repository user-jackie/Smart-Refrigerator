#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
和风天气TCP服务器
定时获取天气数据并通过TCP发送给客户端
"""

import socket
import json
import time
import threading
from weather_api import QWeatherAPI


class WeatherTCPServer:
    def __init__(self, host='0.0.0.0', port=8888, update_interval=3600):
        """
        初始化天气TCP服务器
        :param host: 服务器地址
        :param port: 服务器端口
        :param update_interval: 更新间隔（秒），默认3600秒（1小时）
        """
        self.host = host
        self.port = port
        self.update_interval = update_interval
        self.server_socket = None
        self.clients = []
        self.running = False
        self.weather_data = {}
        
        # 初始化和风天气API
        self.api_key = "xxx"    #替换为自己的api_key
        self.api_host = "xxx"   #替换为自己的api_host
        self.weather_api = QWeatherAPI(self.api_key, self.api_host)
        
        # 默认城市
        self.city_name = "北京"
    
    def fetch_weather_data(self):
        """获取天气数据"""
        print(f"正在获取 {self.city_name} 的天气数据...")
        
        # 获取城市信息
        city_info = self.weather_api.get_city_location(self.city_name)
        if not city_info:
            print("获取城市信息失败")
            return None
        
        location_id = city_info.get("id")
        lat = city_info.get('lat')
        lon = city_info.get('lon')
        
        # 获取实时天气
        now_weather = self.weather_api.get_weather_now(location_id)
        
        # 获取3天预报
        forecast_3d = self.weather_api.get_weather_3d(location_id)
        
        # 获取空气质量
        lat_lon = f"{lat},{lon}"
        air_quality = self.weather_api.get_air_quality(lat_lon)
        
        # 获取分钟级降水
        lon_lat = f"{lon},{lat}"
        minutely = self.weather_api.get_minutely_precipitation(lon_lat)

        # 组装数据
        data = {
            "timestamp": time.time(),
            "city": {
                "name": city_info.get('name'),
                "id": location_id,
                "lat": lat,
                "lon": lon
            },
            "current": {},
            "forecast": [],
            "air_quality": {},
            "precipitation": {}
        }
        
        # 实时天气
        if now_weather and now_weather.get("now"):
            now = now_weather["now"]
            data["current"] = {
                "temp": now.get('temp'),
                "feelsLike": now.get('feelsLike'),
                "text": now.get('text'),
                "windDir": now.get('windDir'),
                "windScale": now.get('windScale'),
                "humidity": now.get('humidity'),
                "pressure": now.get('pressure'),
                "vis": now.get('vis')
            }
        
        # 3天预报
        if forecast_3d and forecast_3d.get("daily"):
            for day in forecast_3d["daily"]:
                data["forecast"].append({
                    "date": day.get('fxDate'),
                    "textDay": day.get('textDay'),
                    "textNight": day.get('textNight'),
                    "tempMax": day.get('tempMax'),
                    "tempMin": day.get('tempMin'),
                    "windDirDay": day.get('windDirDay'),
                    "windScaleDay": day.get('windScaleDay'),
                    "humidity": day.get('humidity'),
                    "precip": day.get('precip'),
                    "uvIndex": day.get('uvIndex')
                })
        
        # 空气质量
        if air_quality and 'indexes' in air_quality:
            index = air_quality['indexes'][0]
            primary_pollutant = index.get('primaryPollutant') or {}
            data["air_quality"] = {
                "aqi": index.get('aqi'),
                "category": index.get('category'),
                "primaryPollutant": primary_pollutant.get('name', 'N/A')
            }
            
            # 污染物浓度
            if 'pollutants' in air_quality:
                data["air_quality"]["pollutants"] = {}
                for pollutant in air_quality['pollutants']:
                    code = pollutant.get('code')
                    conc = pollutant.get('concentration') or {}
                    data["air_quality"]["pollutants"][code] = {
                        "name": pollutant.get('name'),
                        "value": conc.get('value'),
                        "unit": conc.get('unit')
                    }
        
        # 分钟级降水
        if minutely:
            data["precipitation"] = {
                "summary": minutely.get('summary', ''),
                "hasRain": False
            }
            if 'minutely' in minutely and minutely['minutely']:
                # 检查是否有降水
                for item in minutely['minutely']:
                    if float(item.get('precip', 0)) > 0:
                        data["precipitation"]["hasRain"] = True
                        break
        
        self.weather_data = data
        print("天气数据获取成功")
        return data
    
    def handle_client(self, client_socket, address):
        """处理客户端连接"""
        print(f"客户端连接: {address}")
        self.clients.append(client_socket)
        
        try:
            while self.running:
                # 接收客户端请求
                try:
                    request = client_socket.recv(1024).decode('utf-8')
                    if not request:
                        break
                    
                    print(f"收到请求: {request}")
                    
                    # 处理请求
                    if request.startswith("GET_WEATHER"):
                        # 立即刷新天气数据
                        self.fetch_weather_data()
                        # 发送天气数据
                        if self.weather_data:
                            json_data = json.dumps(self.weather_data, ensure_ascii=False)
                            client_socket.send(json_data.encode('utf-8'))
                        else:
                            error_msg = json.dumps({"error": "暂无天气数据"}, ensure_ascii=False)
                            client_socket.send(error_msg.encode('utf-8'))
                    
                    elif request.startswith("SET_INTERVAL:"):
                        # 设置更新间隔
                        try:
                            interval = int(request.split(":")[1])
                            self.update_interval = interval
                            response = json.dumps({"status": "ok", "interval": interval}, ensure_ascii=False)
                            client_socket.send(response.encode('utf-8'))
                            print(f"更新间隔已设置为: {interval}秒")
                        except:
                            error_msg = json.dumps({"error": "无效的间隔时间"}, ensure_ascii=False)
                            client_socket.send(error_msg.encode('utf-8'))
                    
                    elif request.startswith("SET_CITY:"):
                        # 设置城市
                        city = request.split(":", 1)[1]
                        self.city_name = city
                        # 立即获取新城市的天气
                        self.fetch_weather_data()
                        response = json.dumps({"status": "ok", "city": city}, ensure_ascii=False)
                        client_socket.send(response.encode('utf-8'))
                        print(f"城市已设置为: {city}")
                    
                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"处理请求错误: {e}")
                    break
        
        except Exception as e:
            print(f"客户端处理错误: {e}")
        finally:
            if client_socket in self.clients:
                self.clients.remove(client_socket)
            client_socket.close()
            print(f"客户端断开: {address}")
    
    def update_weather_loop(self):
        """定时更新天气数据"""
        while self.running:
            self.fetch_weather_data()
            time.sleep(self.update_interval)
    
    def start(self):
        """启动服务器"""
        self.running = True
        
        # 创建服务器socket
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)
        
        print(f"天气TCP服务器启动: {self.host}:{self.port}")
        print(f"更新间隔: {self.update_interval}秒")
        
        # 立即获取一次天气数据
        self.fetch_weather_data()
        
        # 启动定时更新线程
        update_thread = threading.Thread(target=self.update_weather_loop, daemon=True)
        update_thread.start()
        
        # 接受客户端连接
        try:
            while self.running:
                try:
                    self.server_socket.settimeout(1.0)
                    client_socket, address = self.server_socket.accept()
                    client_socket.settimeout(5.0)
                    
                    # 为每个客户端创建线程
                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(client_socket, address),
                        daemon=True
                    )
                    client_thread.start()
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        print(f"接受连接错误: {e}")
        except KeyboardInterrupt:
            print("\n服务器关闭中...")
        finally:
            self.stop()
    
    def stop(self):
        """停止服务器"""
        self.running = False
        
        # 关闭所有客户端连接
        for client in self.clients:
            try:
                client.close()
            except:
                pass
        
        # 关闭服务器socket
        if self.server_socket:
            self.server_socket.close()
        
        print("服务器已停止")


if __name__ == "__main__":
    # 创建并启动服务器
    # 默认每小时更新一次（3600秒）
    server = WeatherTCPServer(host='0.0.0.0', port=8888, update_interval=3600)
    server.start()
