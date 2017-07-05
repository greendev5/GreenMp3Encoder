#! /usr/bin/python

import os
import subprocess
import sys

import bcommon as COMMON


class ThirdPartyBuildSystemLWin32(COMMON.ThirdPartyBuildSystemBase):

    def __init__(self):
        super(ThirdPartyBuildSystemLWin32, self).__init__()

    @staticmethod
    def _convert_mbcs(s):
        dec = getattr(s, "decode", None)
        if dec is not None:
            try:
                s = dec("mbcs")
            except UnicodeError:
                pass
        return s
    
    @staticmethod
    def _remove_duplicates(variable):
        """Remove duplicate values of an environment variable.
        """
        oldList = variable.split(os.pathsep)
        newList = []
        for i in oldList:
            if i not in newList:
                newList.append(i)
        newVariable = os.pathsep.join(newList)
        return newVariable
    
    @staticmethod
    def _normalize_and_reduce_paths(paths):
        """Return a list of normalized paths with duplicates removed.

        The current order of paths is maintained.
        """
        # Paths are normalized so things like:  /a and /a/ aren't both preserved.
        reduced_paths = []
        for p in paths:
            np = os.path.normpath(p)
            # XXX(nnorwitz): O(n**2), if reduced_paths gets long perhaps use a set.
            if np not in reduced_paths:
                reduced_paths.append(np)
        return reduced_paths
    
    def _query_vcvarsall(self):
        """Launch vcvarsall.bat and read the settings from its environment
        """
        
        arch="x86"
        vcvarsall = os.path.join(COMMON.TPS_VCHOME, 'bin', 'vcvars32.bat')
        
        interesting = set(("include", "lib", "libpath", "path"))
        result = {}
        
        try:
            popen = subprocess.Popen('"%s" %s & set' % (vcvarsall, arch),
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE)
        except OSError:
            raise RuntimeError('Could not open "%s".' % vcvarsall)

        stdout, stderr = popen.communicate()
        if popen.wait() != 0:
            raise RuntimeError(stderr.decode("mbcs"))

        stdout = stdout.decode("mbcs")
        for line in stdout.split("\n"):
            line = self._convert_mbcs(line)
            if '=' not in line:
                continue
            line = line.strip()
            key, value = line.split('=', 1)
            key = key.lower()
            if value.endswith(os.pathsep):
                value = value[:-1]
            result[key] = self._remove_duplicates(value)

        return result
        
    def check_win32_env(self):
        vc_env = self._query_vcvarsall()
        
        # adding env vars except path
        for k,v in vc_env.iteritems():
            if k.upper() not in os.environ and k.upper() != 'PATH':
                os.environ[k.upper()] = v
        
        # take care to only use strings in the environment.
        paths = vc_env['path'].split(os.pathsep)
        for p in os.environ['path'].split(';'):
            paths.append(p)
        #append NASM path:
        paths = self._normalize_and_reduce_paths(paths)
        os.environ['path'] = ";".join(paths)
        
        res = self.system('msbuild /version')
        print ''
        if res != 0:
            raise RuntimeError('msbuild command unavailable')
        
        self.exec_cmd('%s --version' % sys.executable)
        
    def build_lame(self):
        build_dir = os.path.join(
            COMMON.TPS_BUILD_TMP_DIR,
            self.only_name_from_arch(COMMON.LAME_SRC_DIST))
            
        with COMMON.build_tmp_folder_context(build_dir):
            self.exec_cmd('copy configMS.h config.h')
            
            replacements = {'/opt:NOWIN98':'', '/MT': '/MD'}
            lines = []
            with open('Makefile.MSVC') as infile:
                for line in infile:
                    for src, target in replacements.iteritems():
                        line = line.replace(src, target)
                    lines.append(line)
            with open('Makefile.MSVC', 'w') as outfile:
                for line in lines:
                    outfile.write(line)
                    
            self.exec_cmd('nmake -f Makefile.MSVC comp=msvc asm=no gtk=no cpu=p3')
            
            target_include_dir = os.path.join(COMMON.TPS_INSTALL_DIR, 'include', 'lame')
            self.mkdir(target_include_dir)
            target_lib_dir = os.path.join(COMMON.TPS_INSTALL_DIR, 'lib')
            self.exec_cmd('copy include\\*.h %s' % target_include_dir)
            self.exec_cmd('copy output\\*.lib %s' % target_lib_dir)
            
    def build_pthreads_win32(self):
        build_dir = os.path.join(
            COMMON.TPS_BUILD_TMP_DIR,
            self.only_name_from_arch(COMMON.PTHREADS_W32_SRC_DIST))
            
        with COMMON.build_tmp_folder_context(build_dir):
            self.exec_cmd('nmake clean VC-static')
            target_include_dir = os.path.join(COMMON.TPS_INSTALL_DIR, 'include')
            target_lib_dir = os.path.join(COMMON.TPS_INSTALL_DIR, 'lib')
            self.exec_cmd('copy pthread.h %s' % target_include_dir)
            self.exec_cmd('copy sched.h %s' % target_include_dir)
            self.exec_cmd('copy *.lib %s' % target_lib_dir)
            

def main():
    tps = ThirdPartyBuildSystemLWin32()

    print 'Started building linux 3rdparty.'

    try:

        print ''
        print 'Extracting dependencies...'
        tps.extract_deps()
        print 'Extracted - OK.'
        
        print ''
        print 'Extracting pthreads-w32'
        tps.extract(COMMON.PTHREADS_W32_SRC_DIST, COMMON.TPS_BUILD_TMP_DIR)
        print 'Extracted - OK.'
        
        print 'Checking win32 build env...'
        tps.check_win32_env()
        print 'win32 build env - OK.'
        
        print ''
        print 'Building lame...'
        tps.build_lame()
        print 'Built - OK.'
        
        print ''
        print 'Building pthreads-w32...'
        tps.build_pthreads_win32()
        print 'Built - OK.'

    except RuntimeError as e:
        print 'ERROR: ', e.message
        exit(1)

    exit(0)


if __name__ == "__main__":
     # parse options
    import optparse, codecs
    codecs.register(lambda name: codecs.lookup('utf-8') if name == 'cp65001' else None) # windows UTF-8 hack
    
    main()
