import gzip,hashlib,struct,tempfile,unittest
from pathlib import Path
from record_identity import identify,candidates
class IdentityTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name);packed=gzip.compress(struct.pack('<I',1)+b'A'+struct.pack('<I',1)+b'A');(self.root/'particles.bin').write_bytes(packed);digest=hashlib.sha256(packed).hexdigest()
  self.rows=[{'source_file':'particles.bin','source_sha256':digest,'prefix_offset':hex(i*5),'payload_offset':hex(i*5+4),'byte_length':'1','record_path':'item[0].native_vector','decode_status':'decoded','original_english':'A'} for i in range(2)]
 def tearDown(self):self.temp.cleanup()
 def test_repeated_path_keeps_both_records(self):
  rs=identify(self.rows,self.root);self.assertNotEqual(rs[0]['identity'],rs[1]['identity']);self.assertTrue(all(r['semantic_path_ambiguous'] for r in rs));self.assertEqual(candidates(rs,'A')['status'],'ambiguous');self.assertEqual(candidates(rs,'B')['status'],'missing')
 def test_stale_hash(self):
  self.rows[0]['source_sha256']='0'*64
  with self.assertRaises(ValueError):identify(self.rows,self.root)
 def test_wrong_length_prefix(self):
  self.rows[0]['byte_length']='2'
  with self.assertRaises(ValueError):identify(self.rows,self.root)
 def test_duplicate_physical_record(self):
  with self.assertRaises(ValueError):identify([self.rows[0],self.rows[0]],self.root)
if __name__=='__main__':unittest.main()
