import copy, unittest
from terminal_records import compare_native
from terminal_aliases import aliases

class TerminalRecordTests(unittest.TestCase):
    def fixture(self):
        fields=[];rows=[]
        for offset,english,match,target in [(4,'Record',1,'名称'),(60,'Inside [Storage].',1,'在[仓库]内。'),(88,'Inside Storage.',0,''),(120,'',0,''),(148,'',0,'')]:
            raw=english.encode().hex()
            fields.append(dict(field_offset=offset,raw_hex=raw))
            rows.append(dict(ordinal='0',field_offset=str(offset),native_id='0',raw_hex=raw,decoded_hex=raw,match=str(match),target_hex=target.encode().hex()))
        return dict(source_sha256='fixture',records=[dict(ordinal=0,fields=fields)]),rows
    def test_exact_same_record_variant(self):
        bank,rows=self.fixture();result,evidence=aliases(bank,rows)
        self.assertEqual(result,{'Inside Storage.':'在[仓库]内。'})
        self.assertEqual(len(evidence),1)
    def test_different_words_not_alias(self):
        bank,rows=self.fixture();rows[2]['decoded_hex']=b'Outside Storage.'.hex()
        self.assertEqual(aliases(bank,rows)[0],{})
    def test_stale_native_export_rejected(self):
        bank,rows=self.fixture();rows[2]['raw_hex']='ff'
        with self.assertRaises(ValueError):aliases(bank,rows)
    def test_missing_or_duplicate_field_rejected(self):
        bank,rows=self.fixture()
        for wrong in (rows[:-1],rows+[copy.deepcopy(rows[0])]):
            with self.assertRaises(ValueError):compare_native(bank,wrong)
    def test_ambiguous_match_not_copied(self):
        bank,rows=self.fixture();rows[1]['match']='-2'
        self.assertEqual(aliases(bank,rows)[0],{})
if __name__=='__main__':unittest.main()
