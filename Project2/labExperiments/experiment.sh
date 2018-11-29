#!/bin/bash

/etc/init.d/networking restart

ifconfig eth0 192.168.41.1/24