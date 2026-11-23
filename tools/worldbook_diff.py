#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""对比「Yuzora 库里的世界书」和「SillyTavern 的世界书/角色卡内嵌书」。

典型事故（2026-11-18 汪汪队立大功）：
    角色卡 character_book 是 V2 格式（keys / position:"after_char" / insertion_order /
    extensions.depth），而导入代码只认 ST world 文件格式（key / 数字 position / order /
    depth）。结果：库里 51 条条目全部 position=0、keywords 全空，
    worldInfoAfter 永远为空，关键词条目永远不触发。

用法：
    # ST world 文件（data/default-user/worlds/*.json）
    python tools/worldbook_diff.py --db "%APPDATA%\\Yuzora\\Yuzora_data.db" \
        --book "汪汪队立大功" --st "path/to/worlds/汪汪队立大功.json"

    # 角色卡导出的 character_book JSON（{name, entries:[...]}）
    python tools/worldbook_diff.py --db ... --book ... --st card_book.json

    # 也接受 ST settings.json / characters 目录里的 .json（取 character_book）
    python tools/worldbook_diff.py --db ... --book ... --st character.json

退出码：0 = 无差异；1 = 有差异；2 = 参数/文件/数据库错误。
只读打开数据库，不会写任何东西。
"""

import argparse
import hashlib
import json
import os
import sqlite3
import sys


def fail(msg):
    print("[错误] " + msg, file=sys.stderr)
    sys.exit(2)


def sha1(text):
    return hashlib.sha1(text.encode("utf-8")).hexdigest()


def json_list(value):
    if value is None:
        return []
    if isinstance(value, list):
        return [str(x) for x in value]
    try:
        data = json.loads(value)
    except (TypeError, ValueError):
        return []
    return [str(x) for x in data] if isinstance(data, list) else []


def normalize_st_entry(entry):
    ext = entry.get("extensions") or {}

    keys = entry.get("key")
    if keys is None:
        keys = entry.get("keys")
    keys = [str(x) for x in (keys or [])]

    position = ext.get("position")
    if position is None:
        position = entry.get("position")
        if isinstance(position, str):
            # ST convertCharacterBook：只有 before_char 是 before，其余按 after
            position = 0 if position == "before_char" else 1
    if position is None:
        position = 0

    depth = ext.get("depth", entry.get("depth", 4))
    order = entry.get("order", entry.get("insertion_order", 100))
    if "disable" in entry:
        enabled = not bool(entry.get("disable"))
    else:
        enabled = bool(entry.get("enabled", True))

    return {
        "name": str(entry.get("comment") or entry.get("name") or "").strip(),
        "keys": sorted(keys),
        "position": int(position),
        "depth": int(depth if depth is not None else 4),
        "order": int(order if order is not None else 100),
        "constant": bool(entry.get("constant", False)),
        "enabled": enabled,
        "content": str(entry.get("content") or ""),
    }


def load_st(path):
    if not os.path.exists(path):
        fail("ST 文件不存在: %s" % path)
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError) as exc:
        fail("读取 ST 文件失败: %s" % exc)

    if isinstance(data, list):
        fail("这个文件是数组（多本世界书），请先拆成单本再比")

    # 角色卡：{ data: { character_book: ... } } 或 { character_book: ... } 或 book 本体
    book = None
    if isinstance(data.get("data"), dict) and "character_book" in data["data"]:
        book = data["data"]["character_book"]
    elif "character_book" in data:
        book = data["character_book"]
    elif "entries" in data or "originalData" in data:
        book = data
    else:
        fail("找不到 entries / character_book，看起来不是世界书或角色卡")

    original = book.get("originalData") or {}
    entries = book.get("entries")
    if isinstance(entries, dict):
        entries = list(entries.values())
    if not entries:
        entries = original.get("entries")
        if isinstance(entries, dict):
            entries = list(entries.values())
    if not entries:
        fail("找不到任何 entries")

    name = (original.get("name") or book.get("name") or
            os.path.splitext(os.path.basename(path))[0])
    return str(name), [normalize_st_entry(e) for e in entries if isinstance(e, dict)]


def load_db(path, book):
    if not os.path.exists(path):
        fail("数据库不存在: %s" % path)
    conn = sqlite3.connect("file:%s?mode=ro" % path, uri=True)
    conn.row_factory = sqlite3.Row
    row = None
    for candidate in conn.execute(
            "SELECT id, name FROM world_info_books WHERE id = ? OR name = ?", (book, book)):
        row = candidate
        break
    if row is None:
        fail("数据库里找不到世界书: %s" % book)

    entries = []
    for r in conn.execute(
            "SELECT name, keywords, content, is_enabled, constant, position, "
            "insert_depth, sort_order FROM world_info_entries WHERE book_id = ?",
            (row["id"],)):
        entries.append({
            "name": str(r["name"] or "").strip(),
            "keys": sorted(json_list(r["keywords"])),
            "position": int(r["position"] or 0),
            "depth": int(r["insert_depth"] or 0),
            "order": int(r["sort_order"] or 0),
            "constant": bool(r["constant"]),
            "enabled": bool(r["is_enabled"]),
            "content": str(r["content"] or ""),
        })
    return {"id": row["id"], "name": row["name"], "entries": entries}


def compare(db, st_entries):
    drifts = []
    db_map = {}
    for entry in db["entries"]:
        db_map.setdefault(entry["name"], []).append(entry)
    st_map = {}
    for entry in st_entries:
        st_map.setdefault(entry["name"], []).append(entry)

    only_db = sorted(set(db_map) - set(st_map))
    only_st = sorted(set(st_map) - set(db_map))
    if only_db:
        drifts.append("库里有、ST 没有的条目: %s" % ", ".join(only_db))
    if only_st:
        drifts.append("ST 有、库里没有的条目: %s" % ", ".join(only_st))

    for name in sorted(set(db_map) & set(st_map)):
        if len(db_map[name]) != 1 or len(st_map[name]) != 1:
            drifts.append("同名条目重复（%s）：库 %d 条 / ST %d 条"
                          % (name, len(db_map[name]), len(st_map[name])))
            continue
        a = db_map[name][0]
        b = st_map[name][0]
        if a["keys"] != b["keys"]:
            drifts.append("关键词不一致 (%s): 库=%s ST=%s"
                          % (name, a["keys"], b["keys"]))
        if a["position"] != b["position"]:
            drifts.append("position 不一致 (%s): 库=%d ST=%d（0=before 1=after 4=atDepth）"
                          % (name, a["position"], b["position"]))
        if a["depth"] != b["depth"]:
            drifts.append("depth 不一致 (%s): 库=%d ST=%d" % (name, a["depth"], b["depth"]))
        if a["order"] != b["order"]:
            drifts.append("order 不一致 (%s): 库=%d ST=%d" % (name, a["order"], b["order"]))
        if a["constant"] != b["constant"]:
            drifts.append("constant 不一致 (%s): 库=%s ST=%s"
                          % (name, a["constant"], b["constant"]))
        if a["enabled"] != b["enabled"]:
            drifts.append("enabled 不一致 (%s): 库=%s ST=%s"
                          % (name, a["enabled"], b["enabled"]))
        if a["content"] != b["content"]:
            drifts.append("正文不一致 (%s): 库 %d 字 sha1=%s / ST %d 字 sha1=%s"
                          % (name, len(a["content"]), sha1(a["content"])[:12],
                             len(b["content"]), sha1(b["content"])[:12]))
    return drifts


def main():
    parser = argparse.ArgumentParser(description="对比 Yuzora 库里的世界书与 ST 世界书/角色卡")
    parser.add_argument("--db", required=True, help="Yuzora_data.db 路径（只读打开）")
    parser.add_argument("--book", required=True, help="世界书名或 id")
    parser.add_argument("--st", required=True,
                        help="ST worlds/*.json，或含 character_book 的角色卡 JSON")
    args = parser.parse_args()

    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

    st_name, st_entries = load_st(args.st)
    db = load_db(args.db, args.book)

    print("库 : %s (id=%s)  条目=%d" % (db["name"], db["id"], len(db["entries"])))
    print("ST : %s  条目=%d" % (st_name, len(st_entries)))
    print()

    drifts = compare(db, st_entries)

    # 没差异时也提示一下关键字段的分布，方便眼睛扫一眼
    if not drifts:
        positions = {}
        keywords = 0
        for entry in db["entries"]:
            positions[entry["position"]] = positions.get(entry["position"], 0) + 1
            if entry["keys"]:
                keywords += 1
        print("position 分布: %s；有关键词的条目: %d"
              % (", ".join("%d=%d" % (k, v) for k, v in sorted(positions.items())), keywords))
        print("OK：没有发现差异。")
        return 0

    print("发现 %d 处差异：" % len(drifts))
    for drift in drifts:
        print("  - " + drift)
    print()
    print("提示：在应用里重新导入同名世界书会就地替换条目（保留 id，角色链接不会断）。")
    return 1


if __name__ == "__main__":
    sys.exit(main())
