from setuptools import find_packages, setup

package_name = 'measure_latency'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='harim',
    maintainer_email='halim9512@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    entry_points={
        'console_scripts': [
            'pub = measure_latency.pub_node:main',
            'sub = measure_latency.sub_node:main',
        ],
    },
)
