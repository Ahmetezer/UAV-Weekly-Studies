"""Etiketli YOLO veri setiyle yeni bir İHA modeli eğitir."""
import argparse
from pathlib import Path


def main():
    from ultralytics import YOLO
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data",type=Path,required=True,help="train/val ve names içeren data.yaml")
    parser.add_argument("--base",type=Path,required=True,help="Örneğin models/baseline/yolo11n.pt")
    parser.add_argument("--epochs",type=int,default=100)
    parser.add_argument("--device",default="cpu")
    parser.add_argument("--imgsz",type=int,default=640)
    args=parser.parse_args()
    if not args.data.is_file() or not args.base.is_file():
        parser.error("Veri tanımı ve başlangıç modeli mevcut dosyalar olmalı.")
    if args.epochs<1:parser.error("Epoch pozitif olmalı.")
    model=YOLO(str(args.base))
    model.train(data=str(args.data),epochs=args.epochs,imgsz=args.imgsz,device=args.device,
                project=str(Path(__file__).resolve().parent/"runs"),name="iha_training",
                exist_ok=False)
    print("Gerçek eğitim tamamlandı. runs/iha_training/weights/best.pt dosyasını main.py --model ile seçin.")


if __name__=="__main__":
    main()
