#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import shlex
from pathlib import Path


XLIB_SOURCES = [
    "Alloc.c",
    "XlibInt.c",
    "StrKeysym.c",
    "ImUtil.c",
    "OpenDis.c",
    "Region.c",
    "Window.c",
    "Prop.c",
    "Selection.c",
    "BrowserBind.c",
    "WindowOps.c",
    "Draw.c",
    "Im.c",
    "Event.c",
    "Test.c",
    "ICCCM.c",
    "Runtime.c",
]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        default="build/compile_commands.json",
        help="Path to the compile_commands.json file to write.",
    )
    return parser.parse_args()


def emscripten_tooling_flags(root: Path) -> list[str]:
    sysroot = root / "build/emscripten/em-cache/sysroot"
    flags = ["--target=wasm32-unknown-emscripten"]
    if sysroot.exists():
        flags.append(f"--sysroot={sysroot}")
    return flags


def compile_entry(
    root: Path,
    compiler: str,
    source: Path,
    output: Path,
    includes: list[Path],
    tooling_flags: list[str],
    defines: list[str],
) -> dict[str, object]:
    args = [
        compiler,
        *tooling_flags,
        "-c",
        *(f"-I{path}" for path in includes),
        *defines,
        str(source),
        "-o",
        str(output),
    ]
    return {
        "directory": str(root),
        "arguments": args,
        "command": shlex.join(args),
        "file": str(source),
    }


def xlib_entries(
    root: Path,
    compiler: str,
    tooling_flags: list[str],
) -> list[dict[str, object]]:
    source_dir = root / "src/libX11"
    build_dir = root / "build/emscripten"
    includes = [root / "include"]
    defines = ["-DSTATIC_BUILD=1"]
    entries: list[dict[str, object]] = []

    for source_name in XLIB_SOURCES:
        source = source_dir / source_name
        output = build_dir / f"{Path(source_name).stem}.o"
        entries.append(
            compile_entry(
                root,
                compiler,
                source,
                output,
                includes,
                tooling_flags,
                defines,
            )
        )
    return entries


def browser_app_entries(
    root: Path,
    compiler: str,
    tooling_flags: list[str],
) -> list[dict[str, object]]:
    return [
        compile_entry(
            root,
            compiler,
            root / "test/browser/main.c",
            root / "build/emscripten/browser-main.o",
            [root / "include"],
            tooling_flags,
            ["-DSTATIC_BUILD=1"],
        )
    ]


def write_compile_commands(output: Path, entries: list[dict[str, object]]) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    tmp = output.with_suffix(output.suffix + ".tmp")
    tmp.write_text(json.dumps(entries, indent=2) + "\n")
    os.replace(tmp, output)
    print(f"wrote {output} ({len(entries)} entries)")


def main():
    args = parse_args()
    root = Path.cwd().resolve()
    output = (root / args.output).resolve()
    compiler = shutil.which("emcc") or "emcc"
    tooling_flags = emscripten_tooling_flags(root)
    entries = [
        *xlib_entries(root, compiler, tooling_flags),
        *browser_app_entries(root, compiler, tooling_flags),
    ]
    write_compile_commands(output, entries)


if __name__ == "__main__":
    main()
