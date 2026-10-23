"""Guard progress accounting against variant inflation and false admission."""
import unittest
from mkstatus import summarize, bar

def record(ident, address='00401000', status='func', role='engine_candidate', integration='linked_harness', admitted=False):
    return dict(id=ident,original_va=address,implementation=status,role=role,integration=integration,game_admitted=admitted)

class ProgressAccountingTests(unittest.TestCase):
    def test_matching_variant_does_not_double_count_or_imply_admission(self):
        s=summarize({'records':[record('functional'),record('match','0x00401000','bm',integration='object_only')]})
        self.assertEqual(s['total'],1)
        self.assertEqual(s['counts'],{'bm':1})

    def test_runtime_and_compiler_helpers_are_excluded(self):
        s=summarize({'records':[record('game'),record('crt','00402000','reused','library_verified'),record('eh','00403000','not_started','compiler_support')]})
        self.assertEqual(s['total'],1)
        self.assertEqual(s['counts'],{'func':1})

    def test_pending_function_stays_in_denominator(self):
        s=summarize({'records':[record('done'),record('pending','00402000','not_started')]})
        self.assertEqual(s['total'],2)
        self.assertEqual(s['counts'],{'func':1,'not_started':1})

    def test_admission_requires_a_game_linked_target(self):
        with self.assertRaisesRegex(ValueError,'Invalid game admission'):
            summarize({'records':[record('invalid',status='bm',integration='object_only',admitted=True)]})
        s=summarize({'records':[record('valid',integration='scenario_exercised',admitted=True)]})
        self.assertEqual(s['counts']['admitted'],1)

    def test_duplicate_ids_and_unknown_status_fail(self):
        with self.assertRaisesRegex(ValueError,'Duplicate implementation'):
            summarize({'records':[record('same'),record('same')]})
        with self.assertRaisesRegex(ValueError,'Unknown implementation'):
            summarize({'records':[record('bad',status='doneish')]})

    def test_empty_catalog_is_not_complete(self):
        self.assertIn('0% (0/0)',bar(0,0))

if __name__=='__main__': unittest.main()
