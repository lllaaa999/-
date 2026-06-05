"""
全A股代码列表获取（带重试和延时）
保存到 D:/股票数据/stock_list.json
"""
import urllib.request
import json
import time
from pathlib import Path

OUTPUT = Path("D:/股票数据/stock_list.json")

all_stocks = []
total = None

for pn in range(1, 60):
    url = (
        f"http://push2.eastmoney.com/api/qt/clist/get?"
        f"pn={pn}&pz=100&po=1&np=1&fltt=2&invt=2&fid=f3"
        f"&fs=m:0+t:6,m:0+t:80,m:1+t:2,m:1+t:23"
        f"&fields=f12,f14"
    )

    # 重试最多3次
    for attempt in range(3):
        try:
            req = urllib.request.Request(url, headers={
                "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
                "Referer": "https://quote.eastmoney.com/",
            })
            with urllib.request.urlopen(req, timeout=15) as resp:
                data = json.loads(resp.read().decode())

            stocks = data.get("data", {}).get("diff", [])
            if not stocks:
                break

            if total is None:
                total = data.get("data", {}).get("total", 0)
                print(f"总记录: {total}")

            for s in stocks:
                all_stocks.append({"code": s["f12"], "name": s["f14"]})

            print(f"  页{pn}: +{len(stocks)} -> 累计 {len(all_stocks)}")
            break  # 成功，跳出重试

        except Exception as e:
            if attempt < 2:
                wait = (attempt + 1) * 2
                print(f"  页{pn} 失败({e.__class__.__name__}), {wait}秒后重试...")
                time.sleep(wait)
            else:
                print(f"  页{pn} 3次重试均失败，跳过")

    # 如果这页没数据或已达总数，结束
    if not stocks or (total and len(all_stocks) >= total):
        break

    time.sleep(0.3)  # 页间延时，避免被限流

# 保存
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
with open(OUTPUT, "w", encoding="utf-8") as f:
    json.dump(all_stocks, f, ensure_ascii=False)

print(f"\n[OK] 保存 {len(all_stocks)} 只 -> {OUTPUT}")
