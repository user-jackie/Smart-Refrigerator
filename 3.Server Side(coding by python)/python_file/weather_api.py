#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
和风天气API接口封装
API文档: https://dev.qweather.com/docs/api/
"""

import requests
import json
from typing import Dict, Optional


class QWeatherAPI:
    """和风天气API类"""
    
    def __init__(self, api_key: str, api_host: str = None, token: str = None):
        """
        初始化和风天气API
        :param api_key: 和风天气API密钥
        :param api_host: API Host (例如: kh6e4ekc2h.re.qweatherapi.com)
        :param token: JWT token (用于需要Bearer认证的接口)
        """
        self.api_key = api_key
        self.token = token
        # 如果提供了API Host，使用个人host；否则使用共享域名
        if api_host:
            self.api_host = api_host
            self.weather_base_url = f"https://{api_host}/v7"
            self.geo_base_url = f"https://{api_host}/geo/v2"
            self.airquality_base_url = f"https://{api_host}/airquality/v1"
        else:
            # 免费订阅使用devapi，商业版使用api
            self.api_host = None
            self.weather_base_url = "https://devapi.qweather.com/v7"
            self.geo_base_url = "https://geoapi.qweather.com/v2"
            self.airquality_base_url = "https://devapi.qweather.com/airquality/v1"
    
    def get_city_location(self, city_name: str) -> Optional[Dict]:
        """
        获取城市位置信息
        :param city_name: 城市名称
        :return: 城市位置信息
        """
        url = f"{self.geo_base_url}/city/lookup"
        params = {
            "location": city_name,
            "key": self.api_key
        }
        
        try:
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            if data.get("code") == "200" and data.get("location"):
                return data["location"][0]
            else:
                print(f"获取城市信息失败: {data.get('code')}")
                return None
        except Exception as e:
            print(f"请求失败: {e}")
            return None
    
    def get_weather_now(self, location: str) -> Optional[Dict]:
        """
        获取实时天气
        :param location: 城市ID或经纬度(如: "101010100" 或 "116.41,39.92")
        :return: 实时天气数据
        """
        url = f"{self.weather_base_url}/weather/now"
        params = {
            "location": location,
            "key": self.api_key
        }
        
        try:
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            if data.get("code") == "200":
                return data
            else:
                print(f"获取实时天气失败: {data.get('code')}")
                return None
        except Exception as e:
            print(f"请求失败: {e}")
            return None

    def get_weather_3d(self, location: str) -> Optional[Dict]:
        """
        获取3天天气预报
        :param location: 城市ID或经纬度
        :return: 3天天气预报数据
        """
        url = f"{self.weather_base_url}/weather/3d"
        params = {
            "location": location,
            "key": self.api_key
        }
        
        try:
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            if data.get("code") == "200":
                return data
            else:
                print(f"获取3天预报失败: {data.get('code')}")
                return None
        except Exception as e:
            print(f"请求失败: {e}")
            return None
    
    def get_weather_7d(self, location: str) -> Optional[Dict]:
        """
        获取7天天气预报
        :param location: 城市ID或经纬度
        :return: 7天天气预报数据
        """
        url = f"{self.weather_base_url}/weather/7d"
        params = {
            "location": location,
            "key": self.api_key
        }
        
        try:
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            if data.get("code") == "200":
                return data
            else:
                print(f"获取7天预报失败: {data.get('code')}")
                return None
        except Exception as e:
            print(f"请求失败: {e}")
            return None
    
    def get_air_quality(self, location: str) -> Optional[Dict]:
        """
        获取空气质量
        :param location: 城市ID或经纬度(格式: "纬度,经度" 如 "39.90,116.40")
        :return: 空气质量数据
        """
        # 解析经纬度
        if ',' in location:
            lat, lon = location.split(',')
            lat = lat.strip()
            lon = lon.strip()
        else:
            # 如果是城市ID，提示需要经纬度
            print("空气质量API需要经纬度坐标，格式: 纬度,经度")
            return None
        
        # 使用新的airquality API端点
        url = f"{self.airquality_base_url}/current/{lat}/{lon}"
        params = {
            "key": self.api_key
        }
        
        try:
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            return data
        except requests.exceptions.HTTPError as e:
            if e.response.status_code == 403:
                print(f"空气质量接口无权限访问，可能需要升级订阅计划")
            else:
                print(f"请求失败: {e}")
            return None
        except Exception as e:
            print(f"请求失败: {e}")
            return None
    
    def get_minutely_precipitation(self, location: str) -> Optional[Dict]:
        """
        获取分钟级降水预报（未来2小时）
        :param location: 经纬度(格式: "经度,纬度" 如 "116.38,39.91")
        :return: 分钟级降水数据
        """
        url = f"{self.weather_base_url}/minutely/5m"
        params = {
            "location": location,
            "key": self.api_key
        }
        
        try:
            response = requests.get(url, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
            
            if data.get("code") == "200":
                return data
            else:
                print(f"获取分钟级降水失败: {data.get('code')}")
                return None
        except Exception as e:
            print(f"请求失败: {e}")
            return None


def main():
    """示例使用"""
    # 初始化API
    api_key = "6fe555ac86ac494dbe88532d1ec6f103"
    api_host = "kh6e4ekc2h.re.qweatherapi.com"
    # 如果有JWT token，可以在这里设置
    token = None  # 例如: "your_jwt_token_here"
    
    weather = QWeatherAPI(api_key, api_host, token)
    
    # 获取城市信息
    city_name = "北京"
    print(f"正在查询 {city_name} 的天气信息...")
    
    city_info = weather.get_city_location(city_name)
    if city_info:
        location_id = city_info.get("id")
        print(f"\n城市: {city_info.get('name')}")
        print(f"城市ID: {location_id}")
        print(f"经纬度: {city_info.get('lon')}, {city_info.get('lat')}")
        
        # 获取实时天气
        print("\n=== 实时天气 ===")
        now_weather = weather.get_weather_now(location_id)
        if now_weather and now_weather.get("now"):
            now = now_weather["now"]
            print(f"温度: {now.get('temp')}°C")
            print(f"体感温度: {now.get('feelsLike')}°C")
            print(f"天气: {now.get('text')}")
            print(f"风向: {now.get('windDir')}")
            print(f"风力: {now.get('windScale')}级")
            print(f"湿度: {now.get('humidity')}%")
        
        # 获取3天预报
        print("\n=== 3天天气预报 ===")
        forecast_3d = weather.get_weather_3d(location_id)
        if forecast_3d and forecast_3d.get("daily"):
            for day in forecast_3d["daily"]:
                print(f"{day.get('fxDate')}: {day.get('textDay')} "
                      f"{day.get('tempMin')}~{day.get('tempMax')}°C")
        
        # 获取空气质量
        print("\n=== 空气质量 ===")
        # 使用经纬度查询空气质量
        lat_lon = f"{city_info.get('lat')},{city_info.get('lon')}"
        air = weather.get_air_quality(lat_lon)
        if air and 'indexes' in air:
            # 新版API数据结构
            index = air['indexes'][0]  # 获取第一个指数（中国标准）
            print(f"AQI: {index.get('aqi')} ({index.get('name')})")
            print(f"等级: {index.get('category')}")
            print(f"主要污染物: {index.get('primaryPollutant', {}).get('name', 'N/A')}")
            
            # 显示各污染物浓度
            if 'pollutants' in air:
                print("\n污染物浓度:")
                for pollutant in air['pollutants']:
                    conc = pollutant.get('concentration', {})
                    print(f"  {pollutant.get('name')}: {conc.get('value')} {conc.get('unit')}")
            
            # 健康建议
            if 'health' in index:
                health = index['health']
                print(f"\n健康影响: {health.get('effect')}")
                if 'advice' in health:
                    advice = health['advice']
                    print(f"一般人群: {advice.get('generalPopulation')}")
                    print(f"敏感人群: {advice.get('sensitivePopulation')}")
        elif air:
            # 如果数据结构不同，打印原始数据
            print(f"空气质量数据: {air}")
        
        # 获取分钟级降水
        print("\n=== 分钟级降水预报 ===")
        # 注意：分钟级降水API使用 经度,纬度 的顺序
        lon_lat = f"{city_info.get('lon')},{city_info.get('lat')}"
        minutely = weather.get_minutely_precipitation(lon_lat)
        if minutely:
            summary = minutely.get('summary', '')
            print(f"降水概况: {summary}")
            
            if 'minutely' in minutely and minutely['minutely']:
                print("\n未来2小时降水预报（每5分钟）:")
                # 只显示前12个数据点（1小时）
                for i, item in enumerate(minutely['minutely'][:12]):
                    time = item.get('fxTime', '')
                    precip = item.get('precip', 0)
                    print(f"  {time}: {precip}mm")
            else:
                print("未来2小时无降水")


if __name__ == "__main__":
    main()
