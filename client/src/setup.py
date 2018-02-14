from setuptools import setup

setup(
    name="pdiprog",
    version="0.1",
    author="Kier Davis",
    author_email="kierdavis@gmail.com",
    platforms="ALL",

    packages=["pdiprog"],
    entry_points={
        'console_scripts': [
            'pdiprog = pdiprog:main',
        ],
    },
)
