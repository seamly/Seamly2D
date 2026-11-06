# @file compare_handoff_writers_test.py
# @author slspencer
# @brief Pins semantic SVG comparison and its coordinate tolerance.
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
# License: https://www.gnu.org/licenses/gpl-3.0.html

from pathlib import Path
import shutil
import unittest
import uuid

from compare_handoff_writers import compare


class HandoffComparisonTests(unittest.TestCase):
    def setUp(self):
        self.directory = Path("build") / "comparison-python-tests" / str(uuid.uuid4())
        self.directory.mkdir(parents=True)
        self.old = self.directory / "pattern_old.svg"
        self.new = self.directory / "pattern_new.svg"

    def tearDown(self):
        shutil.rmtree(self.directory)

    def check(self, old, new):
        self.old.write_text(old)
        self.new.write_text(new)
        return compare(self.old, self.new)

    def test_wrapper_transforms_compose(self):
        old = "<svg><g transform='matrix(1,0,0,1,10,20)'><path d='M1,2 L3,4'/></g></svg>"
        new = "<svg><path d='M11,22 L13,24'/></svg>"
        self.assertIsNone(self.check(old, new))

    def test_coordinate_tolerance_is_not_relaxed(self):
        old = "<svg><path d='M1,2 L3,4'/></svg>"
        self.assertIsNone(self.check(old, "<svg><path d='M1.001,2 L3,4'/></svg>"))
        self.assertIn("point 0", self.check(old, "<svg><path d='M1.011,2 L3,4'/></svg>"))

    def test_metadata_and_label_text_differences_fail(self):
        old = "<svg><g id='front' data-type='piece'><text x='1' y='2'>Front</text></g></svg>"
        self.assertIn("id:", self.check(old, old.replace("id='front'", "id='back'")))
        self.assertIn("text:", self.check(old, old.replace(">Front<", ">Back<")))

    def test_subpath_topology_differences_fail(self):
        old = "<svg><path d='M1,2 M3,4'/></svg>"
        new = "<svg><path d='M1,2 L3,4'/></svg>"
        self.assertIn("commands", self.check(old, new))

    def test_valid_patterns_without_layout_pieces_match(self):
        self.assertIsNone(self.check("", ""))
        with self.assertRaises(Exception):
            self.check("", "<svg/>")


if __name__ == "__main__":
    unittest.main()
