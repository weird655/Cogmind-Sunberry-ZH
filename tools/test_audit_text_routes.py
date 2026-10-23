from pathlib import Path
import shutil,tempfile,unittest
from audit_text_routes import audit
ROOT=Path(__file__).resolve().parents[1]
class RouteGuardTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name);shutil.copytree(ROOT/'src',self.root/'src')
 def tearDown(self):self.temp.cleanup()
 def test_current_routes(self):self.assertTrue(audit(self.root)['passed'])
 def test_unknown_adapter_bypass(self):
  (self.root/'src/new_popup.h').write_text('int x=zh_translate(dict,s,n,out,cap,0);')
  self.assertFalse(audit(self.root)['passed'])
 def test_unknown_entry(self):
  (self.root/'src/new_popup.h').write_text('message_resolve_entry(TEXT_ENTRY_UNKNOWN,s,n,out,cap,0);')
  self.assertFalse(audit(self.root)['passed'])
 def test_new_dialogue_helper_cannot_bypass_entry(self):
  (self.root/'src/new_popup.h').write_text('dialogue_dynamic_translate(a,b,c,d,s,n,out,cap);')
  self.assertFalse(audit(self.root)['passed'])
 def test_commented_example_is_not_code(self):
  (self.root/'src/new_popup.h').write_text('/* zh_translate(dict,s,n,out,cap,0); */')
  self.assertTrue(audit(self.root)['passed'])
 def test_registry_missing_row(self):
  p=self.root/'src/text_context.h';p.write_text(p.read_text().replace('{TEXT_GROUP_DISPLAY,TEXT_SEM_DISPLAY,"display.scratch"}',''))
  self.assertFalse(audit(self.root)['passed'])
if __name__=='__main__':unittest.main()
