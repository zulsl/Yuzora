#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""对比「Yuzora 库里的预设」和「SillyTavern 现在的预设文件」，找出会让请求体跑偏的数据漂移。

典型事故（2026-11-18 狐神抚）：
    用户 11-17 17:56 导入预设，11-18 01:22 又在 ST 里关掉了一条 19907 字的提示词。
    再导出时 ST 文件是最新的，我们库里还是旧的 → 请求体里白白多出一整块。

用法：
    python tools/st_preset_diff.py --db "%APPDATA%\\Yuzora\\Yuzora_data.db" \\
        --preset "狐神抚" --st "path/to/预设.json"

    # 顺带核对一次真实请求的装配顺序（YUZORA_TRACE_PROMPT=1 生成的文件）：
    python tools/st_preset_diff.py ... --trace prompt_trace_last.json

    # 机器可读输出：
    python tools/st_preset_diff.py ... --json

退出码：0 = 无漂移；1 = 有漂移；2 = 参数/文件/数据库错误。
只读打开数据库，不会写任何东西。
"""

import argparse
import datetime
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


def mtime_text(path):
    try:
        return datetime.datetime.fromtimestamp(os.path.getmtime(path)).strftime("%Y-%m-%d %H:%M:%S")
    except OSError:
        return "?"


def load_st(path):
    """读取 ST 预设文件；也接受 SillyTavern 的 settings.json（取 oai_settings）。"""
    if not os.path.exists(path):
        fail("ST 预设文件不存在: %s" % path)
    with open(path, encoding="utf-8") as f:
        data = json.load(f)

    # settings.json 的预设字段在 oai_settings 里
    if "oai_settings" in data and isinstance(data["oai_settings"], dict):
        data = data["oai_settings"]

    if "prompt_order" not in data or "prompts" not in data:
        fail("这个文件里没有 prompt_order / prompts，看起来不是 ST 预设或 settings.json")

    orders = [x for x in data["prompt_order"] if isinstance(x, dict)]
    if not orders:
        fail("prompt_order 为空")

    # ST 的 chat completion 用 dummyId=100001（public/scripts/openai.js:696-698）
    chosen = None
    for entry in orders:
        if str(entry.get("character_id")) == "100001":
            chosen = entry
            break
    if chosen is None:
        chosen = orders[0]

    order = [e.get("identifier") for e in chosen.get("order") or [] if isinstance(e, dict)]
    enabled = {e.get("identifier"): bool(e.get("enabled")) for e in chosen.get("order") or []
               if isinstance(e, dict)}
    prompts = {}
    names = {}
    for p in data.get("prompts") or []:
        if isinstance(p, dict) and p.get("identifier"):
            prompts[p["identifier"]] = p.get("content") or ""
            names[p["identifier"]] = p.get("name") or ""

    # world 文件/预设里的 new_chat_prompt：空串表示"不发标记"（openai.js:893-895 + getChat 丢弃空消息）
    return {
        "character_id": chosen.get("character_id"),
        "order": order,
        "enabled": enabled,
        "prompts": prompts,
        "names": names,
        "new_chat_prompt": data.get("new_chat_prompt"),
        "wi_format": data.get("wi_format"),
        "squash_system_messages": data.get("squash_system_messages"),
    }


def load_db(path, preset):
    if not os.path.exists(path):
        fail("数据库不存在: %s" % path)
    conn = sqlite3.connect("file:%s?mode=ro" % path, uri=True)
    conn.row_factory = sqlite3.Row
    row = None
    for candidate in conn.execute(
            "SELECT id, name, prompt_order, wi_Format, new_chat_prompt, "
            "new_example_chat_prompt, squash_system_messages, updated_at "
            "FROM completion_presets WHERE id = ? OR name = ?",
            (preset, preset)):
        row = candidate
        break
    if row is None:
        fail("数据库里找不到预设: %s" % preset)

    rows = conn.execute(
        "SELECT identifier, is_active, content, role, injection_position, name "
        "FROM completion_presets_prompts WHERE preset_id = ?", (row["id"],))
    prompts = {}
    active = {}
    names = {}
    for r in rows:
        ident = r["identifier"]
        prompts[ident] = r["content"] or ""
        active[ident] = bool(r["is_active"])
        names[ident] = r["name"] or ""

    # DB 的 NULL = 从未设置 → 用 ST 默认；显式空串是有效值（"不要标记"）
    new_chat = row["new_chat_prompt"]
    if new_chat is None:
        new_chat = "[Start a new Chat]"
    new_example = row["new_example_chat_prompt"]
    if new_example is None:
        new_example = "[Example Chat]"
    wi_format = row["wi_Format"]
    if wi_format is None:
        wi_format = "{0}"

    return {
        "id": row["id"],
        "name": row["name"],
        "order": json.loads(row["prompt_order"] or "[]"),
        "enabled": active,
        "prompts": prompts,
        "names": names,
        "new_chat_prompt": new_chat,
        "new_example_chat_prompt": new_example,
        "wi_format": wi_format,
        "squash_system_messages": bool(row["squash_system_messages"]),
        "updated_at": row["updated_at"],
    }


def first_divergence(a, b, label_a, label_b):
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i
    if len(a) != len(b):
        return min(len(a), len(b))
    return -1


def join_limited(items, limit=40):
    items = [str(x) for x in items]
    if len(items) <= limit:
        return ", ".join(items)
    return ", ".join(items[:limit]) + " …（共 %d 条）" % len(items)


def compare(db, st):
    drifts = []

    # 1) 顺序
    idx = first_divergence(db["order"], st["order"], "库", "ST")
    if idx >= 0:
        drifts.append("prompt_order 顺序不一致：第 %d 条起（库 %d 条 / ST %d 条）\n"
                      "    库: %s\n    ST: %s"
                      % (idx, len(db["order"]), len(st["order"]),
                         db["order"][idx] if idx < len(db["order"]) else "<无>",
                         st["order"][idx] if idx < len(st["order"]) else "<无>"))

    # 2) enabled / 内容（以 ST 的 order 为准遍历，再补上单边存在的条目）
    order = list(st["order"]) + [x for x in db["order"] if x not in set(st["order"])]
    for ident in order:
        in_db = ident in db["prompts"]
        in_st = ident in st["prompts"]
        if not in_db and not in_st:
            continue
        if not in_db:
            drifts.append("ST 有、库里没有的提示词: %s" % ident)
            continue
        if not in_st:
            drifts.append("库里有、ST 没有的提示词: %s" % ident)
            continue
        if db["enabled"].get(ident) != st["enabled"].get(ident):
            drifts.append("enabled 不一致 (%s): 库=%s ST=%s"
                          % (ident, db["enabled"].get(ident), st["enabled"].get(ident)))
        db_content = db["prompts"][ident]
        st_content = st["prompts"][ident]
        if db_content != st_content:
            drifts.append("内容不一致 (%s): 库 %d 字 sha1=%s / ST %d 字 sha1=%s"
                          % (ident, len(db_content), sha1(db_content)[:12],
                             len(st_content), sha1(st_content)[:12]))

    # 3) 与装配直接相关的预设字段（这些不是提示词，但会改变请求体）
    field_labels = {
        "new_chat_prompt": "new_chat_prompt",
        "new_example_chat_prompt": "new_example_chat_prompt",
        "wi_format": "wi_format",
    }
    for key, label in field_labels.items():
        st_value = st.get(key)
        if st_value is None:
            continue
        if db[key] != st_value:
            drifts.append("%s 不一致：库=%r ST=%r" % (label, db[key], st_value))

    return drifts


def collapse_runtime_ids(identifiers):
    """把 trace 里的运行时展开还原成"预设里的组"：
        chatHistory-3 / newMainChat   → chatHistory
        dialogueExamples-0-1 / newChat → dialogueExamples
        controlPrompts / continueNudge → 不参与顺序比较
    """
    collapsed = []
    for ident in identifiers:
        if ident.startswith("chatHistory-"):
            parent = "chatHistory"
        elif ident.startswith("dialogueExamples-"):
            parent = "dialogueExamples"
        elif ident == "newMainChat":
            parent = "chatHistory"
        elif ident == "newChat":
            parent = "dialogueExamples"
        elif ident in ("controlPrompts", "continueNudge", "emptyUserMessageReplacement"):
            continue
        else:
            parent = ident
        if not collapsed or collapsed[-1] != parent:
            collapsed.append(parent)
    return collapsed


def compare_trace(trace_path, st, db):
    """核对实际装配：禁用条目是否被发送、顺序、空标记、世界书包裹、残留宏。"""
    if not os.path.exists(trace_path):
        fail("trace 文件不存在: %s" % trace_path)
    with open(trace_path, encoding="utf-8") as f:
        trace = json.load(f)

    entries = {str(e.get("identifier") or ""): e for e in trace.get("entries") or []}
    messages = trace.get("messages") or []
    actual = [str(m.get("identifier") or "") for m in messages]

    drifts = []
    warnings = []

    # 1) 禁用的提示词绝不能出现在请求里（狐神抚 3a59f94a 就是这一条）
    disabled = {ident for ident, on in st["enabled"].items() if not on}
    bad = [ident for ident in actual if ident in disabled]
    if bad:
        details = []
        for ident in dict.fromkeys(bad):  # 去重、保序
            name = (entries.get(ident, {}).get("name")
                    or st.get("names", {}).get(ident) or "")
            details.append("%s%s" % (ident, " (%s)" % name if name else ""))
        drifts.append("禁用的提示词被发送（%d 条）: %s"
                      % (len(details), ", ".join(details)))

    # 2) 顺序：只比较"两边都出现"的组，避免空 prompt（宏渲染后为空）造成假差异
    actual_collapsed = collapse_runtime_ids(actual)
    expected = [ident for ident in st["order"] if st["enabled"].get(ident)]
    actual_set = set(actual_collapsed)
    expected_set = set(expected)
    actual_common = [ident for ident in actual_collapsed if ident in expected_set]
    expected_common = [ident for ident in expected if ident in actual_set]
    idx = first_divergence(actual_common, expected_common, "实际", "ST 期望")
    if idx >= 0:
        drifts.append("消息顺序与 ST 期望不一致：第 %d 条起\n    实际: %s\n    ST : %s\n"
                      "    全部实际: %s\n    ST 期望: %s"
                      % (idx,
                         actual_common[idx] if idx < len(actual_common) else "<无>",
                         expected_common[idx] if idx < len(expected_common) else "<无>",
                         join_limited(actual_collapsed),
                         join_limited(expected)))

    unknown = [ident for ident in actual_collapsed
               if ident not in st["enabled"] and ident not in st["prompts"]]
    if unknown:
        drifts.append("请求里出现 ST 预设里不存在的提示词（identifier 对不上）: %s"
                      % join_limited(unknown))

    missing = [ident for ident in expected if ident not in actual_set]
    if missing:
        warnings.append("启用但未出现在请求里的提示词（宏渲染后为空/锚点无内容属正常）: %s"
                        % join_limited(missing))

    # 3) New Chat 标记：DB 里空串 ⇔ 不发；非空 ⇔ 恰好一条
    expects_new_chat = bool(db["new_chat_prompt"])
    count_new_chat = actual.count("newMainChat")
    if expects_new_chat and count_new_chat != 1:
        drifts.append("new_chat_prompt 非空，但请求里 newMainChat 出现 %d 次（应为 1）"
                      % count_new_chat)
    if not expects_new_chat and count_new_chat != 0:
        drifts.append("new_chat_prompt 为空（显式 \"\"），但请求里仍有 newMainChat —— "
                      "ST 会丢弃空消息，这里不应出现")

    # 4) New Example Chat 标记：空串时不能有 newChat；非空时每个示例块一条
    expects_example = bool(db["new_example_chat_prompt"])
    count_example = actual.count("newChat")
    blocks = {ident.split("-")[1] for ident in actual if ident.startswith("dialogueExamples-")}
    if not expects_example and count_example != 0:
        drifts.append("new_example_chat_prompt 为空（显式 \"\"），但请求里仍有 %d 条 newChat 标记"
                      % count_example)
    if expects_example and blocks and count_example != len(blocks):
        drifts.append("new_example_chat_prompt 非空：示例块 %d 个 / newChat 标记 %d 条"
                      % (len(blocks), count_example))

    # 5) wi_format：非空且不是 "{0}" 时，worldInfoBefore/After 必须被前后缀包住
    wi_format = db["wi_format"] or ""
    if wi_format and wi_format != "{0}":
        prefix, sep, suffix = wi_format.partition("{0}")
        for message in messages:
            ident = str(message.get("identifier") or "")
            if ident not in ("worldInfoBefore", "worldInfoAfter"):
                continue
            head = str(message.get("head") or "")
            tail = str(message.get("tail") or "")
            if sep == "":
                if not head.startswith(wi_format[:min(len(wi_format), 120)]):
                    drifts.append("%s 没有使用 wi_format（ST 会把内容替换成格式串本身）" % ident)
            else:
                if not head.startswith(prefix[:min(len(prefix), 120)]):
                    drifts.append("%s 开头缺少 wi_format 前缀 %r" % (ident, prefix))
                if not tail.endswith(suffix[-120:]):
                    drifts.append("%s 结尾缺少 wi_format 后缀 %r" % (ident, suffix))

    # 6) 残留宏：ST 对未知宏也会原样保留，所以只提示
    macros = [m for m in messages if m.get("containsMacro")]
    if macros:
        warnings.append("仍有未替换的 {{...}}（%d 条，ST 对未知宏也会保留，需人工确认）: %s"
                        % (len(macros), ", ".join(str(m.get("index")) for m in macros)))

    return drifts, warnings


def main():
    parser = argparse.ArgumentParser(description="对比 Yuzora 库里的预设与 SillyTavern 预设文件")
    parser.add_argument("--db", required=True, help="Yuzora_data.db 路径（只读打开）")
    parser.add_argument("--preset", required=True, help="预设名或 id")
    parser.add_argument("--st", required=True, help="ST 预设 JSON（或 settings.json）")
    parser.add_argument("--trace", default=None,
                        help="可选：YUZORA_TRACE_PROMPT=1 生成的 prompt_trace_last.json")
    parser.add_argument("--json", action="store_true", help="输出机器可读 JSON")
    args = parser.parse_args()

    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

    st = load_st(args.st)
    st["file_mtime"] = mtime_text(args.st)
    db = load_db(args.db, args.preset)

    drifts = compare(db, st)
    warnings = []
    if args.trace:
        trace_drifts, trace_warnings = compare_trace(args.trace, st, db)
        drifts.extend(trace_drifts)
        warnings.extend(trace_warnings)

    if args.json:
        print(json.dumps({
            "ok": not drifts,
            "db": {"name": db["name"], "id": db["id"], "updated_at": db["updated_at"]},
            "st": {"file": args.st, "mtime": st["file_mtime"]},
            "drifts": drifts,
            "warnings": warnings,
        }, ensure_ascii=False, indent=2))
        return 0 if not drifts else 1

    print("库 : %s (id=%s)  order=%d  enabled=%d  导入时间=%s"
          % (db["name"], db["id"], len(db["order"]), sum(db["enabled"].values()),
             db["updated_at"] or "?"))
    print("ST : character_id=%s  order=%d  enabled=%d  文件时间=%s"
          % (st["character_id"], len(st["order"]), sum(st["enabled"].values()),
             st["file_mtime"]))
    print()

    if not drifts:
        print("OK：没有发现漂移。")
    else:
        print("发现 %d 处漂移：" % len(drifts))
        for drift in drifts:
            print("  - " + drift)

    for warning in warnings:
        print("  [提示] " + warning)

    if drifts:
        print()
        print("提示：在应用里重新导入同一个预设文件即可覆盖更新（同名导入会就地更新，不再新建副本）。")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
