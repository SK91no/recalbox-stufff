#!/usr/bin/env python3
"""Cache key for a Buildroot package, including what it was built against.

A package's own configuration does not describe its artefact. The artefact was
compiled — and sometimes statically linked — against a set of dependencies, so
when one of those moves the artefact is stale in a way no file listing can
reveal. The key therefore folds in a fingerprint of every package in the
dependency closure, read from `make show-info` and the resolved .config.
"""

import argparse
import hashlib
import json
import os
import re

KEY_FORMAT = "pkg-cache-key v1"
KEY_LENGTH = 12

LOCAL_SITE = re.compile(r"_SITE\s*=\s*\$\(TOPDIR\)/\.\./(projects/[\w.-]+)")


class Fingerprints:
    """The identity of each package: its version, its configuration, its sources."""

    def __init__(self, graph, config, tree):
        self._graph = graph
        self._config = config
        self._tree = tree
        self._known = {}

    def of(self, name):
        if name not in self._known:
            self._known[name] = self._compute(name)
        return self._known[name]

    def _compute(self, name):
        parts = ["name=" + name]
        described = self._graph.get(name, {})
        parts.append("version=" + str(described.get("version", "")))
        parts.extend("config=" + line for line in self._config.about(name))
        directory = self._package_dir(name)
        if directory:
            parts.extend("file=" + entry for entry in self._digest_tree(directory))
            for site in self._local_sites(directory):
                parts.extend("site=" + entry for entry in self._digest_tree(site))
        return "\n".join(parts)

    def _package_dir(self, name):
        """Where the Recalbox overlay for this package lives, if it has one.

        A package may sit one level deep (package/libretro-mame/libretro-mame-old);
        its siblings share that parent, so the parent is what identifies it.
        """
        root = os.path.join(self._tree, "package")
        direct = os.path.join(root, name)
        if os.path.isdir(direct):
            return direct
        if not os.path.isdir(root):
            return None
        for category in sorted(os.listdir(root)):
            if os.path.isdir(os.path.join(root, category, name)):
                return os.path.join(root, category)
        return None

    def _local_sites(self, directory):
        """The vendored source trees this package builds from, if any."""
        sites = set()
        for entry in sorted(os.listdir(directory)):
            if not entry.endswith(".mk"):
                continue
            with open(os.path.join(directory, entry), "r", errors="replace") as handle:
                for match in LOCAL_SITE.finditer(handle.read()):
                    site = os.path.join(self._tree, match.group(1))
                    if os.path.isdir(site):
                        sites.add(site)
        return sorted(sites)

    def _digest_tree(self, directory):
        digests = []
        for parent, directories, files in os.walk(directory):
            directories[:] = sorted(d for d in directories if d != ".git")
            for name in sorted(files):
                path = os.path.join(parent, name)
                if os.path.islink(path) or not os.path.isfile(path):
                    continue
                digest = hashlib.sha256()
                with open(path, "rb") as handle:
                    for block in iter(lambda: handle.read(1 << 16), b""):
                        digest.update(block)
                digests.append(os.path.relpath(path, self._tree) + " " + digest.hexdigest())
        return digests


class Configuration:
    """The resolved .config, sliced per package.

    This is what a defconfig cannot tell us: an option a package only receives
    because something else selected it, which is precisely how the same package
    ends up built two different ways in two projects.
    """

    def __init__(self, text):
        self._lines = sorted(line.strip() for line in text.splitlines() if line.startswith("BR2_"))

    def about(self, name):
        prefix = "BR2_PACKAGE_" + name.upper().replace("-", "_")
        return [line for line in self._lines
                if line.startswith(prefix + "=") or line.startswith(prefix + "_")]


def closure_of(graph, name):
    """Every package this one is built against, directly or not."""
    seen = set()
    pending = list(graph.get(name, {}).get("dependencies", []))
    while pending:
        current = pending.pop()
        if current in seen or current == name:
            continue
        seen.add(current)
        pending.extend(graph.get(current, {}).get("dependencies", []))
    return sorted(seen)


def key_of(fingerprints, graph, buildroot_sha, name):
    digest = hashlib.sha256()
    digest.update(KEY_FORMAT.encode())
    digest.update(("buildroot=" + buildroot_sha).encode())
    digest.update(fingerprints.of(name).encode())
    for dependency in closure_of(graph, name):
        digest.update(fingerprints.of(dependency).encode())
    return digest.hexdigest()[:KEY_LENGTH]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--graph", required=True, help="JSON produced by `make show-info`")
    parser.add_argument("--config", required=True, help="the resolved output/.config")
    parser.add_argument("--buildroot-sha", required=True, help="SHA of the buildroot submodule")
    parser.add_argument("--packages", required=True, help="space-separated package names")
    parser.add_argument("--tree", default=".", help="repository root")
    arguments = parser.parse_args()

    with open(arguments.graph, "r", errors="replace") as handle:
        text = handle.read()
    graph = json.loads(text[text.index("{"):text.rindex("}") + 1])
    with open(arguments.config, "r", errors="replace") as handle:
        config = Configuration(handle.read())

    fingerprints = Fingerprints(graph, config, os.path.abspath(arguments.tree))
    for name in arguments.packages.split():
        if name not in graph:
            continue
        print(name, key_of(fingerprints, graph, arguments.buildroot_sha, name))


if __name__ == "__main__":
    main()
