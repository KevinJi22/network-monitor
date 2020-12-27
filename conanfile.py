from conans import ConanFile, CMake

class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"

    generators = 'cmake_find_package'

    requires = [
        ('boost/1.74.0'),
    ]
    
    # download Boost static libraries instead of shared ones
    # this embeds library code into executable and prevents us from needing to install the shared library files
    default_options = (
        'boost:shared=False',
    )
