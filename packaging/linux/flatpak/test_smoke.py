#!/usr/bin/env python3
"""Offline regression checks for the GUI smoke driver's safety policies."""

import configparser
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("smoke", Path(__file__).with_name("smoke.py"))
smoke = importlib.util.module_from_spec(spec)
spec.loader.exec_module(smoke)


class DrawingFailureTest(unittest.TestCase):
    def check_log(self, contents):
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "amule.log"
            log.write_bytes(contents)
            smoke.check_drawing_failures(log)

    def test_unrelated_gtk_warning_is_allowed(self):
        self.check_log(b"Gtk-WARNING **: cannot open optional resource\nnormal shutdown\n")

    def test_empty_log_is_allowed(self):
        self.check_log(b"")

    def test_drawing_failure_fails(self):
        with self.assertRaisesRegex(RuntimeError, "drawing failure"):
            self.check_log(b"Gtk-WARNING **: drawing failure for widget 'wxPizza'\n")

    def test_only_drawing_failure_lines_are_reported(self):
        with self.assertRaises(RuntimeError) as result:
            self.check_log(b"unrelated warning\ndrawing failure: first\ndrawing failure: second\n")
        self.assertIn("first", str(result.exception))
        self.assertIn("second", str(result.exception))
        self.assertNotIn("unrelated warning", str(result.exception))

    def test_invalid_utf8_does_not_hide_failure(self):
        with self.assertRaisesRegex(RuntimeError, "drawing failure"):
            self.check_log(b"\xff\ndrawing failure: cairo\n")

    def test_missing_log_fails(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(FileNotFoundError):
                smoke.check_drawing_failures(Path(directory) / "missing.log")


class PermissionTest(unittest.TestCase):
    permissions = ("[Context]\nshared=network;ipc;\nsockets=wayland;fallback-x11;\n"
                   "filesystems=xdg-download;xdg-music:ro;~/.aMule:create;\n")

    def test_restricted_permissions_are_accepted(self):
        smoke.check_permissions(self.permissions)

    def test_explicit_read_write_downloads_are_accepted(self):
        smoke.check_permissions(self.permissions.replace("xdg-download;", "xdg-download:rw;"))

    def test_broad_or_inadequate_filesystem_permissions_fail(self):
        for before, after in [
            ("xdg-download;", "home;"),
            ("xdg-download;", "host;"),
            ("xdg-download;", "xdg-download:ro;"),
            ("xdg-music:ro;", "xdg-music;"),
            ("~/.aMule:create;", "~/.aMule;"),
            ("~/.aMule:create;", "~/.aMule:ro;"),
            ("~/.aMule:create;", ""),
        ]:
            with self.subTest(after=after), self.assertRaisesRegex(RuntimeError, "filesystem"):
                smoke.check_permissions(self.permissions.replace(before, after))

    def test_extra_host_access_fails(self):
        with self.assertRaisesRegex(RuntimeError, "filesystem"):
            smoke.check_permissions(self.permissions.replace("filesystems=", "filesystems=host;"))

    def test_private_mapping_that_hides_legacy_profile_fails(self):
        with self.assertRaisesRegex(RuntimeError, "profile mapping"):
            smoke.check_permissions(self.permissions + "persistent=.aMule;\n")

    def test_gpu_access_fails(self):
        for device in ["dri", "all"]:
            with self.subTest(device=device), self.assertRaisesRegex(RuntimeError, "GPU"):
                smoke.check_permissions(self.permissions + f"devices={device};\n")

    def test_unconditional_x11_fails(self):
        with self.assertRaisesRegex(RuntimeError, "display"):
            smoke.check_permissions(self.permissions.replace("fallback-x11;", "fallback-x11;x11;"))

    def test_missing_display_backend_fails(self):
        for backend in ["wayland;", "fallback-x11;"]:
            with self.subTest(backend=backend), self.assertRaisesRegex(RuntimeError, "display"):
                smoke.check_permissions(self.permissions.replace(backend, ""))

    def test_missing_context_fails(self):
        with self.assertRaisesRegex(RuntimeError, "filesystem"):
            smoke.check_permissions("[Application]\nname=org.amule.aMule\n")


class ProfileTest(unittest.TestCase):
    def test_new_or_empty_profile_can_be_prepared(self):
        for existing in [False, True]:
            with self.subTest(existing=existing), tempfile.TemporaryDirectory() as directory:
                home = Path(directory)
                if existing:
                    (home / ".aMule").mkdir()
                config = smoke.prepare_profile(home, "x11")
                self.assertEqual(config, home / ".aMule")
                saved = configparser.ConfigParser()
                saved.read(config / "amule.conf")
                self.assertEqual(saved.get("eMule", "Nick"), "Flatpak smoke profile")
                self.assertEqual(saved.getint("eMule", "ConnectToKad"), 0)
                self.assertEqual(saved.getint("eMule", "ConnectToED2K"), 0)
                self.assertEqual(saved.getint("Razor_Preferences", "Maximized"), 0)

    def test_native_wayland_keeps_default_window_state(self):
        with tempfile.TemporaryDirectory() as directory:
            config = smoke.prepare_profile(Path(directory), "gnome-wayland")
            self.assertNotIn("Maximized", (config / "amule.conf").read_text())

    def test_existing_profile_is_never_overwritten(self):
        for filename in ["amule.conf", "preferences.dat", "unrelated-file"]:
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as directory:
                home = Path(directory)
                config = home / ".aMule"
                config.mkdir()
                existing = config / filename
                existing.write_bytes(b"existing user data")
                with self.assertRaisesRegex(RuntimeError, "empty"):
                    smoke.prepare_profile(home, "x11")
                self.assertEqual(existing.read_bytes(), b"existing user data")
                self.assertEqual(list(config.iterdir()), [existing])

    def test_symlinked_profile_is_never_followed(self):
        for dangling in [False, True]:
            with self.subTest(dangling=dangling), tempfile.TemporaryDirectory() as directory:
                home = Path(directory)
                target = home / "target"
                if not dangling:
                    target.mkdir()
                (home / ".aMule").symlink_to(target)
                with self.assertRaisesRegex(RuntimeError, "symlink"):
                    smoke.prepare_profile(home, "x11")
                self.assertFalse((target / "amule.conf").exists())

    def test_rerun_refuses_to_replace_the_previous_profile(self):
        with tempfile.TemporaryDirectory() as directory:
            home = Path(directory)
            config = smoke.prepare_profile(home, "x11")
            original = (config / "amule.conf").read_bytes()
            with self.assertRaisesRegex(RuntimeError, "empty"):
                smoke.prepare_profile(home, "kde-wayland")
            self.assertEqual((config / "amule.conf").read_bytes(), original)


if __name__ == "__main__":
    unittest.main()
