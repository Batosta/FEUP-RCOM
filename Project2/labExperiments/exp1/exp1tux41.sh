#!/bin/bash

/etc/init.d/networking restart

ifconfig eth0 192.168.40.1/24

arp -d 192.168.40.254