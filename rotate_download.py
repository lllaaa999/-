"""多平台轮转下载——每个平台只取一小批就切换，避免被封"""
import subprocess
import urllib.request
import json
import csv
import time
import sys
from pathlib import Path

DATA_DIR = Path("D:/股票数据/kline")
DATA_DIR.mkdir(parents=True, exist_ok=True)

# 各平台每轮下载量（保守值，不会触发限流）
ROUNDS = [
    {"name": "eastmoney", "count": 15, "delay": 1.5},
    {"name": "sina", "count": 80, "delay": 0.5},
    {"name": "xueqiu", "count": 10, "delay": 3},
]

SINA_URL = "https://money.finance.sina.com.cn/quotes_service/api/json_v2.php/CN_MarketData.getKLineData"


def dl_eastmoney(code):
    r = subprocess.run(
        f"opencli eastmoney kline {code} --period day --limit 1000 -f json",
        shell=True, capture_output=True, text=True, timeout=25,
    )
    if r.returncode != 0:
        return None
    return json.loads(r.stdout)


def dl_sina(symbol):
    url = f"{SINA_URL}?symbol={symbol}&scale=240&ma=no&datalen=1300"
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0", "Referer": "https://finance.sina.com.cn/",
    })
    with urllib.request.urlopen(req, timeout=20) as resp:
        return json.loads(resp.read().decode("gbk"))


def dl_xueqiu(code):
    sym = f"SH{code}" if code.startswith(("60", "68")) else f"SZ{code}"
    r = subprocess.run(
        f"opencli xueqiu kline {sym} --days 1825 -f json",
        shell=True, capture_output=True, text=True, timeout=35,
    )
    if r.returncode != 0:
        return None
    return json.loads(r.stdout)


# 加载待下载列表
with open("D:/股票数据/stock_list.json", "r", encoding="utf-8") as f:
    stocks = json.load(f)

existing = {p.stem for p in DATA_DIR.glob("*.csv") if p.stat().st_size > 100}
todo = [s for s in stocks if s["code"] not in existing]

print(f"已有{len(existing)} | 待下载{len(todo)} | 轮转模式", flush=True)

dl_funcs = {"eastmoney": dl_eastmoney, "sina": dl_sina, "xueqiu": dl_xueqiu}

ok = fail = 0
start = time.time()
idx = 0  # 当前在 todo 中的位置
round_num = 0

while idx < len(todo):
    for plat in ROUNDS:
        if idx >= len(todo):
            break

        batch = todo[idx : idx + plat["count"]]
        batch_ok = 0

        for s in batch:
            code = s["code"]
            sym = s.get("symbol", "")
            if not sym:
                sym = f"sh{code}" if code.startswith(("60", "68")) else f"sz{code}"

            try:
                func = dl_funcs[plat["name"]]
                arg = code if plat["name"] != "sina" else sym
                data = func(arg)

                if data and isinstance(data, list) and len(data) > 0:
                    filepath = DATA_DIR / f"{code}.csv"
                    with open(filepath, "w", newline="", encoding="utf-8-sig") as f:
                        w = csv.DictWriter(f, fieldnames=data[0].keys())
                        w.writeheader()
                        w.writerows(data)
                    batch_ok += 1

                time.sleep(plat["delay"])
            except Exception:
                pass

        ok += batch_ok
        fail += len(batch) - batch_ok
        idx += len(batch)

        pct = idx / len(todo) * 100
        elapsed = time.time() - start
        rate = idx / elapsed if elapsed > 0 else 0
        eta = (len(todo) - idx) / rate if rate > 0 else 0

        print(
            f"[{pct:5.1f}%] {idx}/{len(todo)} OK={ok} FAIL={fail} "
            f"| {plat['name']} +{batch_ok} | {rate:.1f}/s | ETA={eta:.0f}s",
            flush=True,
        )

    # 每轮结束汇报
    round_num += 1
    elapsed = time.time() - start
    rate = idx / elapsed if elapsed > 0 else 0
    eta = (len(todo) - idx) / rate if rate > 0 else 0
    print(
        f"--- 第{round_num}轮完成: +{ok + fail}只(累计{idx}/{len(todo)}) "
        f"| {rate:.1f}/s | 剩余≈{eta/60:.0f}分钟 ---",
        flush=True,
    )

    # 稍息
    if idx < len(todo):
        time.sleep(5)

elapsed = time.time() - start
total = len(list(DATA_DIR.glob("*.csv")))
print(f"\n完成: {total}只 OK={ok} FAIL={fail} | {elapsed/60:.0f}分钟", flush=True)
