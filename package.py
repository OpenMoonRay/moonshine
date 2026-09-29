# Copyright 2024-2025 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

# -*- coding: utf-8 -*-
import os

name = 'moonshine'

if 'early' not in locals() or not callable(early):
    def early(): return lambda x: x

@early()
def version():
    """
    Increment the build in the version.
    """
    _version = '15.6'
    from rezbuild import earlybind
    return earlybind.version(this, _version)

description = 'Shaders for Moonray'

authors = [
    'PSW Rendering and Shading',
    'moonbase-dev@dreamworks.com',
]

help = ('For assistance, '
        "please contact the folio's owner at: moonbase-dev@dreamworks.com")

variants = [
    [   # variant 0
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2023.1',
        'gcc-11.x'
    ],
    [   # variant 1
        'os-rocky-9',
        'opt_level-debug',
        'refplat-vfx2023.1',
        'gcc-11.x'
    ],
    [   # variant 2
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2023.1',
        'clang-17.0.6.x'
    ],
    [   # variant 3
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2024.0',
        'gcc-11.x'
    ],
    [   # variant 4
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2025.0',
        'gcc-11.x'
    ],
    [   # variant 5
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-houdini21.0',
        'gcc-11.x'
    ],
    [   # variant 6
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2026.0',
        'gcc-14.2.x'
    ],
    [   # variant 7
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2022.0',
        'gcc-9.3.x.1'
    ],
]

conf_rats_variants = variants[0:2]
conf_CI_variants = variants

# Add ephemeral package to each variant.
for i, variant in enumerate(variants):
    variant.insert(0, '.moonshine_variant-%d' % i)

requires = [
    'moonray-18.4',
    'scene_rdl2-16.2',
]

private_build_requires = [
    'cmake_modules-2.0',
    'cppunit',
    'ispc-1.20.0.x',
    'python-3.9|3.10|3.11|3.13'
]

commandstr = lambda i: "cd build/"+os.path.join(*variants[i])+"; ctest -j $(nproc)"
testentry = lambda i: ("variant%d" % i,
                       { "command": commandstr(i),
                        "requires": ["cmake"],
                        "on_variants": {
                            "type": "requires",
                            "value": [".moonshine_variant-%d" % i]
                            },
                        "run_on": "explicit",
                        }, )
testlist = [testentry(i) for i in range(len(variants))]
tests = dict(testlist)

def commands():
    prependenv('CMAKE_MODULE_PATH', '{root}/lib64/cmake')
    prependenv('CMAKE_PREFIX_PATH', '{root}')
    prependenv('SOFTMAP_PATH', '{root}')
    prependenv('MOONRAY_DSO_PATH', '{root}/rdl2dso')
    prependenv('RDL2_DSO_PATH', '{root}/rdl2dso')
    prependenv('LD_LIBRARY_PATH', '{root}/lib64')
    prependenv('PATH', '{root}/bin')
    prependenv('MOONRAY_CLASS_PATH', '{root}/coredata')

uuid = 'f778de67-e145-411e-b958-b792e5ca016a'

config_version = 0
