#! /usr/bin/python

import os
import bcommon as COMMON


class ThirdPartyBuildSystemLinux(COMMON.ThirdPartyBuildSystemBase):

    def __init__(self):
        super(ThirdPartyBuildSystemLinux, self).__init__()

    def build_lame(self):
        lame_path = self.only_name_from_arch(
            arch_file=COMMON.LAME_SRC_DIST,
            arch_ext='.tar.gz')
        lame_path = os.path.join(
            COMMON.TPS_BUILD_TMP_DIR, lame_path)
        print 'Building lame in "%s":' % lame_path
        with COMMON.build_tmp_folder_context(lame_path):
            cmd = ' '.join([
                './configure',
                '--enable-shared=no',
                '--enable-static=yes',
                '--prefix="%s"' % COMMON.TPS_INSTALL_DIR,
                '--enable-nasm'
            ])
            self.exec_cmd(cmd)
            self.exec_cmd('make')
            self.exec_cmd('make install')


def main():
    tps = ThirdPartyBuildSystemLinux()

    print 'Started building linux 3rdparty.'

    try:

        print ''
        print 'Extracting dependencies...'
        tps.extract_deps()
        print 'Extracted - OK.'

        print ''
        print 'Building lame...'
        tps.build_lame()
        print 'Built - OK.'

    except RuntimeError as e:
        print 'ERROR: ', e.message
        exit(1)

    exit(0)


if __name__ == '__main__':
    main()
