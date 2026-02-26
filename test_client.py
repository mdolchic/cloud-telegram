import os
import socket


HOST = "127.0.0.1"
PORT = 5555

def recv_exact(sock: socket.socket, n: int) -> bytes:
    data = bytearray()
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise RuntimeError("connection closed while receiving bytes")
        data.extend(chunk)
    return bytes(data)

def recv_line(sock: socket.socket) -> str:
    data = bytearray()
    while True:
        b = sock.recv(1)
        if not b:
            raise RuntimeError("connection closed while receiving line")
        if b == b"\n":
            break
        if b != b"\r":
            data.extend(b)
        if len(data) > 1_000_000:
            raise RuntimeError("line too long")
    return data.decode("utf-8", errors="strict")


def send_addj(sock: socket.socket, uid: str, jpeg_path: str) -> str:
    with open(jpeg_path, "rb") as f:
        jpg = f.read()

    cmd = f"ADDJ {uid} {len(jpg)}\n".encode("utf-8")
    sock.sendall(cmd)
    sock.sendall(jpg)

    line = recv_line(sock)
    if line.startswith("OK "):
        filename = line.split(" ", 1)[1]
        return filename
    raise RuntimeError(f"ADDJ failed: {line}")


def get_allj(sock: socket.socket, uid: str, out_dir: str) -> int:
    os.makedirs(out_dir, exist_ok=True)

    sock.sendall(f"GETALLJ {uid}\n".encode("utf-8"))

    first = recv_line(sock)
    if not first.startswith("OK "):
        raise RuntimeError(f"GETALLJ failed: {first}")

    count = int(first.split(" ", 1)[1])

    got = 0
    while True:
        header = recv_line(sock)
        if header == "END":
            break

        parts = header.split()
        if len(parts) != 3 or parts[0] != "JPEG":
            raise RuntimeError(f"bad JPEG header: {header}")

        filename = parts[1]
        nbytes = int(parts[2])

        data = recv_exact(sock, nbytes)

        out_path = os.path.join(out_dir, filename)
        with open(out_path, "wb") as f:
            f.write(data)

        got += 1

    return got


def main():
    uid = "123"
    jpeg_path = "test.jpg"
    out_dir = "out"

    if not os.path.exists(jpeg_path):
        print(f"Put a JPEG named '{jpeg_path}' next to this script and run again.")
        return

    with socket.create_connection((HOST, PORT)) as sock:
        name = send_addj(sock, uid, jpeg_path)
        print(f"Uploaded as: {name}")

        got = get_allj(sock, uid, out_dir)
        print(f"Downloaded {got} file(s) into ./{out_dir}/")


if __name__ == "__main__":
    main()