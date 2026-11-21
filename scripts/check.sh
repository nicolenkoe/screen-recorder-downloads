#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
java -m jdk.compiler/com.sun.tools.javac.Main -d build/tests android/src/ru/screenrecorder/app/ReleasePolicy.java tests/ReleasePolicyTest.java
java -cp build/tests ReleasePolicyTest
java -m jdk.compiler/com.sun.tools.javac.Main -d build/tests android/src/ru/screenrecorder/app/PausePlan.java android/src/ru/screenrecorder/app/EditPlan.java android/src/ru/screenrecorder/app/AudioMix.java tests/AndroidEditTest.java
java -cp build/tests ru.screenrecorder.app.AndroidEditTest
java -m jdk.compiler/com.sun.tools.javac.Main -d build/tests android/src/ru/screenrecorder/app/RecordingControls.java tests/RecordingControlsTest.java
java -cp build/tests ru.screenrecorder.app.RecordingControlsTest
python3 - <<'PY'
from pathlib import Path
import xml.etree.ElementTree as ET
m=ET.parse('android/AndroidManifest.xml').getroot()
assert m.attrib['package']=='ru.screenrecorder.app'
s=Path('android/src/ru/screenrecorder/app/Updater.java').read_text()
for text in ['getLong("size")','SHA-256','getPackageArchiveInfo','getLongVersionCode','getApkContentsSigners','part.renameTo(apk)','if(!valid(a,apk,m))']:
 assert text in s,text
assert 'putBoolean("stable",true)' not in s
assert 'Updater.check(this,false)' in Path('android/src/ru/screenrecorder/app/MainActivity.java').read_text()
assert '.jks' not in '\n'.join(str(p) for p in Path('.').glob('android/**/*'))
print('PASS: manifest and update-validation integration points inspected (not runtime Android tests)')
PY
