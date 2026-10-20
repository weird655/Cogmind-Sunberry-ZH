from pathlib import Path
import tempfile,unittest
from audit_runtime_routes import report
from audit_text_routes import audit
ROOT=Path(__file__).resolve().parents[1]
class RuntimeCoverageTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.path=Path(self.temp.name)/'routes.log';self.names=[r['name'] for r in audit(ROOT)['entries']]
 def tearDown(self):self.temp.cleanup()
 def lines(self):return [f'ROUTE entry={name} calls=0 matched=0 missing=0 ambiguous=0 invalid=0 bound=0 layouts=0 layout-failures=0' for name in self.names]
 def test_unseen_is_not_missing_hook(self):
  self.path.write_text('\n'.join(self.lines()));r=report(ROOT,self.path);self.assertEqual(r['observed'],[]);self.assertEqual(len(r['not_yet_observed']),len(self.names))
 def test_truncated_snapshot_rejected(self):
  self.path.write_text('\n'.join(self.lines()[:-1]))
  with self.assertRaises(ValueError):report(ROOT,self.path)
 def test_inconsistent_counters_rejected(self):
  rows=self.lines();rows[0]=rows[0].replace('matched=0','matched=1');self.path.write_text('\n'.join(rows))
  with self.assertRaises(ValueError):report(ROOT,self.path)
if __name__=='__main__':unittest.main()
