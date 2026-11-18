#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
SDK_PATH="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-/workspace/work/android-tools/sdk}}"
BUILD_TOOLS="$SDK_PATH/build-tools/35.0.0"
BUILD_DIR="$PWD/build/android"
mkdir -p "$BUILD_DIR/classes" "$BUILD_DIR/dex"
"$BUILD_TOOLS/aapt2" compile --dir android/res -o "$BUILD_DIR/resources.zip"
"$BUILD_TOOLS/aapt2" link -o "$BUILD_DIR/base.apk" -I "$SDK_PATH/platforms/android-35/android.jar" --manifest android/AndroidManifest.xml "$BUILD_DIR/resources.zip"
find android/src -name '*.java' > "$BUILD_DIR/sources.txt"
java -m jdk.compiler/com.sun.tools.javac.Main -source 8 -target 8 -classpath "$SDK_PATH/platforms/android-35/android.jar" -d "$BUILD_DIR/classes" @"$BUILD_DIR/sources.txt"
java -m jdk.jartool/sun.tools.jar.Main --create --file "$BUILD_DIR/classes.jar" -C "$BUILD_DIR/classes" .
"$BUILD_TOOLS/d8" --min-api 29 --lib "$SDK_PATH/platforms/android-35/android.jar" --output "$BUILD_DIR/dex" "$BUILD_DIR/classes.jar"
cp "$BUILD_DIR/base.apk" "$BUILD_DIR/unsigned.apk"
python3 - "$BUILD_DIR" <<'PY'
import sys,pathlib,zipfile
p=pathlib.Path(sys.argv[1])
with zipfile.ZipFile(p/'unsigned.apk','a',zipfile.ZIP_DEFLATED) as z:
 for dex in (p/'dex').glob('*.dex'): z.write(dex,dex.name)
PY
"$BUILD_TOOLS/zipalign" -f 4 "$BUILD_DIR/unsigned.apk" "$BUILD_DIR/ScreenRecorder-unsigned.apk"
if [[ -n "${SIGNING_KEYSTORE:-}" ]]; then
 "$BUILD_TOOLS/apksigner" sign --ks "$SIGNING_KEYSTORE" --ks-key-alias recorder --ks-pass env:SIGNING_PASSWORD --key-pass env:SIGNING_PASSWORD --out "$BUILD_DIR/ScreenRecorder.apk" "$BUILD_DIR/ScreenRecorder-unsigned.apk"
 "$BUILD_TOOLS/apksigner" verify --verbose --print-certs "$BUILD_DIR/ScreenRecorder.apk"
fi
