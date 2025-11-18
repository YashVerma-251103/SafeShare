#!/bin/bash

BUILD_PATH="./build/src/client/safeshare-client"

# 1. Ensure binary exists
if [ ! -f "$BUILD_PATH" ]; then
    echo "Error: Client binary not found at $BUILD_PATH"
    exit 1
fi

# 2. Check if user provided arguments
if [ $# -eq 0 ]; then
    echo "Usage: $0 <command> [args...]"
    echo ""
    echo "Commands:"
    echo "  discover"
    echo "  request  <ip> <port> <name> <reason>"
    echo "  list     <ip> <port> <token>"
    echo "  download <ip> <port> <token> <remote_file> <local_file>"
    exit 1
fi

# 3. Run the client with passed arguments
"$BUILD_PATH" "$@"