#!/bin/bash

echo $PATH

# Build application.
qmake .. "USE_WEBENGINE=$USE_WEBENGINE"
