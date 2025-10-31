#!/usr/bin/env python3
"""
ENSF 460 – Assignment 4 (Driver Project 4)
Mode 1 PC-side logger: receives ADC samples over UART and generates:
  - DataFrame with time_s, adc, voltage_v
  - CSV and Excel outputs
  - Two separate plots (ADC vs time, Voltage vs time)
"""

import argparse
import sys
import time
import re
from pathlib import Path

import serial            # pyserial
import pandas as pd      # pandas, openpyxl
import matplotlib.pyplot as plt

# === Configuration (adjustable by CLI flags too) ===
DEFAULT_VREF = 3.3          # volts; update if your ADC Vref differs
ADC_BITS      = 10          # PIC24F ADC resolution (10-bit)
ADC_MAX       = (1 << ADC_BITS) - 1  # 1023

# Expect a sync line like: "Syncing - sample period (ms): 100\n"
SYNC_RE = re.compile(rb"Syncing\s*-\s*sample\s*period\s*\(ms\):\s*(\d+)\s*")
# ADC values are sent as decimal tokens separated by commas, e.g. " 00123 ,"
NUM_RE  = re.compile(rb"(-?\d+)")

def wait_for_sync(ser, timeout_s=8.0):
    """Block until we see the MCU's sync line or timeout. Return sample_period_ms or None."""
    deadline = time.time() + timeout_s
    buf = bytearray()
    ser.timeout = 0.2
    while time.time() < deadline:
        chunk = ser.read(128)
        if not chunk:
            continue
        buf.extend(chunk)
        m = SYNC_RE.search(buf)
        if m:
            try:
                return int(m.group(1))
            except Exception:
                return None
    return None

def capture_samples(ser, duration_s=10.0):
    """
    Capture comma-delimited ADC integers for duration_s seconds.
    Firmware format assumed: "<spaces><digits><spaces>,"
    Returns: list of (time_s, adc_int).
    """
    t0 = time.time()
    ser.timeout = 0.05
    token = bytearray()
    out = []

    while (now := time.time()) - t0 < duration_s:
        b = ser.read(1)
        if not b:
            continue
        if b == b',':
            m = NUM_RE.search(token)
            if m:
                try:
                    val = int(m.group(1))
                    out.append((now - t0, val))
                except Exception:
                    pass
            token.clear()
        else:
            token.extend(b)
    return out

def to_dataframe(samples, vref):
    """
    Convert list[(t, adc)] -> DataFrame with clipped integer adc and computed voltage_v.
    Uses Vin = ADCBUF * Vref / (2^N). N = 10 for PIC24F. (Resolution formula per course notes.)
    """
    if not samples:
        return pd.DataFrame(columns=["time_s", "adc", "voltage_v"])
    df = pd.DataFrame(samples, columns=["time_s", "adc"])
    df["adc"] = df["adc"].clip(lower=0, upper=ADC_MAX).astype(int)
    df["voltage_v"] = df["adc"] * (vref / (1 << ADC_BITS))
    return df

def save_outputs(df, stem="adc_capture"):
    """Write CSV and Excel. Return (csv_path, xlsx_path or None)."""
    ts = time.strftime("%Y%m%d_%H%M%S")
    csv_path = Path(f"{stem}_{ts}.csv")
    xlsx_path = Path(f"{stem}_{ts}.xlsx")

    df.to_csv(csv_path, index=False)
    try:
        df.to_excel(xlsx_path, index=False)
    except Exception as e:
        print(f"Warning: Excel save failed ({e}). CSV still saved.", file=sys.stderr)
        xlsx_path = None
    return csv_path, xlsx_path

def plot_two_figures(df):
    """Two separate figures, as per assignment: ADC vs time, then Voltage vs time."""
    if df.empty:
        print("No data captured; skipping plots.")
        return

    # Figure 1: ADC vs time
    plt.figure()
    plt.plot(df["time_s"], df["adc"])
    plt.xlabel("Time (s)")
    plt.ylabel("ADC (counts)")
    plt.title("ADC vs Time")
    plt.grid(True)

    # Figure 2: Voltage vs time
    plt.figure()
    plt.plot(df["time_s"], df["voltage_v"])
    plt.xlabel("Time (s)")
    plt.ylabel("Voltage (V)")
    plt.title("Voltage vs Time")
    plt.grid(True)

    plt.show()

def main():
    p = argparse.ArgumentParser(description="ENSF 460 UART ADC logger (Mode 1)")
    p.add_argument("--port", required=True, help="Serial port (e.g., COM5 or /dev/tty.usbserial-XXXX)")
    p.add_argument("--baud", type=int, required=True, help="Baud rate (match MCU firmware)")
    p.add_argument("--duration", type=float, default=10.0, help="Capture duration in seconds (default: 10)")
    p.add_argument("--vref", type=float, default=DEFAULT_VREF, help="ADC reference voltage (default: 3.3)")
    p.add_argument("--out", default="adc_capture", help="Output file stem (default: adc_capture)")
    args = p.parse_args()

    # Open serial (8N1, blocking small timeouts)
    try:
        ser = serial.Serial(
            port=args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.2,
        )
    except Exception as e:
        print(f"ERROR: Could not open {args.port} @ {args.baud}: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Opened {args.port} @ {args.baud}. Close any terminal program first.")
    print("Waiting for MCU sync (press the board's start button for Mode 1 streaming)…")
    sp_ms = wait_for_sync(ser, timeout_s=8.0)
    if sp_ms is not None:
        print(f"Sync OK. MCU sample period ≈ {sp_ms} ms")
    else:
        print("No sync detected; proceeding anyway (will still parse commas).", file=sys.stderr)

    print(f"Capturing for {args.duration:.2f} s…")
    samples = capture_samples(ser, duration_s=args.duration)
    ser.close()
    print(f"Serial closed. Samples: {len(samples)}")

    df = to_dataframe(samples, vref=args.vref)

    # Terminal output preview for demo video
    with pd.option_context("display.max_rows", 20, "display.width", 120):
        print("\n=== Preview (first 10 rows) ===")
        print(df.head(10))
        print("\n=== Summary ===")
        print(df.describe(numeric_only=True) if not df.empty else "No data.")

    csv_path, xlsx_path = save_outputs(df, stem=args.out)
    print(f"\nSaved CSV:  {csv_path}")
    if xlsx_path:
        print(f"Saved XLSX: {xlsx_path}")

    plot_two_figures(df)

if __name__ == "__main__":
    main()
