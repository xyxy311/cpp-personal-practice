/* 以气象站为例 */
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

class Observer;

// 主题抽象基类
class Subject {
public:
    virtual ~Subject() = default;
    virtual void attach(std::shared_ptr<Observer> obs) = 0;  // 添加观察者
    virtual void detach(std::shared_ptr<Observer> obs) = 0;
    virtual void notify() = 0;  // 通知所有观察者
};

// 观察者抽象基类
class Observer {
public:
    virtual ~Observer() = default;
    virtual void update(float temperature, float humidity) = 0;
};

// 气象站
class WeatherStation : public Subject {
public:
    void attach(std::shared_ptr<Observer> obs) override {
        m_observers.push_back(obs);
    }
    void detach(std::shared_ptr<Observer> obs) override {
        m_observers.erase(
            std::remove(m_observers.begin(), m_observers.end(), obs), m_observers.end()
        );  // 这是删除vector中元素的惯用法
    }
    void notify() override{
        for (const auto& obs : m_observers) {
            obs->update(m_temperature, m_humidity);
        }
    }

    // 给气象站更新数据
    void update(float temperature, float humidity) {
        m_temperature = temperature;
        m_humidity = humidity;
        notify();
    };

private:
    std::vector<std::shared_ptr<Observer>> m_observers;
    float m_temperature = 26.0f;
    float m_humidity = 50.0f;
};

// 显示气象站数据
class Screen : public Observer {
public:
    void update(float temperature, float humidity) override {
        m_temperature = temperature;
        m_humidity = humidity;
        display();
    }
    void display() {
        std::cout << "====" << ++count << "====" << std::endl;
        std::cout << "【屏幕】当前气温：" << m_temperature << std::endl;
        std::cout << "【屏幕】当前湿度：" << m_humidity << std::endl;
    }

private:
    float m_temperature = 26.0f;
    float m_humidity = 50.0f;
    int count = 0;
};

// 统计气象站数据
class StatisticsDisplay : public Observer {
private:
    float maxTemp = -100.0f;
    float minTemp = 100.0f;
    float sumTemp = 0.0f;
    int count = 0;

public:
    void update(float temperature, float /*humidity*/) override {
        // 忽略湿度，只统计温度
        maxTemp = std::max(maxTemp, temperature);
        minTemp = std::min(minTemp, temperature);
        sumTemp += temperature;
        ++count;
        float avgTemp = sumTemp / count;
        
        std::cout << "【统计】 均温 / 高温 / 低温: " 
                  << avgTemp << " / " << maxTemp << " / " << minTemp << "°C" << std::endl;
    }
};

// 湿度警告
class HumidityAlert : public Observer {
private:
    float warningThreshold = 80.0f;
public:
    void update(float temperature, float humidity) override {
        if (humidity > warningThreshold) {
            std::cout << "【湿度警告】 好水啊 " 
                      << humidity << "% !" << std::endl;
        }
    }
};

// 测试
int main() {
    // 创建主题（天气站）
    auto weatherStation = std::make_shared<WeatherStation>();

    // 创建观察者（显示设备）
    auto screen = std::make_shared<Screen>();
    auto statisticsDisplay = std::make_shared<StatisticsDisplay>();
    auto humidityAlert = std::make_shared<HumidityAlert>();

    // 注册观察者
    weatherStation->attach(screen);
    weatherStation->attach(statisticsDisplay);
    weatherStation->attach(humidityAlert);

    // 模拟数据变化
    weatherStation->update(25.0f, 65.0f);
    weatherStation->update(27.5f, 82.0f);   // 湿度超过80，警告触发
    weatherStation->update(22.0f, 70.0f);

    // 移除一个观察者（比如湿度警告不再需要）
    weatherStation->detach(humidityAlert);
    std::cout << "\n--- 移除湿度警告 ---\n" << std::endl;
    weatherStation->update(30.0f, 90.0f);   // 不会再有警告

    return 0;
}