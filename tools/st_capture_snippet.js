/*
 * 把 SillyTavern 实际发出去的 Chat Completion 请求体抓下来。
 *
 * 为什么需要它：
 *   ST 的请求体只存在于浏览器内存里（openai.js:3210 的 fetch），不会落盘；
 *   "和 ST 对齐"如果只看 Prompt Itemization 的 token 统计是看不出正文差异的。
 *   这个片段 hook 一次 fetch，每发一条消息就下载一个 st_request_*.json，
 *   再用 tools/prompt_diff.py 与 Yuzora 的 request_last.json 逐条比对。
 *
 * 用法：
 *   1. 打开 SillyTavern 页面（F12 → Console）；
 *   2. 整段粘贴执行，看到 "ST request capture enabled"；
 *   3. 正常发一条消息，浏览器会下载 st_request_<时间>.json；
 *   4. 让 Yuzora 用同样的角色 / 聊天状态 / 预设发一条（YUZORA_DUMP_REQUEST=1）；
 *   5. python tools/prompt_diff.py --ours request_last.json --st st_request_xxx.json \
 *          --trace prompt_trace_last.json
 *
 * 说明：
 *   - 只抓 /api/backends/chat-completions/generate（Chat Completion）。
 *     Text Completion 的 body 是 prompt 字符串，prompt_diff.py 不支持，会明确报错。
 *   - 页面刷新后 hook 失效，需要重新粘贴。
 *   - 下载被浏览器拦截时允许"多次下载"即可；也可以用 --download 之外的
 *     方式自己从 Network 面板复制，但请保持 body 原样。
 */
(function () {
    if (window.__yuzoraStCaptureInstalled) {
        console.log('ST request capture already enabled');
        return;
    }
    window.__yuzoraStCaptureInstalled = true;

    const TARGET = /\/api\/backends\/chat-completions\/generate/;
    const originalFetch = window.fetch.bind(window);

    function saveBody(text) {
        try {
            const stamp = new Date().toISOString().replace(/[:.]/g, '-');
            const blob = new Blob([text], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'st_request_' + stamp + '.json';
            document.body.appendChild(a);
            a.click();
            a.remove();
            setTimeout(() => URL.revokeObjectURL(url), 5000);
            console.log('[yuzora] ST request saved:', a.download, text.length, 'bytes');
        } catch (e) {
            console.warn('[yuzora] save failed, body follows', e, text);
        }
    }

    window.fetch = function (input, init) {
        try {
            const url = typeof input === 'string' ? input : (input && input.url) || '';
            if (TARGET.test(url) && init && typeof init.body === 'string') {
                saveBody(init.body);
            }
        } catch (e) {
            console.warn('[yuzora] capture hook error', e);
        }
        return originalFetch(input, init);
    };

    console.log('ST request capture enabled');
})();
