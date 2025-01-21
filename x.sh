#!/bin/bash
cd ../VRemoteStorage-submodule && make && scp server_file_module.ko 192.168.122.79: && scp server_file_module.ko 192.168.122.78:
