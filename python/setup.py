from distutils.core import setup

long_description = open("README.rst").read()
long_description += "\n.. code-block:: python\n\n    "
long_description += "\n    ".join(open("output_example.py").read().split("\n"))

setup(
    name="radiance",
    version="0.1dev5",
    packages=["radiance",],
    license="MIT",
    description="Python tools for the Radiance video art system",
    long_description_content_type="text/x-rst",
    long_description=long_description,
    url="https://radiance.video",
    author="Eric Van Albert",
    author_email="eric@van.al",
)
