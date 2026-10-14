#!/usr/bin/env python3
"""Offline source/packet invariants for the wired Apex Pro Mini OpenRGB port.

Run: python3 scripts/check-apex-pro-mini-protocol.py
This is not a C++ compile test or a physical HID acceptance test.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "Controllers/SteelSeriesController"
CONTROLLER = BASE / "SteelSeriesApexController/SteelSeriesApexController.cpp"
DEVICES = BASE / "SteelSeriesDevices.h"
DETECT = BASE / "SteelSeriesControllerDetect.cpp"
REGIONS = BASE / "SteelSeriesApexRegions.h"

EXPECTED_HID = (
    41, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46, 137,
    43, 20, 26, 8, 21, 23, 28, 24, 12, 18, 19, 47, 48, 49,
    57, 4, 22, 7, 9, 10, 11, 13, 14, 15, 51, 52, 50, 40,
    225, 100, 29, 27, 6, 25, 5, 17, 16, 54, 55, 56, 135, 229,
    224, 227, 226, 139, 44, 138, 136, 230, 231, 240, 228, 42,
)


class MiniProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cpp = CONTROLLER.read_text()
        cls.devices = DEVICES.read_text()
        cls.detect = DETECT.read_text()
        cls.regions = REGIONS.read_text()

    def test_product_and_vendor_interface(self):
        self.assertRegex(self.devices, r"STEELSERIES_APEX_PRO_MINI_PID\s+0x161[Ee]")
        self.assertRegex(
            self.detect,
            r"REGISTER_HID_DETECTOR_IPU\(\s*\"SteelSeries Apex Pro Mini Wired\"[^\n]+"
            r"STEELSERIES_APEX_PRO_MINI_PID,\s*1,\s*0xFFC0,\s*1",
        )

    def test_hid_codes_are_exact_and_unique(self):
        match = re.search(r"apex_pro_mini_hid_codes\[\]\s*=\s*\{([^}]+)\}", self.cpp)
        self.assertIsNotNone(match)
        actual = tuple(int(n) for n in re.findall(r"\b\d+\b", match.group(1)))
        self.assertEqual(actual, EXPECTED_HID)
        self.assertEqual(len(actual), 68)
        self.assertEqual(len(set(actual)), 68)

    def test_all_mini_codes_exist_in_shared_openrgb_key_table(self):
        match = re.search(r"static unsigned int keys\[\]\s*=\s*\{([^}]+)\}", self.cpp)
        self.assertIsNotNone(match)
        shared = {int(n, 16) for n in re.findall(r"0x[0-9A-Fa-f]+", match.group(1))}
        self.assertTrue(set(EXPECTED_HID).issubset(shared))

    def test_direct_packet_shape_and_release(self):
        self.assertIn("unsigned char report[513] = {};", self.cpp)
        self.assertIn("report[1] = 0x40;", self.cpp)
        self.assertIn("report[2] = static_cast<unsigned char>(count);", self.cpp)
        self.assertIn("hid_send_feature_report(dev, report, sizeof(report))", self.cpp)
        self.assertIn("device_pid == STEELSERIES_APEX_PRO_MINI_PID ? 0x41 : reset_cmd", self.cpp)
        self.assertIn("hid_write(dev, obuf, STEELSERIES_PACKET_OUT_SIZE)", self.cpp)

    def test_mini_region_patch_is_defined(self):
        self.assertRegex(self.regions, r'\"64820\"\s*,\s*\{\s*apex_mini_us_region_patch')
        self.assertIn('return "64820";', self.cpp)

    def test_unverified_queries_are_skipped(self):
        self.assertIn("device_pid != STEELSERIES_APEX_PRO_MINI_PID && ReadBrightness", self.cpp)
        self.assertIn('return "Unknown";', self.cpp)


if __name__ == "__main__":
    unittest.main()
