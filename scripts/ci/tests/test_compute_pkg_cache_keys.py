"""What the CI may believe about the key file, whatever the container did."""

import os
import pathlib
import subprocess

REPOSITORY = pathlib.Path(__file__).resolve().parents[3]
SCRIPT = REPOSITORY / "scripts" / "ci" / "compute-pkg-cache-keys.sh"


def a_fake_docker(tmp_path, body):
    """A stand-in for `docker run`, so the script can be exercised without one."""
    fake = tmp_path / "docker"
    fake.write_text("#!/bin/sh\n" + body + "\n")
    fake.chmod(0o755)
    return fake


def run_with(tmp_path, docker, packages="libcurl dolphin-emu"):
    output = tmp_path / "output"
    output.mkdir(exist_ok=True)
    environment = dict(os.environ)
    environment.update({
        "DOCKER": str(docker),
        "OUTPUTDIR": str(output),
        "WORKDIR": str(REPOSITORY),
        "ARCH": "rpi5_64",
        "PKG_CACHE_LIST": packages,
    })
    completed = subprocess.run(
        ["bash", str(SCRIPT), "recalbox-rpi5_64-1"],
        cwd=str(tmp_path), env=environment, capture_output=True, text=True,
    )
    return completed, output / ".pkg-cache-keys"


def test_the_keys_the_container_computed_are_what_the_ci_reads(tmp_path):
    docker = a_fake_docker(tmp_path, f'printf "libcurl aaa\\ndolphin-emu bbb\\n" > {tmp_path}/output/.pkg-cache-keys')
    completed, keys = run_with(tmp_path, docker)
    assert completed.returncode == 0
    assert keys.read_text() == "libcurl aaa\ndolphin-emu bbb\n"


def test_a_container_that_fails_leaves_no_key_and_does_not_fail_the_build(tmp_path):
    docker = a_fake_docker(tmp_path, "echo 'make: *** No rule to make target' >&2; exit 2")
    completed, keys = run_with(tmp_path, docker)
    assert completed.returncode == 0
    assert keys.read_text() == ""
    assert "every package will MISS" in completed.stdout


def test_keys_written_before_a_failure_are_dropped_rather_than_half_trusted(tmp_path):
    docker = a_fake_docker(
        tmp_path, f'printf "libcurl aaa\\n" > {tmp_path}/output/.pkg-cache-keys; exit 1')
    _, keys = run_with(tmp_path, docker)
    assert keys.read_text() == ""


def arguments_given_to(tmp_path, packages="libcurl dolphin-emu"):
    docker = a_fake_docker(tmp_path, f'printf "%s\\n" "$@" > {tmp_path}/arguments')
    run_with(tmp_path, docker, packages=packages)
    return (tmp_path / "arguments").read_text().splitlines()


def test_the_container_is_told_the_buildroot_revision_git_records(tmp_path):
    recorded = subprocess.run(
        ["git", "-C", str(REPOSITORY), "ls-tree", "HEAD", "buildroot"],
        capture_output=True, text=True, check=True).stdout.split()[2]
    assert f"BUILDROOT_SHA={recorded}" in arguments_given_to(tmp_path)


def test_a_buildroot_left_dirty_by_a_previous_job_is_reset_before_the_merge(tmp_path):
    """mergeToBR refuses a tree it did not leave clean, and the runner keeps one between jobs."""
    given = " ".join(arguments_given_to(tmp_path))
    assert "git reset --hard HEAD" in given and "git clean -fdx" in given
    assert "GIT_CONFIG_KEY_0=safe.directory" in given


def test_the_package_list_reaches_the_container_whole(tmp_path):
    assert "PKG_CACHE_LIST=libcurl vice kodi" in arguments_given_to(tmp_path, packages="libcurl vice kodi")
