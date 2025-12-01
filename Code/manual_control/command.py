import socket
import sys
import termios
import tty

TCP_IP = "192.168.50.111"   # Replace with your ESP32 IP
TCP_PORT = 5005
TIMEOUT_S = 5

def getch():
    """Read a single character (no Enter required). Works on Linux/macOS/WSL."""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def wifi_setup():
    print(f"Connecting to ESP32 at {TCP_IP}:{TCP_PORT}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(TIMEOUT_S)
    s.connect((TCP_IP, TCP_PORT))
    print("Connected to ESP32.")
    return s

def main():
    sock = wifi_setup()
    print("Manual control ready. Use WASD to move, x to stop, q to quit.")
    while True:
        ch = getch()
        if ch in ["w", "a", "s", "d", "x"]:
            sock.sendall((ch + "\n").encode("utf-8"))
            print(f"Sent: {ch}")

        elif ch == "q":
            print("Quitting...")
            break

    sock.close()

if __name__ == "__main__":
    main()
