import os
import shutil
import urllib
import urllib2
import tarfile
import contextlib


BASE_DIR = os.path.abspath(os.path.dirname(__file__))

# TPS_ - third party sources
TPS_SOURCES_DIR = os.path.join(BASE_DIR, 'sources')
TPS_BUILD_TMP_DIR = os.path.join(BASE_DIR, 'build.tmp')
TPS_INSTALL_DIR = os.path.join(BASE_DIR, 'build')

LAME_SRC_DIST = os.path.join(TPS_SOURCES_DIR, 'lame-3.99.5.tar.gz')


@contextlib.contextmanager
def build_tmp_folder_context(tmp_folder):
    tmp_folder = os.path.abspath(tmp_folder)
    old_dir = os.getcwd()
    os.chdir(tmp_folder)
    try:
        yield
    finally:
        os.chdir(old_dir)


class ThirdPartyBuildSystemBase(object):

    def __init__(self):
        pass
        
    def name_from_arch(self, arch_file):
        n = arch_file.rfind('.tar.gz')
        if n == -1:
            return arch_file
        return arch_file[0:n]

    def only_name_from_arch(self, arch_file, arch_ext='.tar.gz'):
        bn = os.path.basename(arch_file)
        n = bn.rfind(arch_ext)
        if n == -1:
            return bn
        return bn[0:n]
        
    # run a command
    def system(self, cmd):
        print "RUN:", cmd
        return os.system(cmd)

    def exec_cmd(self, cmd):
        res = self.system(cmd)
        print ''
        if res != 0:
            raise RuntimeError('[%s] - command failed' % cmd)
        
    # make a directory
    def mkdir(self, dir):
        try:
            os.mkdir(dir)
        except:
            pass
        else:
            print "MKDIR: ", dir
            
    def rmdir(self, dir):
        if not os.path.exists(dir):
            return
        shutil.rmtree(dir)
        print 'RMDIR: ', dir
        
    def rmfile(self, file):
        if not os.path.exists(file):
            return
        os.remove(file)
        print 'RMFILE: ', file
        
        
    def extract(self, file, where):
        tar = tarfile.open(file)
        tar.extractall(where)
        tar.close()
        print 'EXTRACTED: ', file

    def extract_deps(self):
        self.rmdir(TPS_INSTALL_DIR)
        self.rmdir(TPS_BUILD_TMP_DIR)
        self.mkdir(TPS_BUILD_TMP_DIR)

        self.extract(LAME_SRC_DIST, TPS_BUILD_TMP_DIR)

        self.mkdir(TPS_INSTALL_DIR)
        self.mkdir(os.path.join(TPS_INSTALL_DIR, 'bin'))
        self.mkdir(os.path.join(TPS_INSTALL_DIR, 'include'))
        self.mkdir(os.path.join(TPS_INSTALL_DIR, 'lib'))
