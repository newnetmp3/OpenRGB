#!/usr/bin/env python3
"""Source-level regression checks for the wired Apex Pro Mini native controller.

Run from the repository root:
    python3 -m unittest discover -s tests -p test_apex_pro_mini_native.py

These checks do not require hardware, Qt, or hidapi. They guard the
hardware-verified 68-address protocol and 61-key US ANSI presentation.
They are not a substitute for compiling and testing against the keyboard.
"""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DRIVER = ROOT / "Controllers/SteelSeriesController/SteelSeriesApexController/SteelSeriesApexController.cpp"
VIEW = ROOT / "Controllers/SteelSeriesController/SteelSeriesApexController/RGBController_SteelSeriesApex.cpp"


def hid_addresses(source):
    match = re.search(
        r"static const unsigned char apex_pro_mini_hid_codes\[\]\s*=\s*\{([^}]+)\}",
        source,
        re.S,
    )
    if match is None:
        raise AssertionError("Mini HID address table missing")
    return [int(value) for value in re.findall(r"\d+", match.group(1))]


def ansi_matrix(source):
    match = re.search(
        r"static const unsigned int mini_ansi_matrix\[5\]\[15\]\s*=\s*\{(.*?)\n\s*\};",
        source,
        re.S,
    )
    if match is None:
        raise AssertionError("Mini ANSI matrix missing")
    rows = re.findall(r"\{([^{}]+)\}", match.group(1))
    if len(rows) != 5:
        raise AssertionError("Expected five ANSI rows")
    return [[None if cell == "NA" else int(cell)
             for cell in re.findall(r"\b(?:NA|\d+)\b", row)] for row in rows]


class ApexProMiniNativeRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.driver = DRIVER.read_text(encoding="utf-8")
        cls.view = VIEW.read_text(encoding="utf-8")

    def test_hid_order_has_68_unique_addresses(self):
        addresses = hid_addresses(self.driver)
        self.assertEqual(len(addresses), 68)
        self.assertEqual(len(set(addresses)), 68)
        self.assertEqual(addresses[0], 41)  # Esc
        self.assertEqual(addresses[67], 42)  # Backspace

    def test_ansi_matrix_has_61_distinct_physical_keys(self):
        matrix = ansi_matrix(self.view)
        self.assertTrue(all(len(row) == 15 for row in matrix))
        keys = [key for row in matrix for key in row if key is not None]
        self.assertEqual(len(keys), 61)
        self.assertEqual(len(set(keys)), 61)
        self.assertTrue(all(0 <= key < 68 for key in keys))
        self.assertEqual(matrix[0][0], 0)   # Esc
        self.assertEqual(matrix[0][13], 67)  # Backspace
        self.assertEqual(set(range(68)) - set(keys),
                         {13, 40, 43, 54, 59, 61, 62})

    def test_mini_direct_and_onboard_reports_are_separate(self):
        self.assertIn("report[1] = 0x40;", self.driver)
        self.assertIn("hid_send_feature_report(dev, report, sizeof(report))", self.driver)
        self.assertIn("report[1] = 0x41;", self.driver)
        self.assertIn("hid_write(dev, report, sizeof(report))", self.driver)
        self.assertRegex(self.driver, r"if\(active_mode == APEX_MODE_ONBOARD\)\s*\{\s*return;")

    def test_mini_remains_a_separate_pid_layout(self):
        self.assertIn("apex_controller->IsApexProMini()", self.view)
        self.assertIn("mini_zone.matrix_map.height = 5;", self.view)
        self.assertIn("mini_zone.matrix_map.width = 15;", self.view)


if __name__ == "__main__":
    unittest.main()
