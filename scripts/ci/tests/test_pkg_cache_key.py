"""What a package cache key must notice, and what it must ignore."""

import importlib.util
import json
import os
import pathlib

SCRIPT = pathlib.Path(__file__).resolve().parent.parent / "pkg-cache-key.py"
_spec = importlib.util.spec_from_file_location("pkg_cache_key", SCRIPT)
key_module = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(key_module)


def a_tree(tmp_path, packages):
    """A repository holding one package/<name>/ directory per entry."""
    for name, files in packages.items():
        directory = tmp_path / "package" / name
        directory.mkdir(parents=True, exist_ok=True)
        for filename, content in files.items():
            (directory / filename).write_text(content)
    return tmp_path


def a_key(tmp_path, graph, config, name, buildroot_sha="abc123"):
    fingerprints = key_module.Fingerprints(graph, key_module.Configuration(config), str(tmp_path))
    return key_module.key_of(fingerprints, graph, buildroot_sha, name)


GRAPH = {
    "dolphin-emu": {"version": "5.0", "dependencies": ["libcurl"]},
    "libcurl": {"version": "8.9.0", "dependencies": ["libopenssl"]},
    "libopenssl": {"version": "3.3.1", "dependencies": []},
    "vice": {"version": "3.8", "dependencies": []},
}


def test_a_package_key_moves_when_a_dependency_changes_configuration(tmp_path):
    tree = a_tree(tmp_path, {"dolphin-emu": {"dolphin-emu.mk": "x"}, "libcurl": {"libcurl.mk": "y"}})
    without = a_key(tree, GRAPH, "BR2_PACKAGE_LIBCURL=y\n", "dolphin-emu")
    with_websockets = a_key(
        tree, GRAPH, "BR2_PACKAGE_LIBCURL=y\nBR2_PACKAGE_LIBCURL_WEBSOCKETS_SUPPORT=y\n", "dolphin-emu"
    )
    assert without != with_websockets


def test_a_package_key_holds_still_when_an_unrelated_package_changes(tmp_path):
    tree = a_tree(tmp_path, {"dolphin-emu": {"dolphin-emu.mk": "x"}, "vice": {"vice.mk": "y"}})
    before = a_key(tree, GRAPH, "BR2_PACKAGE_VICE=y\n", "dolphin-emu")
    after = a_key(tree, GRAPH, "BR2_PACKAGE_VICE=y\nBR2_PACKAGE_VICE_SDL2=y\n", "dolphin-emu")
    assert before == after


def test_a_package_key_moves_when_its_own_sources_change(tmp_path):
    tree = a_tree(tmp_path, {"libcurl": {"libcurl.mk": "one"}})
    before = a_key(tree, GRAPH, "", "libcurl")
    (tree / "package" / "libcurl" / "libcurl.mk").write_text("two")
    after = a_key(tree, GRAPH, "", "libcurl")
    assert before != after


def test_a_package_key_reaches_the_dependencies_of_its_dependencies(tmp_path):
    tree = a_tree(tmp_path, {"dolphin-emu": {"dolphin-emu.mk": "x"}})
    before = a_key(tree, GRAPH, "BR2_PACKAGE_LIBOPENSSL=y\n", "dolphin-emu")
    after = a_key(tree, GRAPH, "BR2_PACKAGE_LIBOPENSSL=y\nBR2_PACKAGE_LIBOPENSSL_ENGINE=y\n", "dolphin-emu")
    assert before != after


def test_a_package_key_moves_when_a_vendored_source_tree_changes(tmp_path):
    tree = a_tree(tmp_path, {"frontend-pkg": {
        "frontend-pkg.mk": "FRONTEND_PKG_SITE = $(TOPDIR)/../projects/frontend\n"
                           "FRONTEND_PKG_SITE_METHOD = local\n"}})
    (tree / "projects" / "frontend").mkdir(parents=True)
    (tree / "projects" / "frontend" / "main.cpp").write_text("int main() { return 0; }")
    graph = {"frontend-pkg": {"version": "1", "dependencies": []}}
    before = a_key(tree, graph, "", "frontend-pkg")
    (tree / "projects" / "frontend" / "main.cpp").write_text("int main() { return 1; }")
    after = a_key(tree, graph, "", "frontend-pkg")
    assert before != after


def test_a_package_key_moves_when_buildroot_itself_moves(tmp_path):
    tree = a_tree(tmp_path, {"libcurl": {"libcurl.mk": "x"}})
    before = a_key(tree, GRAPH, "", "libcurl", buildroot_sha="aaa")
    after = a_key(tree, GRAPH, "", "libcurl", buildroot_sha="bbb")
    assert before != after


def test_a_nested_package_is_identified_by_the_directory_it_shares(tmp_path):
    directory = tmp_path / "package" / "libretro-mame" / "libretro-mame-old"
    directory.mkdir(parents=True)
    (directory / "libretro-mame-old.mk").write_text("x")
    fingerprints = key_module.Fingerprints({}, key_module.Configuration(""), str(tmp_path))
    assert fingerprints._package_dir("libretro-mame-old") == str(tmp_path / "package" / "libretro-mame")


def test_the_closure_of_a_package_lists_every_package_it_is_built_against():
    assert key_module.closure_of(GRAPH, "dolphin-emu") == ["libcurl", "libopenssl"]


def test_a_configuration_slice_ignores_options_belonging_to_another_package():
    config = key_module.Configuration(
        "BR2_PACKAGE_LIBCURL=y\nBR2_PACKAGE_LIBCURL_WEBSOCKETS_SUPPORT=y\nBR2_PACKAGE_VICE=y\n"
    )
    assert config.about("libcurl") == ["BR2_PACKAGE_LIBCURL=y", "BR2_PACKAGE_LIBCURL_WEBSOCKETS_SUPPORT=y"]


def test_a_graph_wrapped_in_make_noise_is_still_read(tmp_path):
    graph_file = tmp_path / "graph.json"
    graph_file.write_text("make: Entering directory '/work'\n" + json.dumps(GRAPH) + "\nmake: Leaving\n")
    text = graph_file.read_text()
    assert json.loads(text[text.index("{"):text.rindex("}") + 1]) == GRAPH


def test_a_package_missing_from_the_graph_is_given_no_key(tmp_path, capsys):
    tree = a_tree(tmp_path, {"libcurl": {"libcurl.mk": "x"}})
    graph_file = tmp_path / "graph.json"
    graph_file.write_text(json.dumps(GRAPH))
    config_file = tmp_path / ".config"
    config_file.write_text("BR2_PACKAGE_LIBCURL=y\n")
    import sys
    argv = sys.argv
    sys.argv = ["pkg-cache-key.py", "--graph", str(graph_file), "--config", str(config_file),
                "--buildroot-sha", "abc", "--packages", "libcurl not-selected", "--tree", str(tree)]
    try:
        key_module.main()
    finally:
        sys.argv = argv
    printed = capsys.readouterr().out.split()
    assert printed[0] == "libcurl" and "not-selected" not in printed
