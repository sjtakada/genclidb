#!/bin/sh

exec ./cli -m ../cli.json/quagga.cli_mode.json -j ../cli.json -p 'zebra/api' -n `cat ../www/null_key.txt`