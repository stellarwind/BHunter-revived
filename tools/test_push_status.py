"""Exercise real commits/pushes only against isolated local bare repositories."""
import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from mkstatus import START, END, refresh
from push_status import publish, PublishError


class PushStatusTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='bhunter push test ')
        self.addCleanup(self.temp.cleanup)
        self.base=Path(self.temp.name);self.repo=self.base/'source checkout';self.repo.mkdir()
        self.remote=self.base/'remote.git'
        self.git('init','-q','-b','main')
        subprocess.run(['git','init','--bare','-q',str(self.remote)],check=True,capture_output=True)
        self.git('config','user.name','Test');self.git('config','user.email','test@example.invalid')
        self.git('config','core.autocrlf','false')
        self.git('remote','add','origin',str(self.remote))
        (self.repo/'META/checkpoint').mkdir(parents=True)
        record=dict(id='owner',original_va='00401000',role='game',implementation='func',integration='linked_harness',game_admitted=False,evidence=['META/checkpoint/validation.json'])
        self.write_json('META/function-status.json',dict(records=[record],role_catalog='META/roles.json'))
        self.write_json('META/roles.json',dict(records=[dict(address='00401000',role='game'),dict(address='00402000',role='unknown')]))
        self.write_json('META/checkpoint/validation.json',dict(passed=True,complete_new_functions=1,translation_units=3,real_total=dict(checks=11,failures=0),clean_total=dict(checks=9,failures=0),missing_module_controls=2))
        (self.repo/'META/checkpoint/README.md').write_text('Evidence')
        (self.repo/'README.md').write_text('Before\n'+START+'\nstale\n'+END+'\nAfter\n')
        (self.repo/'notes.txt').write_text('old');(self.repo/'remove.txt').write_text('old')
        self.git('add','--all');self.git('commit','-qm','Initial fixture')

    def git(self,*args,allowed=(0,)):
        result=subprocess.run(['git',*args],cwd=self.repo,capture_output=True,text=True)
        self.assertIn(result.returncode,allowed,result.stderr)
        return result.stdout.strip()

    def write_json(self,name,value):
        (self.repo/name).write_text(json.dumps(value),encoding='utf-8')

    def run_publish(self,**kwargs):
        with contextlib.redirect_stdout(io.StringIO()):publish(self.repo,**kwargs)

    def remote_head(self):
        return subprocess.check_output(['git','--git-dir='+str(self.remote),'rev-parse','refs/heads/main'],text=True).strip()

    def test_refresh_commit_all_and_push_to_local_remote(self):
        (self.repo/'notes.txt').write_text('staged');self.git('add','notes.txt')
        (self.repo/'notes.txt').write_text('latest unstaged text')
        (self.repo/'new.txt').write_text('new');(self.repo/'remove.txt').unlink()
        self.run_publish(message='Complete checkpoint')
        self.assertEqual(self.git('rev-parse','HEAD'),self.remote_head())
        self.assertEqual(self.git('status','--porcelain'),'')
        self.assertEqual(self.git('show','HEAD:notes.txt'),'latest unstaged text')
        self.assertIn('new.txt',self.git('ls-tree','--name-only','HEAD'))
        self.assertNotIn('remove.txt',self.git('ls-tree','--name-only','HEAD'))
        text=(self.repo/'README.md').read_text()
        self.assertIn('50.0% (1/2)',text);self.assertIn('11 real-asset checks',text)
        self.assertTrue(text.startswith('Before\n'));self.assertTrue(text.endswith('After\n'))
        self.assertEqual(self.git('rev-parse','--abbrev-ref','@{upstream}'),'origin/main')

    def test_dry_run_does_not_mutate_files_index_or_commits(self):
        readme=(self.repo/'README.md').read_bytes();index=(self.repo/'.git/index').read_bytes();head=self.git('rev-parse','HEAD')
        self.run_publish(dry_run=True)
        self.assertEqual(readme,(self.repo/'README.md').read_bytes())
        self.assertEqual(index,(self.repo/'.git/index').read_bytes())
        self.assertEqual(head,self.git('rev-parse','HEAD'))
        result=subprocess.run(['git','--git-dir='+str(self.remote),'show-ref'],capture_output=True)
        self.assertEqual(result.returncode,1)

    def test_no_changes_still_pushes_an_outstanding_commit(self):
        self.run_publish();self.git('commit','--allow-empty','-qm','Pending local commit')
        head=self.git('rev-parse','HEAD');self.run_publish()
        self.assertEqual(head,self.git('rev-parse','HEAD'));self.assertEqual(head,self.remote_head())

    def test_bad_status_stops_before_staging_or_committing(self):
        (self.repo/'README.md').write_text('Missing markers')
        head=self.git('rev-parse','HEAD');index=(self.repo/'.git/index').read_bytes()
        with self.assertRaisesRegex(ValueError,'marker'):self.run_publish()
        self.assertEqual(index,(self.repo/'.git/index').read_bytes())
        self.assertEqual(head,self.git('rev-parse','HEAD'))
        self.assertFalse((self.repo/'.git/bhunter-push-status.lock').exists())

    def test_rejected_push_keeps_local_commit_and_remote_history(self):
        self.run_publish();other=self.base/'other checkout'
        subprocess.run(['git','clone','-q','--branch','main',str(self.remote),str(other)],check=True,capture_output=True)
        for args in [('config','user.name','Other'),('config','user.email','other@example.invalid')]:
            subprocess.run(['git',*args],cwd=other,check=True,capture_output=True)
        (other/'remote.txt').write_text('remote change')
        for args in [('add','--all'),('commit','-qm','Remote advanced'),('push','-q','origin','main')]:
            subprocess.run(['git',*args],cwd=other,check=True,capture_output=True)
        remote_before=self.remote_head();local_before=self.git('rev-parse','HEAD')
        (self.repo/'notes.txt').write_text('local work')
        with self.assertRaisesRegex(PublishError,'push failed'):self.run_publish()
        self.assertNotEqual(local_before,self.git('rev-parse','HEAD'))
        self.assertEqual(remote_before,self.remote_head())
        self.assertEqual((self.repo/'notes.txt').read_text(),'local work')
        self.assertFalse((self.repo/'.git/bhunter-push-status.lock').exists())

    def test_detached_head_fails_before_readme_change(self):
        self.git('checkout','--detach','-q');before=(self.repo/'README.md').read_bytes()
        with self.assertRaisesRegex(PublishError,'Detached'):self.run_publish()
        self.assertEqual(before,(self.repo/'README.md').read_bytes())

    @unittest.skipUnless(os.name=='nt','Windows CMD wrapper')
    def test_cmd_wrapper_from_another_directory_with_spaces_and_error_code(self):
        source=Path(__file__).resolve().parents[1];(self.repo/'tools').mkdir()
        for name in ['tools/mkstatus.py','tools/push_status.py','mkstatus.cmd','push.cmd']:
            shutil.copy2(source/name,self.repo/name)
        for wrapper in ('mkstatus.cmd','push.cmd'):
            result=subprocess.run(['cmd','/d','/c','call',str(self.repo/wrapper),'--dry-run'],cwd=self.base,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertIn('DRY RUN',result.stdout)
            result=subprocess.run(['cmd','/d','/c','call',str(self.repo/wrapper),'--invalid-option'],cwd=self.base,capture_output=True,text=True)
            self.assertEqual(result.returncode,2,result.stdout+result.stderr)
        # Exercise the exact requested entry point against a real local remote.
        result=subprocess.run(['cmd','/d','/c','call',str(self.repo/'mkstatus.cmd'),'Status checkpoint'],cwd=self.base,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertIn('SUCCESS:',result.stdout)
        self.assertEqual(self.git('rev-parse','HEAD'),self.remote_head())
        self.assertIn('50.0% (1/2)',(self.repo/'README.md').read_text())
        before=self.git('rev-parse','HEAD')
        result=subprocess.run(['cmd','/d','/c','call',str(self.repo/'mkstatus.cmd'),'--check'],cwd=self.base,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertIn('README is current',result.stdout)
        self.assertEqual(before,self.git('rev-parse','HEAD'))


if __name__=='__main__':unittest.main()
