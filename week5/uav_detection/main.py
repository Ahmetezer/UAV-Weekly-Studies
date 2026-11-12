"""İHA modelini video/kamera üzerinde çalıştırır; panel ve isteğe bağlı kayıt üretir."""
import argparse
from pathlib import Path
import time

HERE = Path(__file__).resolve().parent


def target_classes(names, requested):
    """Genel COCO modelindeki her nesnenin yanlışlıkla Drone etiketlenmesini önler."""
    wanted = {name.casefold() for name in requested}
    return [index for index, name in names.items() if name.casefold() in wanted]


def main():
    import cv2
    import numpy as np
    from ultralytics import YOLO
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default=str(HERE/"videos/Smarter_Drone_Swarms.mp4"),
                        help="Video yolu, kamera numarası veya RTSP adresi")
    parser.add_argument("--model", type=Path, default=HERE/"models/drone_model.pt")
    parser.add_argument("--conf", type=float, default=0.20)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--device", default="cpu", help="cpu veya CUDA GPU numarası: 0")
    parser.add_argument("--save", type=Path, help="Çıktı MP4 yolu; verilmezse yalnız ekranda gösterir")
    parser.add_argument("--no-show", action="store_true")
    parser.add_argument("--max-frames", type=int, default=0, help="0: tüm video")
    parser.add_argument("--all-classes", action="store_true", help="Özel İHA filtresini kaldırır")
    parser.add_argument("--target-names", nargs="+",
                        default=["quadcopter","fixed-wing","drone","uav","iha","İHA"])
    args = parser.parse_args()
    if not 0 < args.conf <= 1 or args.imgsz <= 0:
        parser.error("Güven eşiği (0,1] aralığında; giriş boyutu pozitif olmalı.")
    if not args.model.is_file():
        raise FileNotFoundError(args.model)
    if args.save and args.save.exists():
        raise FileExistsError(f"Önceki kayıt korunuyor: {args.save}")
    model = YOLO(str(args.model))
    classes = None if args.all_classes else target_classes(model.names, args.target_names)
    if classes == []:
        raise ValueError("Model İHA sınıfı içermiyor. Eğitilmiş drone_model.pt dosyasını seçin.")
    source = int(args.source) if args.source.isdecimal() else args.source
    capture = cv2.VideoCapture(source)
    if not capture.isOpened():
        raise RuntimeError(f"Görüntü kaynağı açılamadı: {args.source}")
    fps = capture.get(cv2.CAP_PROP_FPS)
    fps = fps if 0 < fps < 1000 else 25.0
    writer, frames, started = None, 0, time.perf_counter()
    try:
        while True:
            ok, frame = capture.read()
            if not ok:
                break
            infer_start = time.perf_counter()
            result = model.predict(frame, conf=args.conf, imgsz=args.imgsz, classes=classes,
                                   agnostic_nms=not args.all_classes,
                                   device=args.device, verbose=False)[0]
            inference_ms = (time.perf_counter()-infer_start)*1000
            height, width = frame.shape[:2]
            panel_width = 300
            canvas = np.zeros((height,width+panel_width,3),dtype=np.uint8)
            canvas[:,:width] = frame
            count = len(result.boxes)
            for box in result.boxes:
                x1,y1,x2,y2 = map(int,box.xyxy[0].tolist())
                label = model.names[int(box.cls[0])] if args.all_classes else "Drone"
                cv2.rectangle(canvas,(x1,y1),(x2,y2),(0,220,100),2)
                cv2.putText(canvas,f"{label} {float(box.conf[0]):.2f}",(x1,max(22,y1-8)),
                            cv2.FONT_HERSHEY_SIMPLEX,0.65,(0,220,100),2)
            frames += 1
            lines = ["IHA TESPITI", "Drone var" if count else "Drone yok",
                     f"Tespit: {count}", f"Model: {inference_ms:.1f} ms",
                     f"Isleme: {frames/(time.perf_counter()-started):.1f} FPS",
                     f"Esik: {args.conf:.2f}", "Q/ESC: cikis", "BOSLUK: duraklat"]
            if args.all_classes:
                lines[1] = "Genel nesne tespiti"
            for i,line in enumerate(lines):
                cv2.putText(canvas,line,(width+15,35+i*35),cv2.FONT_HERSHEY_SIMPLEX,
                            0.6,(225,230,235),1,cv2.LINE_AA)
            if args.save:
                if writer is None:
                    args.save.parent.mkdir(parents=True,exist_ok=True)
                    writer = cv2.VideoWriter(str(args.save),cv2.VideoWriter_fourcc(*"mp4v"),
                                             fps,(width+panel_width,height))
                    if not writer.isOpened():
                        raise RuntimeError("MP4 yazıcısı açılamadı.")
                writer.write(canvas)
            if not args.no_show:
                scale = min(1.0,1280/canvas.shape[1])
                display = cv2.resize(canvas,None,fx=scale,fy=scale) if scale<1 else canvas
                cv2.imshow("IHA tespiti",display)
                key = cv2.waitKey(1)&0xFF
                if key in (27,ord("q")):
                    break
                if key == ord(" "):
                    if cv2.waitKey(0)&0xFF in (27,ord("q")):
                        break
            if args.max_frames and frames>=args.max_frames:
                break
    finally:
        capture.release()
        if writer is not None:
            writer.release()
        if not args.no_show:
            cv2.destroyAllWindows()
    if frames == 0:
        raise RuntimeError("Kaynak açıldı ancak okunabilir video karesi alınamadı.")
    print(f"İşlenen kare: {frames}; tespit sayısı başarı yüzdesi değildir.")
    if args.save:
        print(f"Kayıt: {args.save}")


if __name__ == "__main__":
    main()
