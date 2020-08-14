#!/bin/bash

docker build -t plzdie:latest . && docker run -it --entrypoint /src/script.sh plzdie:latest