#!/usr/bin/env python3
"""
sync_notes_to_esp32.py — push the NOTES vault to the ESP32 over WiFi.

The ESP32 runs the NOTES_SYNC sketch as its own WiFi access point (SSID
"ESP32_NOTES") and serves a small HTTP API. This script:

  1. Reads the NOTES vault on the desktop  (READ-ONLY — never writes it).
  2. Hashes every file (sha256).
  3. GETs the board's manifest of what it already holds.
  4. Uploads only changed/new files, deletes files the board has but the
     vault no longer does, then stores the new manifest on the board.

Nothing here ever modifies the NOTES vault. The board writes only its own SD
card (under /NOTES), each upload streamed to a temp file and renamed into
place so an interrupted run can't corrupt the existing mirror.

Usage:
    # First join the board's WiFi network "ESP32_NOTES", then:
    python sync_notes_to_esp32.py [HOST] [--notes DIR] [--dry-run]

    HOST        Board address (default 192.168.4.1).
    --notes DIR Vault to mirror (default /home/artie/REPOS/NOTES).
    --dry-run   Show what would change; contact the board for its manifest
                but send nothing.
"""

import argparse
import hashlib
import http.client
import json
import os
import sys
import time
import urllib.parse

DEFAULT_HOST  = "192.168.4.1"
DEFAULT_NOTES = "/home/artie/REPOS/NOTES"

# Directories never mirrored (version control / editor caches).
SKIP_DIRS = {".git"}

TIMEOUT = 120
RETRIES = 4
RETRY_DELAY = 2


def _send(host, method, url, body=None, headers=None):
    """One HTTP request, retried on timeout/connection errors.
    Returns (status, data); raises after RETRIES failures."""
    last = None
    for attempt in range(1, RETRIES + 1):
        try:
            conn = http.client.HTTPConnection(host, timeout=TIMEOUT)
            conn.request(method, url, body, headers or {})
            resp = conn.getresponse()
            data = resp.read()
            conn.close()
            return resp.status, data
        except (OSError, http.client.HTTPException) as e:
            last = e
            if attempt < RETRIES:
                print(f"    retry {attempt}/{RETRIES - 1}: {type(e).__name__}")
                time.sleep(RETRY_DELAY)
    raise last


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:                 # read-only
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def build_local_manifest(notes_root):
    """Map of "/relative/posix/path" -> (abspath, sha256) for the whole vault."""
    local = {}
    for dirpath, dirnames, filenames in os.walk(notes_root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            abspath = os.path.join(dirpath, name)
            if not os.path.isfile(abspath):
                continue
            rel = os.path.relpath(abspath, notes_root).replace(os.sep, "/")
            try:
                local["/" + rel] = (abspath, sha256_of(abspath))
            except OSError as e:
                print(f"  skip (unreadable): {rel}  ({e})")
    return local


def get_remote_manifest(host):
    _, data = _send(host, "GET", "/manifest")
    try:
        return json.loads(data or b"{}")
    except json.JSONDecodeError:
        return {}


def upload_file(host, rel, abspath):
    boundary = "----notesync-boundary"
    with open(abspath, "rb") as f:              # read-only
        payload = f.read()
    body = (
        f"--{boundary}\r\n"
        'Content-Disposition: form-data; name="file"; filename="upload.bin"\r\n'
        "Content-Type: application/octet-stream\r\n\r\n"
    ).encode() + payload + f"\r\n--{boundary}--\r\n".encode()
    url = "/upload?path=" + urllib.parse.quote(rel)
    status, _ = _send(host, "POST", url, body,
                      {"Content-Type": f"multipart/form-data; boundary={boundary}"})
    return status


def delete_file(host, rel):
    status, _ = _send(host, "POST", "/delete?path=" + urllib.parse.quote(rel))
    return status


def put_manifest(host, manifest):
    status, _ = _send(host, "POST", "/manifest", json.dumps(manifest).encode(),
                      {"Content-Type": "application/json"})
    return status


def main():
    ap = argparse.ArgumentParser(description="Push the NOTES vault to the ESP32.")
    ap.add_argument("host", nargs="?", default=DEFAULT_HOST,
                    help=f"board address (default {DEFAULT_HOST})")
    ap.add_argument("--notes", default=DEFAULT_NOTES,
                    help=f"vault directory (default {DEFAULT_NOTES})")
    ap.add_argument("--dry-run", action="store_true",
                    help="show changes without sending anything")
    ap.add_argument("--only", default=None,
                    help="sync ONLY this vault subpath (e.g. /RECIPES). "
                         "Files outside it are left untouched on the board "
                         "(no deletes outside the prefix).")
    ap.add_argument("--max-mb", type=float, default=None,
                    help="skip any vault file larger than this many MB "
                         "(e.g. --max-mb 5 to avoid huge zips/PDFs).")
    args = ap.parse_args()

    if not os.path.isdir(args.notes):
        print(f"Error: NOTES dir not found: {args.notes}")
        sys.exit(1)

    print(f"Hashing vault: {args.notes}")
    local = build_local_manifest(args.notes)
    print(f"  {len(local)} files in vault")

    if args.max_mb is not None:
        cap = int(args.max_mb * 1024 * 1024)
        oversized = [rel for rel, (ap_, _) in local.items()
                     if os.path.getsize(ap_) > cap]
        for rel in sorted(oversized):
            mb = os.path.getsize(local[rel][0]) / (1024 * 1024)
            print(f"  skip (>{args.max_mb} MB, {mb:.1f} MB): {rel}")
            del local[rel]

    only = args.only
    if only is not None:
        if not only.startswith("/"):
            only = "/" + only
        only = only.rstrip("/")
        local = {rel: v for rel, v in local.items()
                 if rel == only or rel.startswith(only + "/")}
        print(f"  --only {only}: {len(local)} files match")

    print(f"Fetching board manifest from {args.host} ...")
    try:
        remote = get_remote_manifest(args.host)
    except OSError as e:
        print(f"Error: cannot reach board at {args.host}: {e}")
        print("Are you joined to the 'ESP32_NOTES' WiFi network?")
        sys.exit(1)
    print(f"  board reports {len(remote)} files")

    to_upload = [rel for rel, (_, h) in local.items() if remote.get(rel) != h]
    if only is not None:
        # Only ever delete board files that live UNDER the prefix; everything
        # else the board holds is left exactly as-is.
        to_delete = [rel for rel in remote
                     if (rel == only or rel.startswith(only + "/"))
                     and rel not in local]
    else:
        to_delete = [rel for rel in remote if rel not in local]
    to_upload.sort()
    to_delete.sort()

    print(f"\nPlan: {len(to_upload)} to upload, {len(to_delete)} to delete.")
    if args.dry_run:
        for rel in to_upload:
            print(f"  [upload] {rel}")
        for rel in to_delete:
            print(f"  [delete] {rel}")
        print("\n(dry run — nothing sent)")
        return

    # Start from what the board already holds so a filtered (--only) or
    # size-capped run never drops other files from the stored manifest.
    confirmed = dict(remote)
    failed = []

    sent = 0
    for i, rel in enumerate(to_upload, 1):
        abspath, h = local[rel]
        try:
            status = upload_file(args.host, rel, abspath)
        except (OSError, http.client.HTTPException) as e:
            print(f"  [{i}/{len(to_upload)}] upload {rel}  FAILED ({type(e).__name__})")
            failed.append(rel)
            continue
        if status == 200:
            sent += 1
            confirmed[rel] = h
            print(f"  [{i}/{len(to_upload)}] upload {rel}  ok")
        else:
            print(f"  [{i}/{len(to_upload)}] upload {rel}  HTTP {status}")
            failed.append(rel)

    removed = 0
    for i, rel in enumerate(to_delete, 1):
        try:
            status = delete_file(args.host, rel)
        except (OSError, http.client.HTTPException) as e:
            print(f"  [{i}/{len(to_delete)}] delete {rel}  FAILED ({type(e).__name__})")
            continue
        if status == 200:
            removed += 1
            confirmed.pop(rel, None)
        print(f"  [{i}/{len(to_delete)}] delete {rel}  HTTP {status}")

    # Store manifest of everything confirmed on the board, even on a partial run,
    # so re-running only sends the remainder.
    try:
        mstatus = put_manifest(args.host, confirmed)
        print(f"\nManifest stored ({len(confirmed)} files): HTTP {mstatus}")
    except (OSError, http.client.HTTPException):
        print("\nWARNING: manifest not stored; re-run may resend some files.")

    print(f"Done. uploaded={sent}  deleted={removed}  failed={len(failed)}  "
          f"vault_files={len(local)}")
    if failed:
        print(f"  {len(failed)} failed — just re-run to retry only those.")
        sys.exit(1)


if __name__ == "__main__":
    main()
