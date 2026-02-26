# cloud-telegram

Проект состоит из C++ сервера и Telegram-бота: сервер принимает запросы, 
хранит данные на диске и поддерживает базовые операции с альбомами/файлами (создание/добавление/получение/список), 
а бот на Python выступает клиентом - принимает команды в Telegram, отправляет их на сервер и возвращает пользователю результат.

- `src/` - код C++ сервера
- `bot.py` - Telegram-бот
- `test_client.py` - простой клиент, чтобы тестировать без telegram

## Как запускать

### 1) собрать сервер (C++)
```bash
cmake -S . -B build
cmake --build build -j
```

### 2) запустить сервер
```bash
./build/database_server
```

### 3) подготовить окружение для бота (Python)
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 4) указать токен Telegram-бота
- создать файл `.env` в корне проекта
- добавить туда:
```text
BOT_TOKEN=твой_токен_из_BotFather
```

### 5) запустить бота (сервер должен быть уже запущен)
```bash
python bot.py
```

### 6) если надо проверить без telegram
```bash
python test_client.py
```
