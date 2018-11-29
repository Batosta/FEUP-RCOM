#!/bin/bash

/etc/init.d/networking restart

ifconfig eth0 192.168.40.254/24