name: X360 Web Total 0.4.0 - Build and Publish

on:
  workflow_dispatch:
  push:
    tags:
      - 'v*'

permissions:
  contents: write

concurrency:
  group: x360-web-total-release-${{ github.ref }}
  cancel-in-progress: false

env:
  SOURCE_ARCHIVE: X360-Web-Total-0.4.0-source.zip
  LIBXENON_IMAGE: free60/libxenon@sha256:3534d7952210ee5553790a1c1f12001145655e28f9e12a01ce151efe4b4f743d
  XELL_LAUNCH2_URL: https://github.com/mitchellwaite/XellLaunch2/releases/download/v2.2.0/XellLaunch2.xex
  XELL_LAUNCH2_SHA256: 49dd8666b83a7c7b37597affc46513fe2cb9ab5cb2fe1fb71da1b13175276bb0

jobs:
  build:
    name: Verify source, test, compile and package Xbox 360
    runs-on: ubuntu-24.04
    timeout-minutes: 60

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4
        with:
          persist-credentials: false

      - name: Extract the uploaded 0.4.0 source archive
        shell: bash
        run: |
          set -Eeuo pipefail
          archive="$GITHUB_WORKSPACE/$SOURCE_ARCHIVE"
          test -s "$archive" || {
            echo "SOURCE ERROR: $SOURCE_ARCHIVE is missing from the repository root."
            exit 1
          }
          temp="$RUNNER_TEMP/x360-web-total-source"
          rm -rf "$temp"
          mkdir -p "$temp"
          python3 - "$archive" "$temp" <<'PY'
          import sys
          import zipfile
          from pathlib import PurePosixPath

          archive_path, destination = sys.argv[1:]
          with zipfile.ZipFile(archive_path) as z:
              entries = z.infolist()
              if not entries or len(entries) > 2000:
                  raise SystemExit('SOURCE ERROR: ZIP is empty or contains too many entries.')
              total = 0
              for entry in entries:
                  name = entry.filename
                  path = PurePosixPath(name)
                  if path.is_absolute() or not path.parts or '..' in path.parts or '\\' in name:
                      raise SystemExit(f'SOURCE ERROR: unsafe ZIP path: {name}')
                  total += entry.file_size
                  if entry.file_size > 16 * 1024 * 1024 or total > 64 * 1024 * 1024:
                      raise SystemExit('SOURCE ERROR: source archive exceeds size limits.')
              bad = z.testzip()
              if bad:
                  raise SystemExit(f'SOURCE ERROR: ZIP CRC error: {bad}')
              z.extractall(destination)
          PY
          project="$temp/X360-Web-Total-0.4.0"
          test -d "$project" || {
            echo 'SOURCE ERROR: expected top-level folder X360-Web-Total-0.4.0/ inside archive.'
            find "$temp" -maxdepth 2 -type f -print
            exit 1
          }
          test "$(tr -d '\r\n' < "$project/VERSION")" = '0.4.0'
          echo "XW_PROJECT_DIR=$project" >> "$GITHUB_ENV"
          echo "Extracted source: $project"
          find "$project" -maxdepth 2 -type f | sort | head -80

      - name: Verify project structure
        shell: bash
        run: |
          set -Eeuo pipefail
          test -f "$XW_PROJECT_DIR/VERSION"
          test -f "$XW_PROJECT_DIR/browser/Makefile"
          test -f "$XW_PROJECT_DIR/browser/CMakeLists.txt"
          test -f "$XW_PROJECT_DIR/scripts/package_usb.py"
          test -f "$XW_PROJECT_DIR/scripts/test_package_usb.py"
          test -f "$XW_PROJECT_DIR/README-INSTALL-PT-BR.txt"
          test -f "$XW_PROJECT_DIR/docs/BOOT-FLOW.md"
          test -f "$XW_PROJECT_DIR/usb/X360WebTotal/config/browser.ini"
          test -f "$XW_PROJECT_DIR/usb/X360WebTotal/ui/home.html"
          echo "Project structure verified. Version: $(cat "$XW_PROJECT_DIR/VERSION")"

      - name: Run package validator tests
        run: python3 "$XW_PROJECT_DIR/scripts/test_package_usb.py"

      - name: Install host build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y --no-install-recommends build-essential cmake gcc

      - name: Build and test portable C modules
        shell: bash
        run: |
          set -Eeuo pipefail
          cmake -S "$XW_PROJECT_DIR/browser" -B "$RUNNER_TEMP/xw-host-build" \
            -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
          cmake --build "$RUNNER_TEMP/xw-host-build" --parallel 2
          ctest --test-dir "$RUNNER_TEMP/xw-host-build" --output-on-failure --no-tests=error

      - name: Pull pinned LibXenon toolchain
        run: docker pull "$LIBXENON_IMAGE"

      - name: Compile Xbox 360 PowerPC ELF
        shell: bash
        run: |
          set -Eeuo pipefail
          docker run --rm \
            --volume "$XW_PROJECT_DIR/browser:/workspace/browser" \
            --workdir /workspace/browser \
            --entrypoint /bin/bash \
            "$LIBXENON_IMAGE" \
            -lc 'set -Eeuo pipefail; export DEVKITXENON=/usr/local/xenon; export PATH="$DEVKITXENON/bin:$DEVKITXENON/usr/bin:$PATH"; make clean all DEVKITXENON="$DEVKITXENON"'
          test -s "$XW_PROJECT_DIR/browser/xenon.elf"
          python3 - "$XW_PROJECT_DIR" <<'PY'
          import sys
          from pathlib import Path
          project = Path(sys.argv[1])
          sys.path.insert(0, str(project / 'scripts'))
          import package_usb
          elf = project / 'browser' / 'xenon.elf'
          package_usb.validate_elf(elf)
          print(f'Validated Xbox 360 PowerPC ELF: {elf.stat().st_size} bytes')
          PY

      - name: Download and verify XellLaunch2
        shell: bash
        run: |
          set -Eeuo pipefail
          curl --fail --show-error --location --retry 4 --retry-all-errors \
            --connect-timeout 20 --max-time 180 "$XELL_LAUNCH2_URL" \
            --output "$RUNNER_TEMP/XellLaunch2.xex"
          echo "$XELL_LAUNCH2_SHA256  $RUNNER_TEMP/XellLaunch2.xex" | sha256sum --check

      - name: Create and verify FAT32 USB package
        shell: bash
        run: |
          set -Eeuo pipefail
          python3 "$XW_PROJECT_DIR/scripts/package_usb.py" \
            --elf "$XW_PROJECT_DIR/browser/xenon.elf" \
            --xell "$RUNNER_TEMP/XellLaunch2.xex" \
            --project "$XW_PROJECT_DIR" \
            --output "$RUNNER_TEMP/X360-Web-Total-USB.zip"
          test -s "$RUNNER_TEMP/X360-Web-Total-USB.zip"

      - name: Generate release checksums
        shell: bash
        run: |
          set -Eeuo pipefail
          cd "$RUNNER_TEMP"
          sha256sum X360-Web-Total-USB.zip XellLaunch2.xex > SHA256SUMS-RELEASE.txt

      - name: Upload build artifacts
        uses: actions/upload-artifact@v4
        with:
          name: X360-Web-Total-0.4.0-USB
          path: |
            ${{ runner.temp }}/X360-Web-Total-USB.zip
            ${{ runner.temp }}/XellLaunch2.xex
            ${{ runner.temp }}/SHA256SUMS-RELEASE.txt
            ${{ runner.temp }}/x360-web-total-source/X360-Web-Total-0.4.0/browser/xenon.elf
          if-no-files-found: error
          retention-days: 90

      - name: Publish public GitHub Release
        if: startsWith(github.ref, 'refs/tags/')
        uses: softprops/action-gh-release@v2
        with:
          name: X360 Web Total ${{ github.ref_name }}
          tag_name: ${{ github.ref_name }}
          generate_release_notes: true
          draft: false
          prerelease: ${{ contains(github.ref_name, '-') }}
          fail_on_unmatched_files: true
          files: |
            ${{ runner.temp }}/X360-Web-Total-USB.zip
            ${{ runner.temp }}/XellLaunch2.xex
            ${{ runner.temp }}/SHA256SUMS-RELEASE.txt
            ${{ runner.temp }}/x360-web-total-source/X360-Web-Total-0.4.0/browser/xenon.elf

      - name: Build summary
        if: always()
        shell: bash
        run: |
          {
            echo '## X360 Web Total 0.4.0'
            echo
            echo "Build status: ${{ job.status }}"
            echo "Trigger: ${{ github.event_name }}"
            echo 'The release is published only for a pushed v* tag.'
            echo 'A successful cross-build still requires physical Xbox 360 testing.'
          } >> "$GITHUB_STEP_SUMMARY"
