import setuptools
from cmake_setuptools import *
setuptools.setup(
     name='timeswipe',
     version='0.3',
     ext_modules=[CMakeExtension('timeswipe_py')],
     cmdclass={'build_ext': CMakeBuildExt},
     author="Ilya Gavrilov",
     author_email="gilyav@gmail.com",
     description="TimeSwipe python3 module",
     long_description="pydoc timeswipe",
     long_description_content_type="text/markdown",
     url="https://github.com/panda-official/TimeSwipe",
     packages=setuptools.find_packages(),
     install_requires = ['cmake_setuptools'],
     classifiers=[
         "Programming Language :: Python :: 3",
         "License :: OSI Approved :: MIT License",
         "Operating System :: OS Independent",
     ],
 )
