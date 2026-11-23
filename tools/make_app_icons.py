"""从 AppIconKitchen 的 1024 原图生成 Yuzora 各平台图标。

只做两件事（都不改源素材）：
1. 生成"圆角"版本：Windows 桌面图标不会自己加圆角，必须画进像素里；
2. 生成 Windows 需要的 .ico（多尺寸）和 QIcon 用的多尺寸 .png，并拷好 Android mipmap。

用法：
    python tools/make_app_icons.py --src /path/to/app-icons
    python tools/make_app_icons.py --src ... --radius 0.18   # 想更方一点
"""

from __future__ import annotations

import argparse
import io
import struct
from pathlib import Path

from PIL import Image, ImageDraw

# ---------------------------------------------------------------- 默认路径
# 源素材不在仓库里（1024 原图 + 工作文件），必须显式传 --src，别写死某台机器的路径
DEFAULT_SRC = None
DEFAULT_PROJECT = Path(__file__).resolve().parents[1]

# QIcon 会按这些尺寸各取一张图（DPI 缩放时不必从 256 硬降采样）
PNG_SIZES = (16, 20, 24, 32, 40, 48, 64, 96, 128, 256)
# Windows .ico 里打包的尺寸；256 是资源管理器"超大图标"用的
ICO_SIZES = (16, 20, 24, 32, 40, 48, 64, 96, 128, 256)
# Android 传统 mipmap 密度 -> 边长
ANDROID_MIPMAPS = {
    "mipmap-mdpi": 48,
    "mipmap-hdpi": 72,
    "mipmap-xhdpi": 96,
    "mipmap-xxhdpi": 144,
    "mipmap-xxxhdpi": 192,
}


def rounded_icon(master: Image.Image, size: int, radius_ratio: float) -> Image.Image:
    """把整幅方图裁成圆角方块（外部透明）。

    遮罩按 4 倍超采样再缩回来，避免 16px 小图上圆角出现锯齿。
    注意：只替换 alpha 通道（putalpha），不能用 paste(..., mask) ——
    那会把半透明边缘的 RGB 也按遮罩往黑色压，浅色背景上就是一圈黑边。
    """
    art = master.resize((size, size), Image.LANCZOS)
    ss = 4
    m = size * ss
    mask = Image.new("L", (m, m), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (0, 0, m - 1, m - 1), radius=radius_ratio * m, fill=255
    )
    mask = mask.resize((size, size), Image.LANCZOS)

    out = art.copy()
    out.putalpha(mask)
    return out


def circular_icon(master: Image.Image, size: int) -> Image.Image:
    art = master.resize((size, size), Image.LANCZOS)
    ss = 4
    m = size * ss
    mask = Image.new("L", (m, m), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, m - 1, m - 1), fill=255)
    mask = mask.resize((size, size), Image.LANCZOS)

    out = art.copy()
    out.putalpha(mask)
    return out


def save_ico(path: Path, master: Image.Image, radius_ratio: float) -> None:
    """手写 .ico 容器：每个尺寸都用自己的圆角遮罩，不让 Pillow 从某一张缩小。

    Pillow 的 ICO 保存器只能从一个基准图缩放，会把 256 也缩成糊的，
    所以这里按规范拼 ICONDIR + ICONDIRENTRY + PNG 数据（Vista 起支持）。
    """
    blobs: list[tuple[int, bytes]] = []
    for size in ICO_SIZES:
        buf = io.BytesIO()
        rounded_icon(master, size, radius_ratio).save(
            buf, format="PNG", optimize=True, compress_level=9
        )
        blobs.append((size, buf.getvalue()))

    header = struct.pack("<HHH", 0, 1, len(blobs))  # reserved, type=icon, count
    offset = 6 + 16 * len(blobs)
    entries = b""
    payload = b""
    for size, blob in blobs:
        dim = 0 if size >= 256 else size  # 256 在目录项里记作 0
        entries += struct.pack(
            "<BBBBHHII", dim, dim, 0, 0, 1, 32, len(blob), offset
        )
        payload += blob
        offset += len(blob)

    path.write_bytes(header + entries + payload)


def convert(src: Path | None, project: Path, radius: float) -> None:
    src_dir = src or DEFAULT_SRC
    if src_dir is None:
        raise SystemExit("缺少 --src：指向 AppIconKitchen 输出目录（里面有 original/icon-1024.png）")
    master = Image.open(src_dir / "original" / "icon-1024.png").convert("RGBA")

    app_dir = project / "res" / "app"
    app_dir.mkdir(parents=True, exist_ok=True)

    for size in PNG_SIZES:
        rounded_icon(master, size, radius).save(app_dir / f"yuzora-{size}.png", optimize=True)
    rounded_icon(master, 512, radius).save(app_dir / "yuzora-512.png", optimize=True)
    save_ico(app_dir / "yuzora.ico", master, radius)

    # Android：ic_launcher 用圆角方块，ic_launcher_round 用正圆
    android_res = project / "android" / "res"
    for folder, size in ANDROID_MIPMAPS.items():
        out_dir = android_res / folder
        out_dir.mkdir(parents=True, exist_ok=True)
        rounded_icon(master, size, radius).save(out_dir / "ic_launcher.png", optimize=True)
        circular_icon(master, size).save(out_dir / "ic_launcher_round.png", optimize=True)

    print(f"source : {src_dir / 'original' / 'icon-1024.png'}")
    print(f"radius : {radius:.0%}")
    print(f"pngs   : {len(PNG_SIZES) + 1} in {app_dir}")
    print(f"ico    : {app_dir / 'yuzora.ico'}")
    print(f"android: {len(ANDROID_MIPMAPS)} mipmap dirs under {android_res}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--src", type=Path, help="AppIconKitchen 输出目录（含 original/）")
    parser.add_argument("--project", type=Path, default=DEFAULT_PROJECT, help="Yuzora 仓库根目录")
    parser.add_argument(
        "--radius",
        type=float,
        default=0.22,
        help="圆角半径 / 边长，默认 0.22（近似 macOS/iOS 观感）",
    )
    args = parser.parse_args()
    convert(args.src, args.project, args.radius)


if __name__ == "__main__":
    main()
