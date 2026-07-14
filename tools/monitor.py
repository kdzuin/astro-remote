#!/usr/bin/env python
# Serial monitor that tees to a file. Non-interactive (works from an agent).
# Usage: ~/.pio-venv/bin/python tools/monitor.py [port] [logfile]
import sys, serial

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbserial-B15213141E"
logfile = sys.argv[2] if len(sys.argv) > 2 else "monitor.log"

s = serial.Serial(port, 115200, timeout=1)
print(f"logging {port} -> {logfile} (Ctrl-C to stop)")
with open(logfile, "w") as f:
    try:
        while True:
            line = s.readline().decode("utf-8", "replace").rstrip()
            if line:
                print(line)
                f.write(line + "\n")
                f.flush()
    except KeyboardInterrupt:
        pass
s.close()
