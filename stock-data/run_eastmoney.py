"""东方财富K线下载 - 单线程2秒间隔"""
import subprocess
import json
import csv
import time
import sys
from pathlib import Path

DATA_DIR = Path("D:/股票数据/kline")
DELAY = 2

DATA_DIR.mkdir(parents=True, exist_ok=True)

with open("D:/股票数据/stock_list.json", "r", encoding="utf-8") as f:
    stocks = json.load(f)

existing = {p.stem for p in DATA_DIR.glob("*.csv") if p.stat().st_size > 100}
todo = [s for s in stocks if s["code"] not in existing]

print(f"已有{len(existing)} 待下载{len(todo)} 预计{len(todo)*DELAY//60}分钟", flush=True)

ok = fail = 0
start = time.time()

for i, s in enumerate(todo, 1):
    code = s["code"]
    try:
        r = subprocess.run(
            f"opencli eastmoney kline {code} --period day --limit 1000 -f json",
            shell=True, capture_output=True, text=True, timeout=25,
        )
        if r.returncode == 0:
            data = json.loads(r.stdout)
            if data and isinstance(data, list):
                filepath = DATA_DIR / f"{code}.csv"
                with open(filepath, "w", newline="", encoding="utf-8-sig") as f:
                    writer = csv.DictWriter(f, fieldnames=data[0].keys())
                    writer.writeheader()
                    writer.writerows(data)
                ok += 1
            else:
                fail += 1
        else:
            fail += 1
            err = r.stderr[:80].replace("\n", " ")
            if "fetch failed" in err or "closed" in err:
                print(f"\n检测到限流，安全停止于 {i}/{len(todo)}", flush=True)
                break
    except Exception as e:
        fail += 1

    if i % 50 == 0:
        pct = i / len(todo) * 100
        elapsed = time.time() - start
        rate = i / elapsed if elapsed > 0 else 0
        eta = (len(todo) - i) / rate if rate > 0 else 0
        print(
            f"[{pct:5.1f}%] {i}/{len(todo)} OK={ok} FAIL={fail} "
            f"| {rate:.1f}/s | ETA={eta:.0f}s",
            flush=True,
        )

    time.sleep(DELAY)

elapsed = time.time() - start
total = len(list(DATA_DIR.glob("*.csv")))
print(f"\n完成: {total}只 OK={ok} FAIL={fail} | {elapsed/60:.0f}分钟", flush=True)
