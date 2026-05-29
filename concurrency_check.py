import http.client
import socket
import threading
import time
from queue import Queue


HOST = "127.0.0.1"
PORT = 5555
QUICK_CLIENTS = 10
BLOCK_SECONDS = 3.0


def recv_line(sock: socket.socket) -> str:
    data = bytearray()
    while True:
        chunk = sock.recv(1)
        if not chunk:
            raise RuntimeError("connection closed")
        if chunk == b"\n":
            return data.decode("utf-8", errors="replace")
        if chunk != b"\r":
            data.extend(chunk)


def blocking_client(errors: Queue) -> None:
    try:
        with socket.create_connection((HOST, PORT)) as sock:
            time.sleep(BLOCK_SECONDS)
            sock.sendall(
                b"GET /health HTTP/1.1\r\n"
                b"Host: 127.0.0.1\r\n"
                b"Connection: close\r\n"
                b"\r\n"
            )
            payload = sock.recv(4096)
            if b"200 OK" not in payload:
                raise RuntimeError(f"blocking client got unexpected reply: {payload!r}")
    except Exception as exc:  # noqa: BLE001
        errors.put(exc)


def quick_client(idx: int, results: list[float], errors: Queue) -> None:
    try:
        started = time.perf_counter()
        conn = http.client.HTTPConnection(HOST, PORT, timeout=10)
        try:
            conn.request("GET", "/health")
            response = conn.getresponse()
            payload = response.read()
        finally:
            conn.close()
        if response.status != 200 or b'"status":"ok"' not in payload:
            raise RuntimeError(f"client {idx} got unexpected reply: {response.status} {payload!r}")
        results[idx] = time.perf_counter() - started
    except Exception as exc:  # noqa: BLE001
        errors.put(exc)


def main() -> None:
    print(f"Connecting one blocking client for {BLOCK_SECONDS:.1f}s...")
    errors: Queue = Queue()
    blocker = threading.Thread(target=blocking_client, args=(errors,))
    blocker.start()

    time.sleep(0.2)

    results = [0.0] * QUICK_CLIENTS
    threads = [
        threading.Thread(target=quick_client, args=(idx, results, errors))
        for idx in range(QUICK_CLIENTS)
    ]

    started = time.perf_counter()
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    blocker.join()

    if not errors.empty():
        raise RuntimeError(f"concurrency check failed: {errors.get()}")

    total = time.perf_counter() - started

    print(f"Quick clients completed in {total:.3f}s total")
    print(
        "Per-client latencies:",
        ", ".join(f"{value:.3f}s" for value in results),
    )
    print(
        "If the server handles clients in parallel, the quick clients should finish "
        "well before the blocking client sleep ends."
    )


if __name__ == "__main__":
    main()
