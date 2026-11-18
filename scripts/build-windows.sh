#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/windows
python3 -m pip install --target build/icon-tools cairosvg Pillow
export PYTHONPATH="$PWD/build/icon-tools${PYTHONPATH:+:$PYTHONPATH}"
python3 - <<'PY'
import urllib.request,hashlib,pathlib,zipfile
root=pathlib.Path('build/windows').resolve();pkg=root/'ffmpeg.whl'
if not pkg.exists():urllib.request.urlretrieve('https://files.pythonhosted.org/packages/2c/c6/fa760e12a2483469e2bf5058c5faff664acf66cadb4df2ad6205b016a73d/imageio_ffmpeg-0.6.0-py3-none-win_amd64.whl',pkg)
assert hashlib.sha256(pkg.read_bytes()).hexdigest()=='02fa47c83703c37df6bfe4896aab339013f62bf02c5ebf2dce6da56af04ffc0a'
with zipfile.ZipFile(pkg) as z:
 name=next(n for n in z.namelist() if n.endswith('.exe'));(root/'ffmpeg.exe').write_bytes(z.read(name))
import cairosvg
from PIL import Image
cairosvg.svg2png(url='site/icon.svg',write_to=str(root/'icon.png'),output_width=256,output_height=256)
Image.open(root/'icon.png').save(root/'app.ico',sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
(root/'bundle.rc').write_text('101 RCDATA "'+str(root/'ffmpeg.exe')+'"\n201 ICON "'+str(root/'app.ico')+'"\n#include "'+str(pathlib.Path('windows/version.rc').resolve())+'"\n')
PY
python3 -m pip install --target build/zig ziglang==0.13.0
export ZIG_GLOBAL_CACHE_DIR="$PWD/build/zig-global" ZIG_LOCAL_CACHE_DIR="$PWD/build/zig-local"
build/zig/ziglang/zig rc /fo build/windows/bundle.res -- build/windows/bundle.rc
build/zig/ziglang/zig cc -target x86_64-windows-gnu -Os -Wall -Wextra -Wno-unused-parameter -municode -Wl,--subsystem,windows windows/recorder.c windows/audio.c windows/region.c build/windows/bundle.res -o build/windows/ScreenRecorder-Windows-3.0.exe -luser32 -lgdi32 -lmsimg32 -lshell32 -lcomdlg32 -lcomctl32 -lmfplay -lmfplat -lmfuuid -lpropsys -lole32 -ladvapi32 -luuid
