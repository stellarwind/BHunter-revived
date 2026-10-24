"""Prevent incomplete denominators, variant inflation and false game admission."""
import unittest
from mkstatus import summarize, bar, render


def record(ident, address='00401000', status='func', role='engine_candidate',
           integration='linked_harness', admitted=False, **extra):
    return dict(id=ident, original_va=address, implementation=status, role=role,
                integration=integration, game_admitted=admitted, **extra)


def catalog(*roles):
    return {'records': [dict(address=f'{0x401000 + i * 0x1000:08x}', role=role)
                        for i, role in enumerate(roles)]}


class ProgressAccountingTests(unittest.TestCase):
    def test_full_map_includes_untracked_unknown_candidates(self):
        s = summarize({'records': [record('done')]}, catalog('engine_candidate', 'unknown', 'game'))
        self.assertEqual((s['total'], s['tracked'], s['recovered'], s['remaining']), (3, 1, 1, 2))
        self.assertEqual(s['counts']['not_started'], 2)

    def test_object_match_does_not_shadow_linked_function(self):
        s = summarize({'records': [record('functional'),
            record('match', '0x00401000', 'bm', integration='object_only')]}, catalog('engine_candidate'))
        self.assertEqual(s['recovered'], 1)
        self.assertEqual(s['counts']['func'], 1)
        self.assertEqual(s['variant_counts']['comparison_only'], 1)
        self.assertNotIn('bm', s['counts'])

    def test_object_only_and_historical_functions_are_not_restored(self):
        rows = [record('match', status='bm', integration='object_only'),
                record('old', '00402000', historical_variant=True),
                record('superseded', '00403000', validation={'superseded_by': 'new'})]
        s = summarize({'records': rows}, catalog('game', 'game', 'unknown'))
        self.assertEqual(s['recovered'], 0)
        self.assertEqual(s['counts']['not_started'], 3)
        self.assertEqual(s['variant_counts']['historical_or_superseded'], 2)

    def test_all_verified_helper_roles_are_excluded(self):
        roles = ['game', 'library_verified', 'compiler_support', 'import_thunk', 'shared_tail']
        s = summarize({'records': [record('done'),
            record('crt', '00402000', 'reused', 'library_verified')]}, catalog(*roles))
        self.assertEqual((s['mapped'], s['excluded'], s['total']), (5, 4, 1))

    def test_link_required_and_adapters_are_separate(self):
        rows = [record('wip', status='wip'),
                record('object', '00402000', integration='object_only'),
                record('adapter', '00403000', status='adapter')]
        s = summarize({'records': rows}, catalog('game', 'game', 'game'))
        self.assertEqual(s['recovered'], 0)
        self.assertEqual(s['counts']['wip'], 1)
        self.assertEqual(s['counts']['not_started'], 1)
        self.assertEqual(s['counts']['adapted_or_reused'], 1)

    def test_game_admission_requires_game_link_and_ignores_history(self):
        with self.assertRaisesRegex(ValueError, 'Invalid game admission'):
            summarize({'records': [record('bad', admitted=True)]}, catalog('game'))
        s = summarize({'records': [record('valid', integration='scenario_exercised', admitted=True)]}, catalog('game'))
        self.assertEqual(s['counts']['admitted'], 1)
        s = summarize({'records': [record('old', integration='linked_game', admitted=True,
                                         historical_variant=True)]}, catalog('game'))
        self.assertNotIn('admitted', s['counts'])

    def test_bad_catalog_and_ledger_fail(self):
        for rows, roles, message in [
            ([record('same'), record('same')], catalog('game'), 'Duplicate implementation'),
            ([record('bad', status='doneish')], catalog('game'), 'Unknown implementation'),
            ([record('missing', '00402000')], catalog('game'), 'absent from role catalog'),
            ([record('conflict')], catalog('library_verified'), 'conflicts with excluded'),
        ]:
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                summarize({'records': rows}, roles)
        roles = catalog('game'); roles['records'] *= 2
        with self.assertRaisesRegex(ValueError, 'Duplicate catalog address'):
            summarize({'records': []}, roles)

    def test_render_uses_full_denominator_and_explains_limits(self):
        s = summarize({'records': [record('done')]}, catalog('game', 'unknown'))
        text = render(s)
        self.assertIn('50.0% (1/2)', text)
        self.assertIn('Function count is not effort', text)
        self.assertIn('0.0% (0/0)', bar(0, 0))


if __name__ == '__main__':
    unittest.main()
