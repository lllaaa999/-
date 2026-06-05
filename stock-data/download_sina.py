"""
全A股K线数据下载（新浪财经版）
纯HTTP API，无需浏览器，单线程+延时避免封禁
存储: D:/股票数据/kline/<代码>.csv
"""
import urllib.request
import json
import time
import sys
import csv
from pathlib import Path

DATA_DIR = Path("D:/股票数据/kline")
DELAY = 2     # 每只间隔秒数（新浪API，非东方财富）
DATALEN = 1300  # 约5.5年日线

DATA_DIR.mkdir(parents=True, exist_ok=True)

# 新浪K线URL
KLINE_URL = ("https://money.finance.sina.com.cn/quotes_service/api/"
             "json_v2.php/CN_MarketData.getKLineData")


def download_one(symbol: str) -> list[dict] | None:
    """下载单只K线"""
    url = f"{KLINE_URL}?symbol={symbol}&scale=240&ma=no&datalen={DATALEN}"
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
        "Referer": "https://finance.sina.com.cn/",
    })
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            raw = resp.read().decode("gbk")
        return json.loads(raw)
    except Exception as e:
        print(f"  FAIL: {e}")
        return None


def main():
    # 加载代码列表
    list_file = Path("D:/股票数据/stock_list.json")
    if not list_file.exists():
        print("[!] stock_list.json 不存在")
        return

    with open(list_file, "r", encoding="utf-8") as f:
        stocks = json.load(f)

    # 筛待下载
    todo = []
    skipped = 0
    for s in stocks:
        fp = DATA_DIR / f"{s['code']}.csv"
        if fp.exists() and fp.stat().st_size > 100:
            skipped += 1
        else:
            todo.append(s)

    total = len(stocks)
    print(f"总计: {total} | 已下载: {skipped} | 待下载: {len(todo)}")
    if not todo:
        print("全部完成!")
        return

    eta_min = len(todo) * DELAY / 60
    print(f"预计: {len(todo)*DELAY}秒 ({eta_min:.0f}分钟)")
    print("=" * 50)

    ok = fail = 0
    start = time.time()

    for i, s in enumerate(todo, 1):
        sym = s.get("symbol", "")
        code = s["code"]
        name = s.get("name", "")

        # 没有symbol时自己拼
        if not sym:
            if code.startswith(("60", "68")):
                sym = f"sh{code}"
            else:
                sym = f"sz{code}"

        data = download_one(sym)

        if data:
            filepath = DATA_DIR / f"{code}.csv"
            with open(filepath, "w", newline="", encoding="utf-8-sig") as f:
                writer = csv.DictWriter(f, fieldnames=["day", "open", "high", "low", "close", "volume"])
                writer.writeheader()
                writer.writerows(data)
            ok += 1
        else:
            fail += 1

        if i % 10 == 0 or i == len(todo):
            pct = i / len(todo) * 100
            elapsed = time.time() - start
            eta = (len(todo) - i) * DELAY
            print(f"[{pct:5.1f}%] {i}/{len(todo)} | OK={ok} FAIL={fail} | "
                  f"耗时{elapsed:.0f}s | 剩余≈{eta:.0f}s | {s['code']} {name}")

        if i < len(todo):
            time.sleep(DELAY)

    elapsed = time.time() - start
    print(f"\n{'='*50}")
    print(f"完成! OK={ok} FAIL={fail} | {elapsed:.0f}秒 ({elapsed/60:.1f}分)")
    print(f"存储: {DATA_DIR} ({len(list(DATA_DIR.glob('*.csv')))}只)")


if __name__ == "__main__":
    main()
