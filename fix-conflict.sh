#!/bin/bash
sed -i '/^<<<<<<</,/^>>>>>>/d' rp2040/picoToEsp32UART/README.md
sed -i 's/sunc/sync/g; s/its intended/their intended/g' rp2040/picoToEsp32UART/README.md
