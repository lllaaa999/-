"""单线程无延迟全量下载新浪K线"""
import urllib.request, json, time, csv, sys
from pathlib import Path

DATA_DIR = Path("D:/股票数据/kline")
KLINE_URL = "https://money.finance.sina.com.cn/quotes_service/api/json_v2.php/CN_MarketData.getKLineData"

DATA_DIR.mkdir(parents=True, exist_ok=True)

stocks = json.load(open("D:/股票数据/stock_list.json", "r", encoding="utf-8"))
existing = {f.stem for f in DATA_DIR.glob("*.csv") if f.stat().st_size > 100}
todo = [s for s in stocks if s["code"] not in existing]

print(f"已有{len(existing)} 待下载{len(todo)} 预计{len(todo)//60}分钟", flush=True)

ok = fail = 0
start = time.time()

for i, s in enumerate(todo, 1):
    sym = s.get("symbol", "")
    if not sym:
        sym = f"sh{s['code']}" if s["code"].startswith(("60", "68")) else f"sz{s['code']}"

    try:
        url = f"{KLINE_URL}?symbol={sym}&scale=240&ma=no&datalen=1300"
        req = urllib.request.Request(url, headers={
            "User-Agent": "Mozilla/5.0",
            "Referer": "https://finance.sina.com.cn/",
        })
        with urllib.request.urlopen(req, timeout=20) as resp:
            data = json.loads(resp.read().decode("gbk"))

        if data:
            fp = DATA_DIR / f"{s['code']}.csv"
            with open(fp, "w", newline="", encoding="utf-8-sig") as f:
                w = csv.DictWriter(f, fieldnames=["day", "open", "high", "low", "close", "volume"])
                w.writeheader()
                w.writerows(data)
            ok += 1
        else:
            fail += 1
    except Exception:
        fail += 1

    if i % 50 == 0 or i == len(todo):
        pct = i / len(todo) * 100
        elapsed = time.time() - start
        rate = i / elapsed if elapsed > 0 else 0
        eta = (len(todo) - i) / rate if rate > 0 else 0
        print(
            f"[{pct:5.1f}%] {i}/{len(todo)} OK={ok} FAIL={fail} "
            f"| {elapsed:.0f}s | {rate:.1f}/s | ETA={eta:.0f}s",
            flush=True,
        )

elapsed = time.time() - start
total = len(list(DATA_DIR.glob("*.csv")))
print(f"DONE: {total}只 OK={ok} FAIL={fail} | {elapsed/60:.0f}分钟", flush=True)
