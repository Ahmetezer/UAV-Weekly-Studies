"""İki ESP8266 seri portundan gerçek ölçüm toplar."""
from pathlib import Path
from lib.drop_lab import collect_main
if __name__=="__main__":
    collect_main(test=False, directory=Path(__file__).resolve().parent)
