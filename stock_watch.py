"""A股实时行情查询工具
用法:
  python stock_watch.py 601991           # 查单只股票
  python stock_watch.py 601991,600519    # 查多只股票
  python stock_watch.py --hot            # 全A股涨幅榜 Top20
  python stock_watch.py --hot --board cyb # 创业板涨幅榜
  python stock_watch.py --hot --board kcb --top 50  # 科创板 Top50
  python stock_watch.py 601991 --detail  # 详细信息（含历史K线概要）
"""

import sys
import time
import argparse

import akshare as ak

from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.text import Text
from rich import box

console = Console()


def fetch_stock(code: str) -> dict[str, str] | None:
    """从全A实时行情中查找单只股票"""
    try:
        df = ak.stock_zh_a_spot_em()
        row = df[df["代码"] == code]
        if row.empty:
            return None
        r = row.iloc[0]
        return {
            "code": str(r["代码"]),
            "name": str(r["名称"]),
            "price": _fmt_num(r.get("最新价")),
            "change_pct": _fmt_num(r.get("涨跌幅")),
            "change_amt": _fmt_num(r.get("涨跌额")),
            "volume": _fmt_vol(r.get("成交量")),
            "amount": _fmt_amt(r.get("成交额")),
            "turnover": _fmt_num(r.get("换手率")),
            "pe": _fmt_num(r.get("市盈率-动态")),
            "high": _fmt_num(r.get("最高")),
            "low": _fmt_num(r.get("最低")),
            "open": _fmt_num(r.get("今开")),
            "prev_close": _fmt_num(r.get("昨收")),
        }
    except Exception as e:
        console.print(f"[red]获取 {code} 失败: {e}[/red]")
        return None


def fetch_hot_list(board: str = "all", top_n: int = 20) -> list[dict]:
    """获取涨幅榜"""
    try:
        df = ak.stock_zh_a_spot_em()

        # 板块过滤
        board_filter = {
            "sha": lambda d: d["代码"].astype(str).str.startswith("60"),
            "sza": lambda d: d["代码"].astype(str).str.startswith(("00", "002", "003")),
            "cyb": lambda d: d["代码"].astype(str).str.startswith("30"),
            "kcb": lambda d: d["代码"].astype(str).str.startswith("68"),
        }
        if board in board_filter:
            df = df[board_filter[board](df)]

        # 按涨幅降序
        df = df.sort_values("涨跌幅", ascending=False).head(top_n)

        stocks = []
        for _, r in df.iterrows():
            stocks.append({
                "code": str(r["代码"]),
                "name": str(r["名称"]),
                "price": _fmt_num(r.get("最新价")),
                "change_pct": _fmt_num(r.get("涨跌幅")),
                "change_amt": _fmt_num(r.get("涨跌额")),
                "volume": _fmt_vol(r.get("成交量")),
                "amount": _fmt_amt(r.get("成交额")),
                "turnover": _fmt_num(r.get("换手率")),
            })
        return stocks
    except Exception as e:
        console.print(f"[red]获取涨幅榜失败: {e}[/red]")
        return []


def fetch_detail(code: str) -> dict | None:
    """获取个股详细信息含近期K线"""
    info = fetch_stock(code)
    if not info:
        return None

    # 获取近30日K线
    try:
        df = ak.stock_zh_a_hist(symbol=code, period="daily", adjust="qfq")
        recent = df.tail(30)
        # 计算近期指标
        closes = recent["收盘"].astype(float)
        highs = recent["最高"].astype(float)
        lows = recent["最低"].astype(float)
        volumes = recent["成交量"].astype(float)

        info["k30_high"] = _fmt_num(highs.max())
        info["k30_low"] = _fmt_num(lows.min())
        info["k30_avg_vol"] = _fmt_vol(volumes.mean())
        info["k30_rise_days"] = str((recent["涨跌幅"].astype(float) > 0).sum())
        info["k30_chg"] = _fmt_num((closes.iloc[-1] - closes.iloc[0]) / closes.iloc[0] * 100)
        info["k30_ma5"] = _fmt_num(closes.tail(5).mean())
        info["k30_ma10"] = _fmt_num(closes.tail(10).mean())
    except Exception:
        pass

    return info


# --- 格式化 ---
def _fmt_num(val, default="-") -> str:
    if val is None or val == "-":
        return default
    try:
        v = float(val)
        if v == round(v, 0):
            return f"{int(v):,}"
        return f"{v:,.2f}"
    except (ValueError, TypeError):
        return str(val)


def _fmt_vol(val) -> str:
    """成交量转万手/亿手"""
    if val is None or val == "-":
        return "-"
    try:
        v = float(val)
        if v >= 1e8:
            return f"{v/1e8:.2f}亿手"
        return f"{v/1e4:.0f}万手"
    except (ValueError, TypeError):
        return str(val)


def _fmt_amt(val) -> str:
    """成交额转亿/万"""
    if val is None or val == "-":
        return "-"
    try:
        v = float(val)
        if v >= 1e8:
            return f"{v/1e8:.2f}亿"
        return f"{v/1e4:.0f}万"
    except (ValueError, TypeError):
        return str(val)


def _color_change(val: str) -> Text:
    """涨红色跌绿色"""
    try:
        v = float(val)
    except (ValueError, TypeError):
        return Text(str(val))
    color = "red" if v > 0 else "green" if v < 0 else "white"
    return Text(f"{v:+.2f}%", style=color)


# --- 渲染 ---
def render_single(stocks: list[dict]) -> None:
    """渲染个股详情表"""
    if not stocks:
        console.print("[yellow]无数据[/yellow]")
        return

    for s in stocks:
        pct = float(s["change_pct"]) if s["change_pct"] != "-" else 0
        style = "red" if pct > 0 else "green" if pct < 0 else "white"

        table = Table(title=f"{s['name']}({s['code']}) 实时行情",
                      box=box.SIMPLE_HEAVY, title_style=style, show_header=False,
                      width=60)
        table.add_column("指标", style="dim", width=12)
        table.add_column("数值", justify="right", width=18)
        table.add_column("指标", style="dim", width=12)
        table.add_column("数值", justify="right", width=18)

        table.add_row("最新价", Text(s["price"], style=style),
                      "涨幅", _color_change(s["change_pct"]))
        table.add_row("涨跌额", Text(s["change_amt"], style=style),
                      "换手率", Text(f"{s['turnover']}%"))
        table.add_row("今开", s["open"], "昨收", s["prev_close"])
        table.add_row("最高", s["high"], "最低", s["low"])
        table.add_row("成交量", s["volume"], "成交额", s["amount"])
        if s["pe"] != "-":
            table.add_row("市盈率(动)", s["pe"], "", "")

        console.print(table)
        console.print()


def render_detail(info: dict) -> None:
    """渲染个股详细信息（含K线概要）"""
    pct = float(info["change_pct"]) if info["change_pct"] != "-" else 0
    style = "red" if pct > 0 else "green" if pct < 0 else "white"

    # 基本信息表
    table = Table(title=f"{info['name']}({info['code']}) 详细信息",
                  box=box.SIMPLE_HEAVY, title_style=style, show_header=False,
                  width=64)
    table.add_column("指标", style="dim", width=12)
    table.add_column("数值", justify="right", width=20)
    table.add_column("指标", style="dim", width=12)
    table.add_column("数值", justify="right", width=20)

    table.add_row("最新价", Text(info["price"], style=style),
                  "涨幅", _color_change(info["change_pct"]))
    table.add_row("涨跌额", Text(info["change_amt"], style=style),
                  "换手率", Text(f"{info['turnover']}%"))
    table.add_row("今开", info["open"], "昨收", info["prev_close"])
    table.add_row("最高", info["high"], "最低", info["low"])
    table.add_row("成交量", info["volume"], "成交额", info["amount"])
    if info["pe"] != "-":
        table.add_row("市盈率(动)", info["pe"], "", "")

    console.print(table)

    # 30日K线概要
    if "k30_high" in info:
        ktable = Table(title="近30日K线概要", box=box.SIMPLE, show_header=False,
                       width=64)
        ktable.add_column("指标", style="dim", width=12)
        ktable.add_column("数值", justify="right", width=20)
        ktable.add_column("指标", style="dim", width=12)
        ktable.add_column("数值", justify="right", width=20)

        ktable.add_row("30日最高", info["k30_high"],
                       "30日最低", info["k30_low"])
        ktable.add_row("30日涨幅", _color_change(info["k30_chg"]),
                       "上涨天数", f"{info['k30_rise_days']}/30")
        ktable.add_row("5日均价", info["k30_ma5"],
                       "10日均价", info["k30_ma10"])
        ktable.add_row("日均成交", info["k30_avg_vol"], "", "")

        console.print(ktable)
        console.print()


def render_hot(stocks: list[dict], board_name: str) -> None:
    """渲染涨幅榜"""
    if not stocks:
        console.print("[yellow]无数据[/yellow]")
        return

    table = Table(title=f"{board_name} 涨幅榜 Top{len(stocks)}",
                  box=box.SIMPLE, highlight=True)
    table.add_column("#", style="dim", width=4)
    table.add_column("代码", width=8)
    table.add_column("名称", width=10)
    table.add_column("最新价", justify="right", width=10)
    table.add_column("涨幅", justify="right", width=10)
    table.add_column("换手率", justify="right", width=8)
    table.add_column("成交额", justify="right", width=12)

    for i, s in enumerate(stocks, 1):
        table.add_row(
            str(i), s["code"], s["name"], s["price"],
            _color_change(s["change_pct"]),
            f"{s['turnover']}%", s["amount"],
        )

    console.print(table)


# --- 主逻辑 ---
def main():
    parser = argparse.ArgumentParser(description="A股实时行情查询")
    parser.add_argument("codes", nargs="?", help="股票代码，逗号分隔")
    parser.add_argument("--hot", action="store_true", help="显示涨幅榜")
    parser.add_argument("--detail", action="store_true", help="显示详细信息（含30日K线概要）")
    parser.add_argument("--board", choices=["all", "sha", "sza", "cyb", "kcb"],
                        default="all", help="板块 (all=全部, sha=沪A, sza=深A, cyb=创业板, kcb=科创板)")
    parser.add_argument("--top", type=int, default=20, help="涨幅榜显示数量 (默认20)")
    parser.add_argument("--sort", action="store_true", help="多只股票按涨幅排序")
    args = parser.parse_args()

    start = time.time()

    if args.hot:
        board_names = {"all": "全A股", "sha": "沪A", "sza": "深A", "cyb": "创业板", "kcb": "科创板"}
        stocks = fetch_hot_list(args.board, args.top)
        render_hot(stocks, board_names.get(args.board, args.board))
    elif args.codes:
        codes = [c.strip() for c in args.codes.split(",") if c.strip()]
        if not codes:
            console.print("[red]请输入有效股票代码[/red]")
            sys.exit(1)

        if args.detail and len(codes) == 1:
            console.print("[dim]正在获取详细信息...[/dim]")
            info = fetch_detail(codes[0])
            if info:
                render_detail(info)
            else:
                console.print(f"[yellow]未找到股票 {codes[0]}[/yellow]")
        else:
            stocks = []
            for code in codes:
                result = fetch_stock(code)
                if result:
                    stocks.append(result)
                else:
                    console.print(f"[yellow]未找到: {code}[/yellow]")

            if args.sort:
                stocks.sort(key=lambda x: float(x["change_pct"]) if x["change_pct"] != "-" else -999,
                            reverse=True)
            elif len(codes) > 1:
                code_order = {c: i for i, c in enumerate(codes)}
                stocks.sort(key=lambda x: code_order.get(x["code"], 999))

            if len(stocks) == 1 and not args.detail:
                render_single(stocks)
            elif stocks:
                render_hot(stocks, "自选股")
    else:
        console.print("[yellow]用法: python stock_watch.py <代码> [选项][/yellow]")
        console.print("[dim]  python stock_watch.py 601991            # 个股实时行情[/dim]")
        console.print("[dim]  python stock_watch.py 601991 --detail   # 含30日K线概要[/dim]")
        console.print("[dim]  python stock_watch.py 601991,600519     # 多只对比[/dim]")
        console.print("[dim]  python stock_watch.py --hot             # 全A涨幅榜[/dim]")
        console.print("[dim]  python stock_watch.py --hot --board cyb # 创业板涨幅榜[/dim]")
        console.print("[dim]  python stock_watch.py --hot --top 10    # 涨幅榜Top10[/dim]")
        sys.exit(0)

    elapsed = time.time() - start
    console.print(f"[dim]耗时 {elapsed:.2f}s[/dim]")


if __name__ == "__main__":
    main()
