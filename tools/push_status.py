"""Refresh recorded README status, commit this checkout, then push without force."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
sys.dont_write_bytecode=True
from mkstatus import ROOT, refresh


class PublishError(RuntimeError):
    pass


def git(root, *args, allowed=(0,)):
    result=subprocess.run(['git', *args], cwd=root, capture_output=True,
                          text=True, encoding='utf-8', errors='replace')
    if result.returncode not in allowed:
        raise PublishError('git '+args[0]+' failed (exit '+str(result.returncode)+'):\n'+
                           (result.stderr+result.stdout).strip())
    return result


def publish(root, message=None, dry_run=False):
    top=Path(git(root,'rev-parse','--show-toplevel').stdout.strip()).resolve()
    if top!=root.resolve():
        raise PublishError('Command must belong to the repository root: '+str(root))
    branch=git(root,'symbolic-ref','--quiet','--short','HEAD',allowed=(0,1)).stdout.strip()
    if not branch:
        raise PublishError('Detached HEAD: check out the intended branch before pushing.')
    if git(root,'ls-files','--unmerged').stdout:
        raise PublishError('Resolve the merge conflicts before committing or pushing.')
    for operation in ('MERGE_HEAD','CHERRY_PICK_HEAD','REVERT_HEAD','rebase-merge','rebase-apply'):
        path=Path(git(root,'rev-parse','--git-path',operation).stdout.strip())
        if (root/path).exists():
            raise PublishError('Finish the current Git operation before pushing: '+operation)
    remote=git(root,'config','--get','branch.'+branch+'.remote',allowed=(0,1)).stdout.strip() or 'origin'
    if remote not in git(root,'remote').stdout.splitlines():
        raise PublishError('No configured push remote '+remote+'. Configure a remote first.')
    destination=git(root,'config','--get','branch.'+branch+'.merge',allowed=(0,1)).stdout.strip() or 'refs/heads/'+branch
    git(root,'check-ref-format',destination)
    print('Branch: '+branch+' -> '+remote+'/'+destination.removeprefix('refs/heads/'), flush=True)
    if dry_run:
        summary, changed=refresh(root,dry_run=True)
        print('DRY RUN: README '+('would update' if changed else 'is current')+
              '; '+str(summary['recovered'])+'/'+str(summary['total'])+' restored functions.')
        print('Would stage all non-ignored changes, commit if needed, and push HEAD. No files, index, commits or remote changed.')
        return
    lock=Path(git(root,'rev-parse','--git-path','bhunter-push-status.lock').stdout.strip())
    lock=root/lock
    try:
        descriptor=os.open(lock,os.O_CREAT|os.O_EXCL|os.O_WRONLY)
    except FileExistsError:
        raise PublishError('Another push/status run is active (lock: '+str(lock)+').')
    try:
        with os.fdopen(descriptor,'w') as handle:
            handle.write(str(os.getpid())+'\n')
        print('Refreshing README from the function ledger and recorded checkpoint...',flush=True)
        summary,_=refresh(root)
        print('Status: '+str(summary['recovered'])+'/'+str(summary['total'])+' restored functions.',flush=True)
        git(root,'add','--all','--','.')
        staged=git(root,'diff','--cached','--quiet',allowed=(0,1)).returncode==1
        if staged:
            if not message:
                message='Update restoration checkpoint ('+str(summary['recovered'])+'/'+str(summary['total'])+' functions)'
            git(root,'commit','--quiet','-m',message)
            print('Committed '+git(root,'rev-parse','--short','HEAD').stdout.strip(),flush=True)
        else:
            print('Nothing to commit; checking for commits still needing push.',flush=True)
        # Hooks can alter files: do not report an incompletely committed tree as success.
        if git(root,'status','--porcelain').stdout:
            raise PublishError('Files changed during the commit. Review them and run the command again; nothing pushed.')
        print('Pushing '+remote+' (ordinary push; no force or automatic merge)...',flush=True)
        result=git(root,'push','--porcelain','--set-upstream',remote,'HEAD:'+destination)
        if result.stdout.strip():print(result.stdout.strip())
        print('SUCCESS: README updated, checkout committed, and push completed.',flush=True)
    finally:
        lock.unlink(missing_ok=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('message',nargs='?',help='Optional commit message, in quotes')
    parser.add_argument('--dry-run',action='store_true',help='Preview without writing, committing or pushing')
    parser.add_argument('--check',action='store_true',help='Check recorded README status without committing or pushing')
    args=parser.parse_args()
    try:
        if args.check:
            summary,_=refresh(ROOT,check=True)
            print('README is current: '+str(summary['recovered'])+'/'+str(summary['total'])+' restored functions.')
        else:
            publish(ROOT,args.message,args.dry_run)
    except (OSError,ValueError,KeyError,PublishError) as error:
        print('PUSH FAILED: '+str(error)+'\nLocal changes/commits are retained. Fix the reported problem and rerun; no reset or force push is performed.',file=sys.stderr)
        return 1
    return 0


if __name__=='__main__':sys.exit(main())
