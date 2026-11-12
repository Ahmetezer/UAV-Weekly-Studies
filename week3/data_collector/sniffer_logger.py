"""Tek sniffer kartının gerçek RSSI/MAC metadata satırlarını CSV'ye kaydeder."""
import argparse
import csv
from datetime import datetime
from pathlib import Path
import time
from lib.drop_lab import LinePort


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port",required=True)
    parser.add_argument("--seconds",type=int,default=60)
    parser.add_argument("--out",type=Path)
    args=parser.parse_args()
    if args.seconds<1:parser.error("Süre pozitif olmalı.")
    output=args.out or Path(__file__).resolve().parent/"data"/(
        "sniffer_"+datetime.now().strftime("%Y%m%d_%H%M%S")+".csv")
    output.parent.mkdir(parents=True,exist_ok=True)
    columns="time_us,channel,rssi,type,subtype,retry,sequence,callback_bytes,transmitter,receiver".split(",")
    count=0;port=LinePort(args.port)
    try:
        with output.open("x",newline="",encoding="utf-8") as file:
            writer=csv.writer(file);writer.writerow(columns)
            deadline=time.monotonic()+args.seconds
            while time.monotonic()<deadline:
                for line in port.lines():
                    if line.startswith("#"):
                        print(line);continue
                    fields=line.split(",")
                    if len(fields)==len(columns) and fields[0].isdigit():
                        writer.writerow(fields);count+=1
                time.sleep(0.005)
    finally:
        port.close()
    print(f"{count} gerçek kayıt: {output}")


if __name__=="__main__":
    main()
