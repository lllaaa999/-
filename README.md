# 股票数据工具

全A股历史K线数据下载，支持东方财富、新浪财经、雪球三个数据源轮转采集。

## 文件结构

```
.
├── rotate_download.py      # 核心：多平台轮转下载（推荐）
├── fetch_stock_list.py     # 获取全A股代码列表
├── eastmoney_download.py   # 东方财富单源下载
├── sina_download.py        # 新浪财经单源下载
├── stock_watch.py          # 实时行情查询
├── .gitignore              # 忽略CSV数据文件
└── README.md
```

## 数据源

| 平台 | 速度 | 稳定性 | 数据字段 |
|------|------|--------|----------|
| 东方财富 | 极快 | 极易限流 | OHLCV + 成交额 + 换手率 + 振幅 |
| 新浪财经 | 快 | 中等 | OHLCV |
| 雪球 | 慢（需浏览器） | 稳定 | OHLCV |

## 快速开始

```bash
# 1. 获取全A股代码列表
python fetch_stock_list.py

# 2. 轮转下载K线（推荐，自动切换平台避免被封）
python rotate_download.py

# 3. 查实时行情
python stock_watch.py 600519
```

## 轮转策略

```
东方财富(15只) → 新浪(80只) → 雪球(10只) → 休息5秒 → 下一轮
```

每个平台只取少量就切换，避免触发任一平台的限流机制。
