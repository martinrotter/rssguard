#!/usr/bin/env python3
"""Generate a JSON Feed containing one live article per Yahoo Finance symbol.

Example:
    python yahoo-stock-feed.py --stocks NVDA CEZ.PR --range 1y

When --stocks is omitted, the script uses DEFAULT_STOCKS below.

The script writes only JSON Feed data to standard output. Diagnostics are
written to standard error so that RSS Guard can safely consume the output.
"""

import argparse
import base64
import concurrent.futures
import html
import json
import math
import sys
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone


YAHOO_CHART_URL = "https://query1.finance.yahoo.com/v8/finance/chart/{}"
YAHOO_QUOTE_URL = "https://finance.yahoo.com/quote/{}"
USER_AGENT = (
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/136.0.0.0 Safari/537.36"
)
REQUEST_TIMEOUT_SECONDS = 15
MAX_WORKERS = 8
MAX_RESPONSE_BYTES = 20 * 1024 * 1024
VALID_RANGES = ("1d", "5d", "1mo", "3mo", "6mo", "1y", "2y", "5y", "10y", "ytd", "max")
DEFAULT_STOCKS = [
    # Information technology.
    "NVDA",
    "MSFT",
    "AAPL",
    "GOOGL",
    "ASML.AS",
    "SAP.DE",
    # Electrical, renewable energy and environmental technology.
    "CEZ.PR",
    "SU.PA",
    "NEE",
    "ENPH",
    "VWS.CO",
    "ORSTED.CO",
    # Agriculture and agricultural inputs.
    "DE",
    "ADM",
    "NTR",
    "MOS",
    # Financial services.
    "JPM",
    "BRK-B",
    "V",
    "ALV.DE",
]


class StockDataError(RuntimeError):
    """Raised when Yahoo does not return usable data for a symbol."""


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Generate a JSON Feed with one current article per Yahoo Finance stock."
    )
    parser.add_argument(
        "--stocks",
        nargs="+",
        default=DEFAULT_STOCKS,
        metavar="SYMBOL",
        help="Yahoo Finance symbols to watch; defaults to DEFAULT_STOCKS from the script.",
    )
    parser.add_argument(
        "--range",
        dest="history_range",
        choices=VALID_RANGES,
        default="1y",
        help="History used for the chart (default: 1y).",
    )
    return parser.parse_args()


def normalize_symbols(symbols):
    result = []
    seen = set()

    for raw_symbol in symbols:
        symbol = raw_symbol.strip().upper()

        if symbol and symbol not in seen:
            seen.add(symbol)
            result.append(symbol)

    return result


def finite_number(value):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return None

    number = float(value)
    return number if math.isfinite(number) else None


def first_available(*values):
    return next((value for value in values if value is not None), None)


def value_at(values, index):
    if not isinstance(values, list) or index >= len(values):
        return None

    return finite_number(values[index])


def download_stock(symbol, history_range):
    encoded_symbol = urllib.parse.quote(symbol, safe="")
    query = urllib.parse.urlencode(
        {
            "interval": "1d",
            "range": history_range,
            "includePrePost": "false",
            "events": "div,splits",
        }
    )
    request = urllib.request.Request(
        f"{YAHOO_CHART_URL.format(encoded_symbol)}?{query}",
        headers={"Accept": "application/json", "User-Agent": USER_AGENT},
    )

    try:
        with urllib.request.urlopen(request, timeout=REQUEST_TIMEOUT_SECONDS) as response:
            data = response.read(MAX_RESPONSE_BYTES + 1)
    except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError) as error:
        raise StockDataError(f"request failed: {error}") from error

    if len(data) > MAX_RESPONSE_BYTES:
        raise StockDataError("response is unexpectedly large")

    try:
        payload = json.loads(data)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise StockDataError(f"invalid JSON response: {error}") from error

    chart = payload.get("chart")

    if not isinstance(chart, dict):
        raise StockDataError("response does not contain chart data")

    if chart.get("error"):
        error = chart["error"]
        description = error.get("description") if isinstance(error, dict) else str(error)
        raise StockDataError(description or "Yahoo returned an unspecified error")

    results = chart.get("result")

    if not isinstance(results, list) or not results:
        raise StockDataError("response does not contain a chart result")

    return results[0]


def extract_bars(result):
    timestamps = result.get("timestamp") or []
    indicators = result.get("indicators") or {}
    quotes = indicators.get("quote") or []

    if not isinstance(timestamps, list) or not quotes or not isinstance(quotes[0], dict):
        return []

    quote = quotes[0]
    bars = []

    for index, raw_timestamp in enumerate(timestamps):
        timestamp = finite_number(raw_timestamp)
        close = value_at(quote.get("close"), index)

        if timestamp is None or close is None:
            continue

        bars.append(
            {
                "timestamp": int(timestamp),
                "open": value_at(quote.get("open"), index),
                "high": value_at(quote.get("high"), index),
                "low": value_at(quote.get("low"), index),
                "close": close,
                "volume": value_at(quote.get("volume"), index),
            }
        )

    return bars


def format_decimal(value, decimals, signed=False):
    if value is None:
        return "N/A"

    prefix = "+" if signed and value >= 0 else ""
    return f"{prefix}{value:,.{decimals}f}"


def iso_timestamp(timestamp):
    return datetime.fromtimestamp(timestamp, timezone.utc).isoformat().replace("+00:00", "Z")


def reduce_chart_points(bars, maximum=800):
    if len(bars) <= maximum:
        return bars

    # Keep the chart compact while retaining the first and last observations.
    last_index = len(bars) - 1
    indices = sorted({round(index * last_index / (maximum - 1)) for index in range(maximum)})
    return [bars[index] for index in indices]


def generate_chart_data_url(symbol, bars, decimals):
    points = reduce_chart_points(bars)

    if len(points) < 2:
        return ""

    width = 800
    height = 300
    left = 64
    right = 20
    top = 24
    bottom = 42
    plot_width = width - left - right
    plot_height = height - top - bottom
    timestamps = [point["timestamp"] for point in points]
    prices = [point["close"] for point in points]
    minimum = min(prices)
    maximum = max(prices)
    spread = maximum - minimum

    if spread == 0:
        spread = max(abs(maximum) * 0.02, 1.0)
        minimum -= spread / 2
        maximum += spread / 2

    padding = spread * 0.06
    minimum -= padding
    maximum += padding
    first_timestamp = timestamps[0]
    timestamp_span = max(timestamps[-1] - first_timestamp, 1)
    price_span = maximum - minimum

    def coordinates(point):
        x = left + ((point["timestamp"] - first_timestamp) / timestamp_span) * plot_width
        y = top + ((maximum - point["close"]) / price_span) * plot_height
        return x, y

    coordinates_list = [coordinates(point) for point in points]
    line_path = " ".join(
        ("M" if index == 0 else "L") + f" {x:.2f} {y:.2f}"
        for index, (x, y) in enumerate(coordinates_list)
    )
    area_path = (
        f"M {coordinates_list[0][0]:.2f} {top + plot_height:.2f} "
        + " ".join(f"L {x:.2f} {y:.2f}" for x, y in coordinates_list)
        + f" L {coordinates_list[-1][0]:.2f} {top + plot_height:.2f} Z"
    )

    if prices[-1] > prices[0]:
        color = "#168a45"
    elif prices[-1] < prices[0]:
        color = "#c5392f"
    else:
        color = "#64748b"

    grid_lines = []

    for index in range(5):
        fraction = index / 4
        y = top + fraction * plot_height
        value = maximum - fraction * price_span
        grid_lines.append(
            f'<line x1="{left}" y1="{y:.2f}" x2="{left + plot_width}" y2="{y:.2f}" '
            'stroke="#8490a3" stroke-opacity="0.28"/>'
        )
        grid_lines.append(
            f'<text x="{left - 8}" y="{y + 4:.2f}" text-anchor="end" '
            f'fill="#737d8c" font-size="11">{html.escape(format_decimal(value, decimals))}</text>'
        )

    first_date = datetime.fromtimestamp(timestamps[0], timezone.utc).strftime("%Y-%m-%d")
    last_date = datetime.fromtimestamp(timestamps[-1], timezone.utc).strftime("%Y-%m-%d")
    last_x, last_y = coordinates_list[-1]
    escaped_symbol = html.escape(symbol)
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}"
 viewBox="0 0 {width} {height}" role="img" aria-label="Price history for {escaped_symbol}">
<title>Price history for {escaped_symbol}</title>
<g font-family="sans-serif">
{''.join(grid_lines)}
<path d="{area_path}" fill="{color}" fill-opacity="0.12"/>
<path d="{line_path}" fill="none" stroke="{color}" stroke-width="2.5" stroke-linejoin="round"/>
<circle cx="{last_x:.2f}" cy="{last_y:.2f}" r="4" fill="{color}"/>
<text x="{left}" y="{height - 14}" fill="#737d8c" font-size="11">{first_date}</text>
<text x="{left + plot_width}" y="{height - 14}" text-anchor="end" fill="#737d8c" font-size="11">{last_date}</text>
</g>
</svg>'''
    encoded_svg = base64.b64encode(svg.encode("utf-8")).decode("ascii")
    return "data:image/svg+xml;base64," + encoded_svg


def html_value(label, value):
    return f"<b>{html.escape(label)}:</b> {html.escape(value)}"


def create_feed_item(requested_symbol, result):
    meta = result.get("meta") or {}
    bars = extract_bars(result)

    if not isinstance(meta, dict) or not bars:
        raise StockDataError("chart does not contain usable prices")

    symbol = str(meta.get("symbol") or requested_symbol).upper()
    name = str(meta.get("longName") or meta.get("shortName") or symbol)
    currency = str(meta.get("currency") or "")
    exchange = str(meta.get("fullExchangeName") or meta.get("exchangeName") or meta.get("exchange") or "")
    current_price = finite_number(meta.get("regularMarketPrice"))
    market_timestamp = finite_number(meta.get("regularMarketTime"))

    if current_price is None:
        current_price = bars[-1]["close"]

    if market_timestamp is None:
        market_timestamp = bars[-1]["timestamp"]

    market_timestamp = int(market_timestamp)
    previous_close = finite_number(meta.get("previousClose"))

    if previous_close is None and len(bars) >= 2:
        previous_close = bars[-2]["close"]

    decimals = meta.get("priceHint", 2)
    decimals = decimals if isinstance(decimals, int) and not isinstance(decimals, bool) else 2
    decimals = min(max(decimals, 0), 6)
    absolute_change = None
    percentage_change = None

    if previous_close not in (None, 0):
        absolute_change = current_price - previous_close
        percentage_change = absolute_change / previous_close * 100

    if absolute_change is None:
        title = f"{name} - {format_decimal(current_price, decimals)} {currency}".rstrip()
    else:
        direction = "\u25b2" if absolute_change > 0 else "\u25bc" if absolute_change < 0 else "\u25cf"
        title = (
            f"{name} - {direction} {format_decimal(percentage_change, 2, signed=True)}% "
            f"({format_decimal(absolute_change, decimals, signed=True)} {currency}) - "
            f"{format_decimal(current_price, decimals)} {currency}"
        ).rstrip()

    latest_bar = bars[-1]
    open_price = first_available(finite_number(meta.get("regularMarketOpen")), latest_bar["open"])
    high_price = first_available(finite_number(meta.get("regularMarketDayHigh")), latest_bar["high"])
    low_price = first_available(finite_number(meta.get("regularMarketDayLow")), latest_bar["low"])
    volume = first_available(finite_number(meta.get("regularMarketVolume")), latest_bar["volume"])
    chart_data_url = generate_chart_data_url(symbol, bars, decimals)
    article_url = YAHOO_QUOTE_URL.format(urllib.parse.quote(symbol, safe=""))
    separator = " &nbsp;|&nbsp; "
    price_text = f"{format_decimal(current_price, decimals)} {currency}".rstrip()
    previous_text = f"{format_decimal(previous_close, decimals)} {currency}".rstrip()
    change_text = (
        "N/A"
        if absolute_change is None
        else (
            f"{format_decimal(absolute_change, decimals, signed=True)} {currency} "
            f"({format_decimal(percentage_change, 2, signed=True)}%)"
        ).strip()
    )
    primary_summary = separator.join(
        [
            html_value("Price", price_text),
            html_value("Change", change_text),
            html_value("Previous", previous_text),
        ]
    )
    trading_summary = separator.join(
        [
            html_value("Open", f"{format_decimal(open_price, decimals)} {currency}".rstrip()),
            html_value("High", f"{format_decimal(high_price, decimals)} {currency}".rstrip()),
            html_value("Low", f"{format_decimal(low_price, decimals)} {currency}".rstrip()),
            html_value("Volume", format_decimal(volume, 0)),
        ]
    )
    chart_html = (
        f'<p><img src="{chart_data_url}" alt="Price history for {html.escape(symbol)}"></p>'
        if chart_data_url
        else ""
    )
    details = separator.join(
        [
            html_value("Exchange", exchange or "N/A"),
            html_value("Updated", iso_timestamp(market_timestamp)),
        ]
    )
    content_html = (
        f"<p>{primary_summary}<br>{trading_summary}</p>"
        f"{chart_html}"
        f'<p>{details}{separator}<a href="{article_url}">Open on Yahoo Finance</a></p>'
    )
    article_time = iso_timestamp(market_timestamp)

    return {
        "id": f"yahoo-stock:{symbol}",
        "url": article_url,
        "title": title,
        "content_html": content_html,
        "date_published": article_time,
        "date_modified": article_time,
        "authors": [{"name": "Yahoo Finance"}],
        "_rssguard": {
            "custom_data": json.dumps(
                {
                    "schema": 1,
                    "symbol": symbol,
                    "price": current_price,
                    "currency": currency,
                    "timestamp": market_timestamp,
                },
                separators=(",", ":"),
            )
        },
    }


def main():
    arguments = parse_arguments()
    symbols = normalize_symbols(arguments.stocks)

    if not symbols:
        print("No valid stock symbols were provided.", file=sys.stderr)
        return 2

    items = []
    worker_count = min(MAX_WORKERS, len(symbols))

    with concurrent.futures.ThreadPoolExecutor(max_workers=worker_count) as executor:
        futures = {
            executor.submit(download_stock, symbol, arguments.history_range): symbol for symbol in symbols
        }

        for future in concurrent.futures.as_completed(futures):
            symbol = futures[future]

            try:
                items.append(create_feed_item(symbol, future.result()))
            except (StockDataError, ValueError, TypeError, KeyError) as error:
                print(f"Could not process {symbol}: {error}", file=sys.stderr)

    if not items:
        print("No stock data could be downloaded.", file=sys.stderr)
        return 1

    feed = {
        "version": "https://jsonfeed.org/version/1.1",
        "title": "Yahoo Finance stocks",
        "home_page_url": "https://finance.yahoo.com/",
        "description": "Current market information for the configured Yahoo Finance symbols.",
        "items": items,
    }
    # ASCII escaping avoids locale-dependent stdout failures when RSS Guard
    # redirects a script on Windows. JSON consumers recover the original text.
    json.dump(feed, sys.stdout, ensure_ascii=True, separators=(",", ":"))
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:
        sys.exit(1)
