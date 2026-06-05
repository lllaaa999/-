# 股票数据工具

全A股历史K线数据下载，支持东方财富、新浪财经、雪球三个数据源轮转采集，自动防封。

## 文件说明

| 文件 | 依赖 | 作用 |
|------|------|------|
| `rotate_download.py` | OpenCLI, 新浪API | **核心脚本**。三个平台轮流各取少量，切完一轮休息5秒再继续。避免任一平台触发限流。每轮汇报进度。 |
| `fetch_stock_list.py` | 新浪API | 从新浪 Market Center 分页拉取全A股代码列表，保存为 `stock_list.json`（约5524只）。 |
| `eastmoney_download.py` | OpenCLI | 单线程调用 `opencli eastmoney kline`，每只间隔2秒，检测到 `fetch failed` 自动停。 |
| `sina_download.py` | 新浪K线API | 单线程直连新浪历史K线接口（纯HTTP，无需浏览器），每50只输出进度。 |
| `stock_watch.py` | akshare, rich | 命令行实时行情工具，支持单只/多只查询、全市场涨幅榜、创业板/科创板排行、K线概要。 |
| `.gitignore` | — | 排除CSV数据文件、Python缓存、IDE配置，不上传大文件。 |

## 数据源

| 平台 | 走浏览器? | 速度 | 限流敏感度 | 数据字段 |
|------|-----------|------|------------|----------|
| 东方财富 | 是(OpenCLI→Chrome) | 快 | 极高 | OHLCV + 成交额 + 换手率 + 振幅 |
| 新浪财经 | 否(纯HTTP) | 快 | 中 | OHLCV |
| 雪球 | 是(OpenCLI→Chrome) | 慢 | 低 | OHLCV |

## 依赖

```bash
pip install akshare rich        # stock_watch.py 需要
npm install -g @jackwener/opencli  # 东方财富/雪球需要
# 还需要 Chrome + OpenCLI 扩展
```

## 使用

```bash
# 1. 获取全A股代码列表
python fetch_stock_list.py

# 2. 下载K线（推荐轮转模式）
python rotate_download.py

# 3. 实时行情
python stock_watch.py 600519              # 单只
python stock_watch.py 600519,000858       # 多只
python stock_watch.py --hot               # 全A涨幅榜Top20
python stock_watch.py --hot --board cyb   # 创业板涨幅榜
```

## 轮转策略

```
东方财富(15只) → 新浪(80只) → 雪球(10只) → 休息5秒 → 循环
```

每个平台只取少量就切换，谁都没反应过来就被换掉了。
