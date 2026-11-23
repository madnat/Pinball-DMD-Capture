"""Counter offsets/wrap and sequential-read ambiguity must not imply filtering."""
import unittest
from analyse_filter_log import delta, moved, validate_pair


class FilterTests(unittest.TestCase):
    def sample(self, raw, filtered, **values):
        return dict(raw=raw, filtered=filtered, rawAfter=values.get('rawAfter', raw),
                    filteredAfter=values.get('filteredAfter', filtered),
                    rawDelta=values.get('rawDelta', 0), filteredDelta=values.get('filteredDelta', 0),
                    ambiguous=values.get('ambiguous', 0))

    def test_independent_offsets_and_wrap(self):
        previous = self.sample(29999, 145)
        current = self.sample(0, 146, rawDelta=1, filteredDelta=1)
        validate_pair(previous, current)
        self.assertEqual(delta(0, 29999), 1)
        validate_pair(self.sample(7, 29999), self.sample(8, 0, rawDelta=1, filteredDelta=1))

    def test_extra_raw_edge_and_window_motion(self):
        previous = self.sample(7, 90)
        current = self.sample(8, 90, rawDelta=1)
        validate_pair(previous, current)
        self.assertFalse(moved(current))
        moving = self.sample(8, 90, rawAfter=9, rawDelta=1)
        with self.assertRaisesRegex(ValueError, 'unflagged'):
            validate_pair(previous, moving)
        moving['ambiguous'] = 1
        validate_pair(previous, moving)
        previous['filteredAfter'] = 91
        with self.assertRaisesRegex(ValueError, 'unflagged'):
            validate_pair(previous, current)

    def test_reject_cross_counter_or_bad_delta(self):
        with self.assertRaisesRegex(ValueError, 'filtered delta'):
            validate_pair(self.sample(100, 7), self.sample(101, 8, rawDelta=1, filteredDelta=94))


if __name__ == '__main__':
    unittest.main()
