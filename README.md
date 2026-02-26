## Build (C++)
```bash
cmake -S . -B build
cmake --build build -j

./build/database_server

python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python bot.py