from dotenv import load_dotenv
load_dotenv()

import asyncio
import http.client
import io
import json
import os
import urllib.parse
from typing import Dict, Tuple

from telegram import (
    Update,
    InlineKeyboardButton,
    InlineKeyboardMarkup,
    InputFile,
    ReplyKeyboardMarkup,
)
from telegram import InputMediaPhoto, InputMediaVideo
from telegram.ext import (
    ApplicationBuilder,
    CallbackQueryHandler,
    CommandHandler,
    ContextTypes,
    MessageHandler,
    filters,
)

SERVER_HOST = os.getenv("DB_SERVER_HOST", "127.0.0.1")
SERVER_PORT = int(os.getenv("DB_SERVER_PORT", "5555"))
BOT_TOKEN = os.getenv("BOT_TOKEN")

pending_count: Dict[str, int] = {}

pending_group_task: Dict[tuple, asyncio.Task] = {}
pending_group_hint_sent: Dict[tuple, bool] = {}

MAIN_KB = ReplyKeyboardMarkup(
    [["Инструкция", "Альбомы"]],
    resize_keyboard=True,
    is_persistent=True,
)

def http_request(method: str, path: str, body: bytes = b"", headers: dict | None = None):
    conn = http.client.HTTPConnection(SERVER_HOST, SERVER_PORT, timeout=30)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        response = conn.getresponse()
        payload = response.read()
        return response.status, payload
    finally:
        conn.close()


def parse_error(payload: bytes, status: int) -> str:
    try:
        data = json.loads(payload.decode("utf-8"))
    except Exception:
        return f"HTTP {status}"
    return data.get("error", f"HTTP {status}")


def send_addf(uid: str, kind: str, name: str, data: bytes) -> Tuple[bool, str]:
    path = "/users/{}/files?kind={}&name={}".format(
        urllib.parse.quote(uid, safe=""),
        urllib.parse.quote(kind, safe=""),
        urllib.parse.quote(name, safe=""),
    )
    status, payload = http_request("POST", path, body=data, headers={"Content-Length": str(len(data))})
    if status != 201:
        return False, parse_error(payload, status)
    reply = json.loads(payload.decode("utf-8"))
    return True, reply["id"]

def send_commit(uid: str, album: str) -> Tuple[bool, str]:
    path = "/users/{}/albums/{}/commit".format(
        urllib.parse.quote(uid, safe=""),
        urllib.parse.quote(album, safe=""),
    )
    status, payload = http_request("POST", path, headers={"Content-Length": "0"})
    if status != 200:
        return False, parse_error(payload, status)
    reply = json.loads(payload.decode("utf-8"))
    return True, f"moved {reply['moved']}"


def send_listalbums(uid: str) -> Tuple[bool, list]:
    path = "/users/{}/albums".format(urllib.parse.quote(uid, safe=""))
    status, payload = http_request("GET", path)
    if status != 200:
        return False, [parse_error(payload, status)]
    data = json.loads(payload.decode("utf-8"))
    return True, data.get("albums", [])


def send_getalbum(uid: str, album: str):
    album_path = "/users/{}/albums/{}".format(
        urllib.parse.quote(uid, safe=""),
        urllib.parse.quote(album, safe=""),
    )
    status, payload = http_request("GET", album_path)
    if status != 200:
        return False, parse_error(payload, status), []

    meta = json.loads(payload.decode("utf-8"))
    items = []
    for item in meta.get("items", []):
        content_path = "/users/{}/albums/{}/items/{}/content".format(
            urllib.parse.quote(uid, safe=""),
            urllib.parse.quote(album, safe=""),
            urllib.parse.quote(item["id"], safe=""),
        )
        content_status, blob = http_request("GET", content_path)
        if content_status != 200:
            return False, f"failed to download item {item['id']}", []
        items.append((item["kind"], item["name"], blob))

    return True, "", items


async def schedule_group_done_hint(update: Update, uid: str, gid: str, delay_s: float = 1.0):
    key = (uid, gid)

    old = pending_group_task.get(key)
    if old and not old.done():
        old.cancel()

    async def _job():
        try:
            await asyncio.sleep(delay_s)
            if pending_count.get(uid, 0) > 0 and not pending_group_hint_sent.get(key, False):
                pending_group_hint_sent[key] = True
                await update.message.reply_text(
                    "✅ Все файлы загружены.\n"
                    "✍️ Теперь напиши название альбома одним сообщением - я сохраню туда все файлы.",
                    reply_markup=MAIN_KB,
                )
        except asyncio.CancelledError:
            return

    pending_group_task[key] = asyncio.create_task(_job())

def cleanup_user_timers(uid: str):
    for k, t in list(pending_group_task.items()):
        if k[0] == uid:
            if t and not t.done():
                t.cancel()
            pending_group_task.pop(k, None)
            pending_group_hint_sent.pop(k, None)


INSTRUCTION_TEXT = (
    "Привет 👋\n"
    "Это твой личный фото-архив в Telegram.\n"
    "\n"
    "Как пользоваться:\n"
    "1️⃣ Отправь одно или несколько фото / файлов подряд\n"
    "2️⃣ Потом напиши название альбома обычным сообщением\n"
    "3️⃣ Все отправленные файлы сохранятся в этот альбом\n"
    "\n"
    "Что ещё можно:\n"
    "📁 Нажми «Альбомы» - список альбомов и открытие любого\n"
    "📤 Из альбома можно вернуть все файлы в исходном виде\n"
)


async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await update.message.reply_text(INSTRUCTION_TEXT, reply_markup=MAIN_KB)


async def handle_photo(update: Update, context: ContextTypes.DEFAULT_TYPE):
    uid = str(update.effective_user.id)

    tg_file = await update.message.photo[-1].get_file()
    data = await tg_file.download_as_bytearray()

    ok, msg = await asyncio.to_thread(send_addf, uid, "P", "photo.jpg", bytes(data))
    if not ok:
        await update.message.reply_text(f"Ошибка: {msg}", reply_markup=MAIN_KB)
        return

    pending_count[uid] = pending_count.get(uid, 0) + 1

    gid = update.message.media_group_id
    if gid:
        await schedule_group_done_hint(update, uid, gid)
    else:
        await update.message.reply_text(
            "Все файлы загружены. Теперь напиши название альбома одним сообщением - я сохраню туда все файлы.",
            reply_markup=MAIN_KB,
        )


async def handle_document(update: Update, context: ContextTypes.DEFAULT_TYPE):
    uid = str(update.effective_user.id)
    doc = update.message.document

    tg_file = await doc.get_file()
    data = await tg_file.download_as_bytearray()

    name = doc.file_name or "file.bin"
    ok, msg = await asyncio.to_thread(send_addf, uid, "D", name, bytes(data))
    if not ok:
        await update.message.reply_text(f"Ошибка: {msg}", reply_markup=MAIN_KB)
        return

    pending_count[uid] = pending_count.get(uid, 0) + 1

    gid = update.message.media_group_id
    if gid:
        await schedule_group_done_hint(update, uid, gid)
    else:
        await update.message.reply_text(
            "Все файлы загружены. Теперь напиши название альбома одним сообщением - я сохраню туда все файлы.",
            reply_markup=MAIN_KB,
        )


async def handle_video(update: Update, context: ContextTypes.DEFAULT_TYPE):
    uid = str(update.effective_user.id)
    vid = update.message.video

    tg_file = await vid.get_file()
    data = await tg_file.download_as_bytearray()

    name = "video.mp4"
    ok, msg = await asyncio.to_thread(send_addf, uid, "D", name, bytes(data))
    if not ok:
        await update.message.reply_text(f"Ошибка: {msg}", reply_markup=MAIN_KB)
        return

    pending_count[uid] = pending_count.get(uid, 0) + 1

    gid = update.message.media_group_id
    if gid:
        await schedule_group_done_hint(update, uid, gid)
    else:
        await update.message.reply_text(
            "Все файлы загружены. Теперь напиши название альбома одним сообщением - я сохраню туда все файлы.",
            reply_markup=MAIN_KB,
        )


async def handle_text(update: Update, context: ContextTypes.DEFAULT_TYPE):
    uid = str(update.effective_user.id)
    text = (update.message.text or "").strip()
    if not text:
        return

    if text == "Инструкция":
        await update.message.reply_text(INSTRUCTION_TEXT, reply_markup=MAIN_KB)
        return

    if text == "Альбомы":
        await albums(update, context)
        return

    if text.startswith("/"):
        return

    if pending_count.get(uid, 0) <= 0:
        await update.message.reply_text("Сначала пришли фото/файлы, потом напиши название альбома.", reply_markup=MAIN_KB)
        return

    ok, msg = await asyncio.to_thread(send_commit, uid, text)
    if not ok:
        await update.message.reply_text(f"Ошибка: {msg}", reply_markup=MAIN_KB)
        return

    pending_count[uid] = 0
    cleanup_user_timers(uid)

    await update.message.reply_text(f"Сохранил в альбом: {text}", reply_markup=MAIN_KB)


async def albums(update: Update, context: ContextTypes.DEFAULT_TYPE):
    uid = str(update.effective_user.id)
    ok, payload = await asyncio.to_thread(send_listalbums, uid)
    if not ok:
        await update.message.reply_text(f"Ошибка: {payload[0]}", reply_markup=MAIN_KB)
        return

    names = payload
    if not names:
        await update.message.reply_text("Альбомов пока нет.", reply_markup=MAIN_KB)
        return

    rows = []
    for a in names[:60]:
        rows.append([InlineKeyboardButton(a, callback_data=f"GET:{a}")])
    kb = InlineKeyboardMarkup(rows)
    await update.message.reply_text("🗂️ Выбери альбом:", reply_markup=kb)


async def on_callback(update: Update, context: ContextTypes.DEFAULT_TYPE):
    q = update.callback_query
    await q.answer()
    uid = str(q.from_user.id)
    data = q.data or ""
    if not data.startswith("GET:"):
        await q.edit_message_text("Неизвестная кнопка.")
        return

    album = data[4:]
    ok, err, items = await asyncio.to_thread(send_getalbum, uid, album)
    if not ok:
        await q.edit_message_text(f"Ошибка: {err}")
        return

    await q.edit_message_text(f"🗂️ Альбом: {album}\n")
    if not items:
        await q.message.reply_text("📭 В этом альбоме пока пусто.", reply_markup=MAIN_KB)
        return

    media = []
    docs = []

    for kind, name, blob in items:
        if kind == "P":
            media.append(("P", name, blob))
        else:
            docs.append((name, blob))

    batch = []
    for _, name, blob in media:
        is_video = (name or "").lower().endswith((".mp4", ".mov", ".m4v"))
        if is_video:
            batch.append(InputMediaVideo(media=blob))
        else:
            batch.append(InputMediaPhoto(media=blob))

    for i in range(0, len(batch), 10):
        await q.message.reply_media_group(media=batch[i:i + 10])

    for name, blob in docs:
        f = InputFile(io.BytesIO(blob), filename=(name or "file.bin"))
        await q.message.reply_document(document=f)

    await q.message.reply_text("✅ Готово!", reply_markup=MAIN_KB)


def main():
    if not BOT_TOKEN:
        raise RuntimeError("Set BOT_TOKEN env var")

    app = ApplicationBuilder().token(BOT_TOKEN).build()

    app.add_handler(CommandHandler("start", start))
    app.add_handler(CommandHandler("albums", albums))
    app.add_handler(CallbackQueryHandler(on_callback))

    app.add_handler(MessageHandler(filters.PHOTO, handle_photo))
    app.add_handler(MessageHandler(filters.VIDEO, handle_video))
    app.add_handler(MessageHandler(filters.Document.ALL, handle_document))
    app.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, handle_text))

    app.run_polling()


if __name__ == "__main__":
    main()
