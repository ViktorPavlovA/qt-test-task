# Релизовать клиент серверное приложение на Qt 6.5.2


## Описание

- **Сервер** — GUI-приложение (QMainWindow + QWidgets), управляет подключениями устройств, отображает клиентов и принимаемые данные, позволяет конфигурировать пороги и отправлять команды Start/Stop.
- **Клиент** — консольное приложение, эмулирующее сетевое устройство. Подключается к серверу, ждёт подтверждения и команды Start, затем периодически отправляет JSON-пакеты трёх типов: NetworkMetrics, DeviceStatus, Log.

## Структура проекта

```bash
.
├── client
│   ├── CMakeLists.txt
│   ├── DeviceClient.cpp
│   ├── DeviceClient.h
│   └── main.cpp
├── server
│   ├── mainwindow
│   │   ├── modules
│   │   │   └── NetworkServer
│   │   │       ├── modules
│   │   │       │   └── ClientSession
│   │   │       │       ├── ClientSession.cpp
│   │   │       │       └── ClientSession.h
│   │   │       ├── NetworkServer.cpp
│   │   │       └── NetworkServer.h
│   │   ├── ServerWindow.cpp
│   │   └── ServerWindow.h
│   ├── CMakeLists.txt
│   └── main.cpp
├── CMakeLists.txt
└── README.md

```

## Требования

- Qt 6.5.2 (модули представленные в CmakeLists.txt)
- CMake ≥ 3.16
- Компилятор с поддержкой C++17 (MSVC / GCC / Clang)
- Ubuntu 22.04 LTS (64-bit) - рекомендуемая ОС для сборки 

## Сборка

### Команда для запуска
```bash

mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/<ваш путь до Qt>/Qt/6.5.2/gcc_64
cmake --build . --config Release

```

Исполняемые файлы появятся в `build/server/TelecomServer` и `build/client/TelecomClient`.

## Запуск

1. Запустите **NPOKalibriServer**. В GUI нажмите «Запустить сервер» (порт 12345).
2. Запустите один или несколько экземпляров **NPOKalibriClient**.
3. Клиенты автоматически пытаются подключиться. После получения `ConnectionAck` ждут команду Start.
4. В GUI сервера нажмите «Старт клиентов» — клиенты начнут слать данные.
5. Можно менять критические пороги в настройках и отправлять конфигурацию клиентам.

## Основные возможности

### Сервер
- QTcpServer в отдельном QThread (NetworkServer)
- Поддержка множества одновременных клиентов
- Таблица клиентов: Client ID, IP, Port, Status
- Таблица данных: Client ID, Type, Content (распарсенный), Timestamp
- Лог событий (QTextEdit)
- Кнопки: Запуск/остановка сервера, Start/Stop клиентов, Отправка конфигурации
- Настройки критических значений (latency, packet_loss, cpu_usage и т.д.)
- Отправка JSON-команд клиентам (Start, Stop, Config)

### Клиент
- Автоматическое переподключение каждые 5 секунд
- Обработка ConnectionAck, Start, Stop, Config
- Генерация случайных данных трёх типов с помощью QRandomGenerator
- Случайная задержка отправки 10–100 мс
- Различная длина сообщений (короткие / средние / длинные логи)
- При превышении критических порогов (полученных от сервера) генерирует предупреждающие Log-пакеты

## Протокол (JSON)

### Сервер → Клиент
```json

{"type": "ConnectionAck", "client_id": "client-1", "status": "ok"}
{"type": "Start"}
{"type": "Stop"}
{"type": "Config", "critical_latency": 50.0, "critical_packet_loss": 0.05, "critical_cpu": 80}

```

### Клиент → Сервер
```json

{"type": "NetworkMetrics", "bandwidth": 125.4, "latency": 18.2, "packet_loss": 0.012}
{"type": "DeviceStatus", "uptime": 3600, "cpu_usage": 42, "memory_usage": 67}
{"type": "Log", "message": "Interface eth0 restarted after timeout", "severity": "WARNING"}

```

