import http.client
import json
import os
import urllib.parse


HOST = "127.0.0.1"
PORT = 5555


def request(method: str, path: str, body: bytes = b"", headers: dict | None = None):
    conn = http.client.HTTPConnection(HOST, PORT, timeout=10)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        response = conn.getresponse()
        payload = response.read()
        return response.status, dict(response.getheaders()), payload
    finally:
        conn.close()


def main():
    uid = "123"
    jpeg_path = "test.jpg"
    album = "demo"

    if not os.path.exists(jpeg_path):
        print(f"Put a JPEG named '{jpeg_path}' next to this script and run again.")
        return

    with open(jpeg_path, "rb") as fh:
        data = fh.read()

    upload_path = "/users/{}/files?kind=P&name={}".format(
        urllib.parse.quote(uid, safe=""),
        urllib.parse.quote("test.jpg", safe=""),
    )
    status, _, payload = request("POST", upload_path, body=data, headers={"Content-Length": str(len(data))})
    print("UPLOAD", status, payload.decode("utf-8", errors="replace"))

    commit_path = "/users/{}/albums/{}/commit".format(
        urllib.parse.quote(uid, safe=""),
        urllib.parse.quote(album, safe=""),
    )
    status, _, payload = request("POST", commit_path, headers={"Content-Length": "0"})
    print("COMMIT", status, payload.decode("utf-8", errors="replace"))

    albums_path = "/users/{}/albums".format(urllib.parse.quote(uid, safe=""))
    status, _, payload = request("GET", albums_path)
    print("ALBUMS", status, payload.decode("utf-8", errors="replace"))

    album_path = "/users/{}/albums/{}".format(
        urllib.parse.quote(uid, safe=""),
        urllib.parse.quote(album, safe=""),
    )
    status, _, payload = request("GET", album_path)
    print("GET ALBUM", status, payload.decode("utf-8", errors="replace"))

    if status != 200:
        return

    items = json.loads(payload)["items"]
    for item in items:
        content_path = "/users/{}/albums/{}/items/{}/content".format(
            urllib.parse.quote(uid, safe=""),
            urllib.parse.quote(album, safe=""),
            urllib.parse.quote(item["id"], safe=""),
        )
        status, headers, blob = request("GET", content_path)
        out_name = headers.get("X-Item-Name", item["name"])
        out_path = os.path.join("out", out_name)
        os.makedirs("out", exist_ok=True)
        with open(out_path, "wb") as fh:
            fh.write(blob)
        print("DOWNLOAD", status, out_path, len(blob))


if __name__ == "__main__":
    main()
