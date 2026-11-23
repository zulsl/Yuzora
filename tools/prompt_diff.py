#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""逐条对比「Yuzora 实际发出的请求体」和「SillyTavern 实际发出的请求体」。

为什么需要这个工具：
    preset 数据一致、trace 顺序一致，仍然可能在正文层面差几个字符
    （宏、世界书格式、空标记、历史 trim……）。只看 token 统计看不出来，
    必须把两边真正发出去的 messages 数组摆在一起 diff。

取数方式（各一次即可）：
    ST 侧：把 tools/st_capture_snippet.js 粘进 ST 的浏览器控制台（F12），
           之后每发一条消息会下载 st_request_<时间>.json。
    我们侧：启动 Yuzora 前设 YUZORA_DUMP_REQUEST=1，发消息后得到 request_last.json。
           同时设 YUZORA_TRACE_PROMPT=1 还能得到 prompt_trace_last.json，
           用 --trace 可以把"多出来的那条"直接标成 3a59f94a 之类的 identifier。

用法：
    python tools/prompt_diff.py --ours request_last.json --st st_request_xxx.json
    python tools/prompt_diff.py --ours ... --st ... --trace prompt_trace_last.json --full
    python tools/prompt_diff.py --ours ... --st ... --params     # 顺带比采样参数
    python tools/prompt_diff.py --selftest

退出码：0 = messages 完全一致；1 = 有差异；2 = 参数/文件错误。
"""

import argparse
import difflib
import hashlib
import json
import os
import re
import sys

MACRO_RE = re.compile(r"\{\{[^{}]*\}\}")
VOLATILE_PARAMS = {
    "messages", "stream", "stream_options", "user", "chat_completion_source",
    "custom_url", "custom_include_headers", "custom_include_body",
    "custom_exclude_body", "custom_exclude_headers", "custom_prompt_post_processing",
    "reverse_proxy", "proxy_password", "proxy_preset",
}


def fail(msg):
    print("[错误] " + msg, file=sys.stderr)
    sys.exit(2)


def sha1(text):
    return hashlib.sha1(text.encode("utf-8")).hexdigest()


def load_payload(path):
    """返回 (原始对象/None, [{'role','name','content'}...])。"""
    if not os.path.exists(path):
        fail("文件不存在: %s" % path)
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError) as exc:
        fail("读取 JSON 失败: %s (%s)" % (path, exc))

    raw = data if isinstance(data, dict) else None
    if isinstance(data, dict):
        messages = data.get("messages")
        if messages is None and isinstance(data.get("body"), dict):
            messages = data["body"].get("messages")
        if messages is None and isinstance(data.get("messages"), list):
            messages = data["messages"]
        if messages is None:
            if "prompt" in data or "input" in data:
                fail("%s 是 Text Completion 请求（prompt 字符串），本工具只比 Chat Completion"
                     % path)
            fail("%s 里没有 messages 字段" % path)
    elif isinstance(data, list):
        messages = data
    else:
        fail("%s 既不是对象也不是数组" % path)

    if not isinstance(messages, list):
        fail("%s 的 messages 不是数组" % path)

    result = []
    for index, message in enumerate(messages):
        if not isinstance(message, dict):
            continue
        content = message.get("content", "")
        if isinstance(content, list):
            parts = []
            for part in content:
                if isinstance(part, dict) and isinstance(part.get("text"), str):
                    parts.append(part["text"])
                elif isinstance(part, str):
                    parts.append(part)
            content = "\n".join(parts)
        elif not isinstance(content, str):
            content = "" if content is None else str(content)
        result.append({
            "index": index,
            "role": message.get("role", ""),
            "name": message.get("name", "") or "",
            "content": content,
        })
    return raw, result


def message_key(message, ignore_name):
    return (
        message["role"],
        "" if ignore_name else message["name"],
        sha1(message["content"]),
    )


def classify_pair(ours, theirs, ignore_name):
    kinds = []
    if ours["role"] != theirs["role"]:
        kinds.append("role")
    if not ignore_name and ours["name"] != theirs["name"]:
        kinds.append("name")
    if ours["content"] != theirs["content"]:
        if (ours["content"].replace("\r\n", "\n") ==
                theirs["content"].replace("\r\n", "\n")):
            kinds.append("仅换行符")
        elif ours["content"].strip() == theirs["content"].strip():
            kinds.append("仅首尾空白")
        else:
            kinds.append("content")
    return kinds


def first_diff_context(a, b):
    n = min(len(a), len(b))
    i = 0
    while i < n and a[i] == b[i]:
        i += 1
    return i, a[max(0, i - 40):i + 80], b[max(0, i - 40):i + 80]


def unified(a, b, label_ours, label_st, limit=60):
    lines = list(difflib.unified_diff(
        a.splitlines(), b.splitlines(),
        fromfile=label_ours, tofile=label_st, lineterm=""))
    if len(lines) > limit:
        lines = lines[:limit] + ["…（diff 超过 %d 行，已截断；用 --full 看完整）" % limit]
    return lines


def compare(ours, theirs, ignore_name=False, full=False, trace_ids=None):
    """返回 findings 列表与分类计数。"""
    findings = []
    counts = {
        "identical": 0, "content": 0, "role": 0, "name": 0,
        "仅换行符": 0, "仅首尾空白": 0, "only-ours": 0, "only-st": 0,
    }

    matcher = difflib.SequenceMatcher(
        None,
        [message_key(m, ignore_name) for m in ours],
        [message_key(m, ignore_name) for m in theirs],
        autojunk=False,
    )

    def ident(side, index):
        if trace_ids and side == "ours" and 0 <= index < len(trace_ids):
            return trace_ids[index]
        return ""

    def add_pair(oi, si):
        a, b = ours[oi], theirs[si]
        kinds = classify_pair(a, b, ignore_name)
        if not kinds:
            counts["identical"] += 1
            return
        for kind in kinds:
            counts[kind] = counts.get(kind, 0) + 1
        pos, ctx_a, ctx_b = first_diff_context(a["content"], b["content"])
        finding = {
            "kind": "pair",
            "kinds": kinds,
            "ours_index": oi,
            "st_index": si,
            "identifier": ident("ours", oi),
            "ours": a,
            "st": b,
            "first_diff": pos,
            "ours_context": ctx_a,
            "st_context": ctx_b,
        }
        if "content" in kinds:
            finding["diff"] = unified(
                a["content"], b["content"],
                "ours[%d]%s" % (oi, " " + finding["identifier"] if finding["identifier"] else ""),
                "st[%d]" % si,
                limit=100000 if full else 60,
            )
        findings.append(finding)

    def add_only(side, index):
        counts["only-ours" if side == "ours" else "only-st"] += 1
        message = ours[index] if side == "ours" else theirs[index]
        findings.append({
            "kind": "only-ours" if side == "ours" else "only-st",
            "ours_index": index if side == "ours" else None,
            "st_index": index if side == "st" else None,
            "identifier": ident(side, index),
            "message": message,
        })

    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            counts["identical"] += i2 - i1
            continue
        if tag == "delete":
            for oi in range(i1, i2):
                add_only("ours", oi)
            continue
        if tag == "insert":
            for si in range(j1, j2):
                add_only("st", si)
            continue

        # replace：按顺序配对，多出来的算单边
        pair_count = min(i2 - i1, j2 - j1)
        for k in range(pair_count):
            add_pair(i1 + k, j1 + k)
        for oi in range(i1 + pair_count, i2):
            add_only("ours", oi)
        for si in range(j1 + pair_count, j2):
            add_only("st", si)

    return findings, counts


def is_clean(findings):
    return not findings


def print_findings(findings, counts, ours, theirs, trace_ids):
    print("消息数: ours=%d  ST=%d   trace=%s" %
          (len(ours), len(theirs), "%d 条" % len(trace_ids) if trace_ids else "无"))
    print("完全一致: %(identical)d  |  content 不同: %(content)d  |  role: %(role)d  |  "
          "name: %(name)d  |  仅换行: %(仅换行符)d  |  仅首尾空白: %(仅首尾空白)d  |  "
          "仅我们有: %(only-ours)d  |  仅 ST 有: %(only-st)d" % counts)
    print()

    for finding in findings:
        if finding["kind"].startswith("only-"):
            m = finding["message"]
            ident = " [" + finding["identifier"] + "]" if finding["identifier"] else ""
            label = "仅我们有" if finding["kind"] == "only-ours" else "仅 ST 有"
            index = finding["ours_index"] if finding["kind"] == "only-ours" else finding["st_index"]
            print("%s[%s#%d%s] %s %s 字: %s" % (
                label, "ours" if finding["kind"] == "only-ours" else "st",
                index, ident, m["role"], len(m["content"]),
                (m["content"][:120].replace("\n", "\\n") + "…")
                if len(m["content"]) > 120 else m["content"].replace("\n", "\\n")))
            print()
            continue

        a, b = finding["ours"], finding["st"]
        ident = " [" + finding["identifier"] + "]" if finding["identifier"] else ""
        print("[ours#%d%s] ↔ [st#%d]  %s" % (
            finding["ours_index"], ident, finding["st_index"],
            "/".join(finding["kinds"])))
        print("  role: ours=%s st=%s" % (a["role"], b["role"]))
        if a["name"] != b["name"]:
            print("  name: ours=%s st=%s" % (a["name"], b["name"]))
        print("  len : ours=%d st=%d  sha1 ours=%s st=%s" % (
            len(a["content"]), len(b["content"]),
            sha1(a["content"])[:12], sha1(b["content"])[:12]))
        if "content" in finding["kinds"]:
            print("  首个差异 @ %d" % finding["first_diff"])
            print("  ours: %r" % finding["ours_context"])
            print("  st  : %r" % finding["st_context"])
            for line in finding.get("diff", []):
                print("    " + line)
        print()


def compare_params(raw_ours, raw_st):
    if not isinstance(raw_ours, dict) or not isinstance(raw_st, dict):
        return
    print("=== 采样参数（仅参考：ST 的 body 还包含路由字段）===")
    keys = sorted(set(raw_ours) | set(raw_st))
    for key in keys:
        if key in VOLATILE_PARAMS:
            continue
        a = raw_ours.get(key, "<无>")
        b = raw_st.get(key, "<无>")
        if a != b:
            print("  %s: ours=%r  st=%r" % (key, a, b))
    print()


def macro_warnings(ours, theirs):
    ours_hits = sum(len(MACRO_RE.findall(m["content"])) for m in ours)
    st_hits = sum(len(MACRO_RE.findall(m["content"])) for m in theirs)
    if ours_hits or st_hits:
        print("[提示] 残留 {{...}} 宏：ours=%d 处，ST=%d 处（ST 对未知宏也会原样保留，"
              "这里只提示，不计入失败）" % (ours_hits, st_hits))
        print()


def selftest():
    def m(role, content, name=""):
        return {"index": 0, "role": role, "name": name, "content": content}

    cases = []

    ours = [m("system", "A"), m("user", "hi")]
    cases.append(("相同", ours, ours, [], {"identical": 2}))

    _, counts = compare([m("system", "A")], [m("system", "B")])
    cases.append(("正文不同", [m("system", "A")], [m("system", "B")], ["content"], {"content": 1}))

    _, counts = compare([m("system", "A"), m("system", "EXTRA")], [m("system", "A")])
    cases.append(("仅我们多", [m("system", "A"), m("system", "EXTRA")],
                  [m("system", "A")], ["only-ours"], {"only-ours": 1}))

    _, counts = compare([m("system", "A")], [m("system", "A"), m("system", "MISSING")])
    cases.append(("仅 ST 多", [m("system", "A")],
                  [m("system", "A"), m("system", "MISSING")], ["only-st"], {"only-st": 1}))

    _, counts = compare([m("system", "A")], [m("user", "A")])
    cases.append(("role 不同", [m("system", "A")], [m("user", "A")], ["role"], {"role": 1}))

    _, counts = compare([m("system", "A\r\nB")], [m("system", "A\nB")])
    cases.append(("仅换行", [m("system", "A\r\nB")], [m("system", "A\nB")],
                  ["仅换行符"], {"仅换行符": 1}))

    _, counts = compare([m("system", "A", "x")], [m("system", "A", "y")], ignore_name=True)
    cases.append(("忽略 name", [m("system", "A", "x")], [m("system", "A", "y")],
                  [], {"identical": 1}))

    failed = 0
    for name, a, b, expected_kinds, expected_counts in cases:
        findings, counts = compare(a, b, ignore_name=(name == "忽略 name"))
        got_kinds = []
        for f in findings:
            got_kinds.extend(f.get("kinds", [f["kind"]]))
        ok = sorted(got_kinds) == sorted(expected_kinds)
        for key, value in expected_counts.items():
            if counts.get(key, 0) != value:
                ok = False
        print("[selftest] %-10s %s  kinds=%s" % (name, "OK" if ok else "FAIL", got_kinds))
        if not ok:
            failed += 1

    if failed:
        print("[selftest] %d 个用例失败" % failed, file=sys.stderr)
        return 1
    print("[selftest] 全部 %d 个用例通过" % len(cases))
    return 0


def main():
    parser = argparse.ArgumentParser(description="逐条对比 Yuzora 与 SillyTavern 的请求体 messages")
    parser.add_argument("--ours", help="Yuzora 的 request_last.json（YUZORA_DUMP_REQUEST=1）")
    parser.add_argument("--st", help="ST 抓下来的 st_request_*.json（tools/st_capture_snippet.js）")
    parser.add_argument("--trace", default=None,
                        help="可选：YUZORA_TRACE_PROMPT=1 生成的 prompt_trace_last.json")
    parser.add_argument("--params", action="store_true", help="顺带对比非 messages 的采样参数")
    parser.add_argument("--full", action="store_true", help="正文 diff 不截断")
    parser.add_argument("--ignore-name", action="store_true", help="忽略 name 字段的差异")
    parser.add_argument("--selftest", action="store_true", help="只跑工具自身的用例")
    args = parser.parse_args()

    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

    if args.selftest:
        return selftest()

    if not args.ours or not args.st:
        parser.error("需要同时给 --ours 和 --st（或使用 --selftest）")

    raw_ours, ours = load_payload(args.ours)
    raw_st, theirs = load_payload(args.st)

    trace_ids = None
    if args.trace:
        if not os.path.exists(args.trace):
            fail("trace 文件不存在: %s" % args.trace)
        with open(args.trace, encoding="utf-8") as f:
            trace = json.load(f)
        trace_ids = [str(m.get("identifier") or "") for m in trace.get("messages") or []]
        if len(trace_ids) != len(ours):
            print("[提示] trace 有 %d 条消息，但 request_last.json 有 %d 条；"
                  "identifier 标注可能对不上" % (len(trace_ids), len(ours)))
            print()

    findings, counts = compare(
        ours, theirs,
        ignore_name=args.ignore_name,
        full=args.full,
        trace_ids=trace_ids,
    )

    print_findings(findings, counts, ours, theirs, trace_ids)
    macro_warnings(ours, theirs)
    if args.params:
        compare_params(raw_ours, raw_st)

    if is_clean(findings):
        print("OK：messages 逐条一致。")
        return 0

    print("发现 %d 处差异（%d 条消息不一致）。" %
          (len(findings), len(ours) - counts["identical"]))
    print("提示：多数差异来自数据漂移或空标记。先跑 "
          "tools/st_preset_diff.py --trace 和 tools/worldbook_diff.py 排除数据问题。")
    return 1


if __name__ == "__main__":
    sys.exit(main())
