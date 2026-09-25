import importlib.util
import json
from pathlib import Path
import re
import tempfile
import unittest


WINDOWS_DIR = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("po_to_nsh", WINDOWS_DIR / "po-to-nsh.py")
BRIDGE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BRIDGE)


class InstallerStringsTest(unittest.TestCase):
    def test_lf_and_crlf_emit_one_windows_line_break(self):
        for text in ("first\nsecond", "first\r\nsecond"):
            with self.subTest(text=text):
                self.assertEqual(BRIDGE.nsis_quote_body(text), r"first$\r$\nsecond")

    def test_mixed_and_consecutive_line_breaks(self):
        self.assertEqual(
            BRIDGE.nsis_quote_body("a\r\n\nb\nc"),
            r"a$\r$\n$\r$\nb$\r$\nc",
        )

    def test_variables_paths_and_quotes_survive(self):
        self.assertEqual(
            BRIDGE.nsis_quote_body('"$INSTDIR\\aMule"\t$0 $APPDATA $(MYSTR_X)\n'),
            r'$\"$INSTDIR\aMule$\"$\t$0 $APPDATA $(MYSTR_X)$\r$\n',
        )

    def test_standalone_carriage_return_is_preserved(self):
        self.assertEqual(BRIDGE.nsis_quote_body("a\rb"), r"a$\rb")

    def test_source_and_bridge_keys_agree_without_carriage_returns(self):
        source = (WINDOWS_DIR / "installer_strings.c").read_text(encoding="utf-8")
        strings = re.findall(r'_\("((?:\\.|[^"\\])*)"\)', source)
        self.assertEqual(
            [BRIDGE._c_unescape(text) for text in strings],
            [text for _, text in BRIDGE.KEYS],
        )
        for key, text in BRIDGE.KEYS:
            with self.subTest(key=key):
                self.assertNotIn("\r", text)

    def test_translated_and_fallback_messages(self):
        msgid = dict(BRIDGE.KEYS)["MYSTR_MSG_AMULE_RUNNING"]
        for newline in ("\n", "\r\n"):
            with self.subTest(newline=newline), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                translated = 'Déjà actif dans "$INSTDIR".' + newline + "Fermez aMule."
                (root / "fr.po").write_text(
                    "msgid " + json.dumps(msgid, ensure_ascii=False)
                    + "\nmsgstr " + json.dumps(translated, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
                installer = root / "installer.nsi"
                installer.write_text(
                    '!insertmacro MUI_LANGUAGE "English"\n'
                    '!insertmacro MUI_LANGUAGE "French"\n'
                    '!insertmacro MUI_LANGUAGE "Czech"\n',
                    encoding="utf-8",
                )
                output = root / "strings.nsh"
                self.assertEqual(
                    BRIDGE.main(["po-to-nsh.py", str(root), str(output), str(installer)]), 0
                )
                lines = output.read_text(encoding="utf-8").splitlines()
                messages = [line for line in lines if line.startswith("LangString MYSTR_MSG_AMULE_RUNNING ")]
                self.assertEqual(len(messages), 2)
                self.assertIn(
                    '${LANG_FRENCH} "Déjà actif dans $\\"$INSTDIR$\\".$\\r$\\nFermez aMule."',
                    messages[0],
                )
                self.assertIn('${LANG_CZECH} "' + BRIDGE.nsis_quote_body(msgid) + '"', messages[1])
                self.assertNotIn(r"$\r$\r$\n", output.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
