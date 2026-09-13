import socket
import subprocess
import time
import signal
import os
import sys

def send_and_recv(sock, cmd_str):
    sock.sendall(cmd_str.encode('utf-8'))
    time.sleep(0.05)
    return sock.recv(4096).decode('utf-8')

def run_integration_tests():
    print("=== Starting MiniRedis Live Integration Tests ===")
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.abspath(os.path.join(script_dir, ".."))
    
    bin_path = os.path.join(repo_root, "build", "MiniRedis")
    if not os.path.exists(bin_path):
        bin_path = os.path.join(os.getcwd(), "MiniRedis")
    
    dump_path = os.path.join(repo_root, "dump.rdb")
    if os.path.exists(dump_path):
        os.remove(dump_path)

    # Start MiniRedis server
    server_proc = subprocess.Popen(
        [bin_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=repo_root
    )
    time.sleep(0.3)

    try:
        # Connect client 1
        s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s1.connect(("127.0.0.1", 8080))
        print("[+] Connected client 1")

        # Test PING
        res = send_and_recv(s1, "PING\r\n")
        assert "+PONG\r\n" in res, f"Expected +PONG, got {res}"
        print("[+] PING -> +PONG verified")

        # Test SET and GET
        res = send_and_recv(s1, "*3\r\n$3\r\nSET\r\n$4\r\nname\r\n$7\r\nSaurabh\r\n")
        assert "+OK\r\n" in res, f"Expected +OK, got {res}"
        print("[+] SET name Saurabh verified")

        res = send_and_recv(s1, "*2\r\n$3\r\nGET\r\n$4\r\nname\r\n")
        assert "$7\r\nSaurabh\r\n" in res, f"Expected $7\\r\\nSaurabh\\r\\n, got {res}"
        print("[+] GET name -> Saurabh verified")

        # Test INCR & DECR
        res = send_and_recv(s1, "INCR counter\r\n")
        assert ":1\r\n" in res, f"Expected :1, got {res}"
        res = send_and_recv(s1, "INCR counter\r\n")
        assert ":2\r\n" in res, f"Expected :2, got {res}"
        res = send_and_recv(s1, "DECR counter\r\n")
        assert ":1\r\n" in res, f"Expected :1, got {res}"
        print("[+] INCR/DECR verified")

        # Test Concurrent Client 2
        s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s2.connect(("127.0.0.1", 8080))
        print("[+] Connected client 2")

        res = send_and_recv(s2, "GET counter\r\n")
        assert "$1\r\n1\r\n" in res, f"Expected $1\\r\\n1\\r\\n, got {res}"
        print("[+] Client 2 read state from Client 1 successfully")

        # Test EXPIRE and TTL
        res = send_and_recv(s1, "EXPIRE counter 10\r\n")
        assert ":1\r\n" in res, f"Expected :1, got {res}"
        res = send_and_recv(s1, "TTL counter\r\n")
        assert res.startswith(":"), f"Expected integer TTL, got {res}"
        print("[+] EXPIRE and TTL verified")

        # Test Pipeline of commands
        pipeline = "SET k1 v1\r\nSET k2 v2\r\nGET k1\r\nGET k2\r\n"
        s1.sendall(pipeline.encode('utf-8'))
        time.sleep(0.1)
        res = s1.recv(4096).decode('utf-8')
        assert "+OK\r\n+OK\r\n$2\r\nv1\r\n$2\r\nv2\r\n" in res, f"Pipelined response unexpected: {res}"
        print("[+] Pipelined requests handled in correct order")

        # Test Malformed input (crash resilience)
        s1.sendall(b"*invalid\r\n")
        time.sleep(0.05)
        # Server should still be alive and responsive
        res = send_and_recv(s1, "PING\r\n")
        assert "+PONG\r\n" in res, "Server crashed on malformed RESP!"
        print("[+] Crash resilience verified: server survived malformed payload")

        s1.close()
        s2.close()

        # Test SIGINT graceful shutdown and persistence
        print("[*] Sending SIGINT to server for clean shutdown...")
        server_proc.send_signal(signal.SIGINT)
        server_proc.wait(timeout=3)
        print("[+] Server exited gracefully with code:", server_proc.returncode)

        # Verify dump.rdb was created / persisted
        assert os.path.exists(dump_path), f"dump.rdb at {dump_path} was not created on graceful shutdown!"
        with open(dump_path, "r") as f:
            content = f.read()
            assert "name=Saurabh" in content, f"dump.rdb missing name=Saurabh: {content}"
            assert "k1=v1" in content, f"dump.rdb missing k1=v1: {content}"
        print("[+] dump.rdb persistence verified after SIGINT shutdown")

        print("\n==========================================")
        print("  LIVE INTEGRATION TESTS PASSED (100%)")
        print("==========================================")

    finally:
        if server_proc.poll() is None:
            server_proc.kill()

if __name__ == "__main__":
    run_integration_tests()
