#!/usr/bin/env python
from __future__ import annotations
import os
import hashlib
import json
import requests
import configgen.recalboxFiles as recalboxFiles


class VpinballConfig:
    def __init__(self, rom, mode):
        self.rom = rom
        self.mode = mode
        self.url = "https://raw.githubusercontent.com/beudbeud/vpconfig/refs/heads/main"
        self.json_file = recalboxFiles.CONF + "cache.json"

    @staticmethod
    def file_as_bytes(file):
        with file:
            return file.read()

    def get_md5(self, rom):
        return hashlib.md5(self.file_as_bytes(open(rom, "rb"))).hexdigest()

    def download_ini(self, folder, mode):
        url_file = "{}/{}/override/{}.ini".format(self.url, folder, mode)
        r = requests.get(url_file)
        rom_name = os.path.splitext(self.rom)[0]
        open("{}.ini".format(rom_name), "wb").write(r.content)

    def load_cache(self):
        try:
            with open(self.json_file) as f:
                return json.load(f)
        except (OSError, json.JSONDecodeError):
            return None

    def find_config(self, md5, mode):
        d = self.load_cache()
        if not isinstance(d, list):
            return
        for i in d:
            if md5 == i["md5"]:
                if mode + ".ini" in i["ini"]:
                    self.download_ini(i["name"], mode)
                    break

    def download_cache(self):
        url_file = self.url + "/data.json"
        try:
            r = requests.get(url_file, timeout=10)
        except requests.RequestException:
            return
        if r.ok:
            open(self.json_file, "wb").write(r.content)

    def get_ini(self):
        # Refresh the cache if it is missing or unreadable (e.g. a previously
        # cached error page). Never let a bad cache crash the game launch.
        if not isinstance(self.load_cache(), list):
            self.download_cache()
        md5 = self.get_md5(self.rom)
        self.find_config(md5, self.mode)
